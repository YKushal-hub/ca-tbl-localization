/**
 * \file ca_tbl_packet_validator.cc
 * \brief CA-TBL Phase S4 -- validate the packet/MAC layer against its own spec.
 *
 * Why a stub PHY and not the real engines
 * ---------------------------------------
 * This runs the MAC against a deterministic stub CaTblPhyProvider, deliberately.
 * The physics engines are already validated separately (results/metric*.csv). What
 * has never been tested is the protocol: whether the ToA formula matches the
 * datasheet, whether the duty-cycle gate actually refuses a frame, whether the
 * priority contention windows separate access delay, whether flood dedup
 * terminates. Running those checks through the stochastic engines would mean a
 * failure could be either a MAC bug or a channel draw, which is exactly the
 * ambiguity that made the pre-S2.5 results unreadable.
 *
 * A physics-backed scenario is the next step. This is the layer under it.
 *
 * Each test states its own pass criterion. Where a criterion is an empirical
 * question rather than a spec requirement, it prints the number and says so
 * instead of asserting -- notably Test 4: whether priority contention windows
 * produce a measurable latency separation is a result, not a guarantee.
 *
 * Usage:
 *   ./ns3 run "ca_tbl_packet_validator"
 *   ./ns3 run "ca_tbl_packet_validator --nodes=12 --contenders=10 --outdir=results/s4"
 */

#include "ns3/core-module.h"
#include "ns3/ca-tbl-energy-model.h"
#include "ns3/ca-tbl-header.h"
#include "ns3/ca-tbl-mac.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

namespace
{

int g_pass = 0;
int g_fail = 0;
std::vector<std::string> g_failures;

void
Check(const std::string& name, bool ok, const std::string& detail = "")
{
    if (ok)
    {
        ++g_pass;
        std::cout << "  PASS  " << name << "\n";
    }
    else
    {
        ++g_fail;
        g_failures.push_back(name + (detail.empty() ? "" : " -- " + detail));
        std::cout << "  FAIL  " << name << (detail.empty() ? "" : "  [" + detail + "]") << "\n";
    }
}

void
Report(const std::string& name, const std::string& value)
{
    std::cout << "  ----  " << name << " = " << value << "\n";
}

/**
 * \brief Deterministic PHY stub.
 *
 * Two topologies:
 *   FULL_MESH -- every node hears every other at identical power. The worst case
 *                for contention: CAD always sees a busy channel, and every
 *                overlap is a collision unless capture applies. This is what
 *                makes the priority-CW test meaningful.
 *   LINE      -- node i hears only i-1 and i+1. Produces genuine hidden
 *                terminals and a multi-hop path for the flood test.
 */
class StubPhy : public CaTblPhyProvider
{
  public:
    enum Topology
    {
        FULL_MESH,
        LINE
    };

    StubPhy(Topology t, uint32_t nNodes, uint16_t baseId)
        : m_topo(t),
          m_n(nNodes),
          m_base(baseId)
    {
    }

    CaTblLinkOutcome Evaluate(uint16_t txId,
                              uint16_t rxId,
                              CaTblTier tier,
                              double txPowerDbm,
                              uint8_t hopsTraversed) override
    {
        (void)tier;
        (void)hopsTraversed;
        CaTblLinkOutcome o;
        const int ti = static_cast<int>(txId) - static_cast<int>(m_base);
        const int ri = static_cast<int>(rxId) - static_cast<int>(m_base);

        bool audible = false;
        if (m_topo == FULL_MESH)
        {
            audible = (ti != ri);
        }
        else
        {
            audible = (std::abs(ti - ri) == 1);
        }

        if (!audible)
        {
            o.received = false;
            o.rxPowerDbm = -200.0;
            return o;
        }

        o.received = true;
        // Identical received power for every audible link. With the 6 dB capture
        // threshold this means NO overlap is ever captured -- every concurrent
        // pair is mutually destroyed. That is the pessimistic case on purpose:
        // if the priority CW cannot separate access delay here, it will not in a
        // kinder channel either.
        o.rxPowerDbm = txPowerDbm - 60.0;
        o.snrDb = 12.0;
        o.nlos = false;
        o.estimatedRangeM = 100.0;
        return o;
    }

    double SensitivityDbm(CaTblTier tier, uint8_t sf) override
    {
        (void)tier;
        (void)sf;
        return -137.0;
    }

  private:
    Topology m_topo;
    uint32_t m_n;
    uint16_t m_base;
};

// ---------------------------------------------------------------------------
// Test 1 -- Time on air against the Semtech formula
// ---------------------------------------------------------------------------

void
TestToa(const std::string& outdir)
{
    std::cout << "\n[1] Time-on-air (Semtech AN1200.13)\n";

    // The single-source-of-truth check. ca-tbl-header.h documents the formula for
    // the report; CaTblEnergyModel::LoRaTimeOnAirS is what the MAC actually
    // charges. Two implementations of one formula is a defect waiting to happen,
    // so assert they agree rather than trusting that they do.
    double worstDelta = 0.0;
    for (uint8_t sf = 7; sf <= 12; ++sf)
    {
        for (uint32_t pl = 9; pl <= 120; pl += 13)
        {
            const bool de = CaTblRecommendedLowDataRateOpt(sf, 125e3);
            const double a = CaTblTimeOnAir(pl, sf, 125e3, 1, true, true, de).GetSeconds();
            const double b = CaTblEnergyModel::LoRaTimeOnAirS(sf, 125e3, pl, 1);
            worstDelta = std::max(worstDelta, std::fabs(a - b));
        }
    }
    Check("header ToA agrees with energy-model ToA",
          worstDelta < 1e-9,
          "max delta " + std::to_string(worstDelta) + " s");

    // Datasheet anchor: SF12/125 kHz, ~50 byte payload takes 1.5-2+ s. The design
    // document uses exactly this figure to justify why the digest mechanism has
    // to exist at all, so if it does not reproduce, that justification is void.
    const double sf12_50 = CaTblTimeOnAir(50, 12, 125e3, 1, true, true, true).GetSeconds();
    Report("ToA SF12/125kHz/50B", std::to_string(sf12_50) + " s");
    Check("SF12/125kHz 50B lands in the documented 1.5-2.5 s band",
          sf12_50 > 1.5 && sf12_50 < 2.5,
          std::to_string(sf12_50));

    // DE must be off at low SF or the denominator 4*(SF-2) distorts the count.
    Check("low-data-rate opt off at SF7/125kHz", !CaTblRecommendedLowDataRateOpt(7, 125e3));
    Check("low-data-rate opt on at SF12/125kHz", CaTblRecommendedLowDataRateOpt(12, 125e3));

    // The airtime table the report needs: ToA per message type per SF. This is
    // what turns "we designed a digest mechanism" into "here is the airtime it
    // saves", which is the only form of that claim a reviewer can check.
    std::ofstream out((outdir + "/s4_toa_by_msgtype.csv").c_str());
    out << "msg_type,msg_class,n_entries,payload_bytes,total_bytes,sf,bw_hz,toa_s,"
        << "duty_2p5_min_interval_s,duty_10_min_interval_s\n";

    const CaTblMsgType types[] = {CA_TBL_MSG_DISCOVERY,
                                  CA_TBL_MSG_DIGEST,
                                  CA_TBL_MSG_ENTRY_PUSH,
                                  CA_TBL_MSG_TRIGGERED_UPDATE,
                                  CA_TBL_MSG_ESC_REQUEST,
                                  CA_TBL_MSG_ESC_GRANT,
                                  CA_TBL_MSG_QUERY,
                                  CA_TBL_MSG_RESPONSE,
                                  CA_TBL_MSG_ACK};
    const uint32_t entryCounts[] = {1, 4, 8};

    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
    {
        for (size_t e = 0; e < 3; ++e)
        {
            const uint32_t n = entryCounts[e];
            const uint32_t pay = CaTblPayloadBytes(types[i], n);
            const uint32_t tot = CA_TBL_MAC_HEADER_BYTES + pay;
            for (uint8_t sf = 7; sf <= 12; sf += (sf == 7 ? 2 : (sf == 9 ? 3 : 1)))
            {
                const double toa = CaTblEnergyModel::LoRaTimeOnAirS(sf, 125e3, tot, 1);
                out << CaTblMsgTypeName(types[i]) << "," << CaTblMsgClassName(types[i]) << "," << n
                    << "," << pay << "," << tot << "," << static_cast<uint32_t>(sf) << "," << 125000
                    << "," << std::fixed << std::setprecision(6) << toa << ","
                    << (toa / 0.025) << "," << (toa / 0.10) << "\n";
            }
        }
    }
    out.close();
    Report("wrote", outdir + "/s4_toa_by_msgtype.csv");

    // The digest's whole purpose, quantified: 4 bytes per peer versus 28.
    const uint32_t dig = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_DIGEST, 8);
    const uint32_t push = CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(CA_TBL_MSG_ENTRY_PUSH, 8);
    const double tDig = CaTblEnergyModel::LoRaTimeOnAirS(12, 125e3, dig, 1);
    const double tPush = CaTblEnergyModel::LoRaTimeOnAirS(12, 125e3, push, 1);
    Report("SF12 airtime, 8-peer digest vs full push",
           std::to_string(tDig) + " s vs " + std::to_string(tPush) + " s");
    Check("digest is cheaper than a full push for the same peer count", tDig < tPush);
}

// ---------------------------------------------------------------------------
// Test 2 -- taxonomy consistency
// ---------------------------------------------------------------------------

void
TestTaxonomy(const std::string& outdir)
{
    std::cout << "\n[2] Message taxonomy (design doc 2.3.1, 2.8, 3.3)\n";

    Check("header is 9 bytes", CA_TBL_MAC_HEADER_BYTES == 9);
    CaTblHeader h;
    Check("serialised header is 9 bytes", h.GetSerializedSize() == 9);

    // Round-trip the header, because a Serialize/Deserialize asymmetry silently
    // corrupts packet_id, which silently breaks dedup, which silently turns the
    // flood into a broadcast storm.
    h.SetPacketId(0xDEADBEEF);
    h.SetOriginId(0xBEEF);
    h.SetHopLimit(3);
    h.SetMsgType(CA_TBL_MSG_QUERY);
    h.SetTier(2);
    Buffer buf;
    buf.AddAtStart(16);
    h.Serialize(buf.Begin());
    CaTblHeader h2;
    h2.Deserialize(buf.Begin());
    Check("header round-trips",
          h2.GetPacketId() == 0xDEADBEEF && h2.GetOriginId() == 0xBEEF &&
              h2.GetHopLimit() == 3 && h2.GetMsgType() == CA_TBL_MSG_QUERY && h2.GetTier() == 2);

    // Spec assertions, straight from the tables.
    Check("TRIGGERED_UPDATE is URGENT",
          CaTblAccessCategoryFor(CA_TBL_MSG_TRIGGERED_UPDATE) == CA_TBL_AC_URGENT);
    Check("ESC_REQUEST is HIGH",
          CaTblAccessCategoryFor(CA_TBL_MSG_ESC_REQUEST) == CA_TBL_AC_HIGH);
    Check("ENTRY_PUSH is NORMAL",
          CaTblAccessCategoryFor(CA_TBL_MSG_ENTRY_PUSH) == CA_TBL_AC_NORMAL);
    Check("DISCOVERY is LOW", CaTblAccessCategoryFor(CA_TBL_MSG_DISCOVERY) == CA_TBL_AC_LOW);

    Check("URGENT CW is 0-3 per spec",
          CaTblCwFor(CA_TBL_AC_URGENT).cwMin == 0 && CaTblCwFor(CA_TBL_AC_URGENT).cwMax == 3);
    Check("CW is strictly ordered URGENT < HIGH < NORMAL < LOW",
          CaTblCwFor(CA_TBL_AC_URGENT).cwMax < CaTblCwFor(CA_TBL_AC_HIGH).cwMax &&
              CaTblCwFor(CA_TBL_AC_HIGH).cwMax < CaTblCwFor(CA_TBL_AC_NORMAL).cwMax &&
              CaTblCwFor(CA_TBL_AC_NORMAL).cwMax < CaTblCwFor(CA_TBL_AC_LOW).cwMax);

    // 2.8: exactly one class gets a real ACK, and it is the escalation exchange.
    Check("escalation is the explicitly-ACKed class",
          CaTblAckPolicyFor(CA_TBL_MSG_ESC_REQUEST) == CA_TBL_ACK_EXPLICIT &&
              CaTblAckPolicyFor(CA_TBL_MSG_ESC_GRANT) == CA_TBL_ACK_EXPLICIT);
    Check("discovery is never ACKed",
          CaTblAckPolicyFor(CA_TBL_MSG_DISCOVERY) == CA_TBL_ACK_NONE);
    Check("triggered-update uses redundancy not ACK",
          CaTblAckPolicyFor(CA_TBL_MSG_TRIGGERED_UPDATE) == CA_TBL_ACK_REDUNDANT);

    // 3.3: the routing metric is scoped to position-fix traffic ONLY. Widening
    // this set widens the routing claim, which the design document explicitly
    // does not make.
    Check("position-fix set is exactly {ENTRY_PUSH, TRIGGERED_UPDATE, RESPONSE}",
          CaTblCarriesPositionFix(CA_TBL_MSG_ENTRY_PUSH) &&
              CaTblCarriesPositionFix(CA_TBL_MSG_TRIGGERED_UPDATE) &&
              CaTblCarriesPositionFix(CA_TBL_MSG_RESPONSE) &&
              !CaTblCarriesPositionFix(CA_TBL_MSG_DISCOVERY) &&
              !CaTblCarriesPositionFix(CA_TBL_MSG_DIGEST) &&
              !CaTblCarriesPositionFix(CA_TBL_MSG_QUERY));

    // Digest/entry-push are a pairwise anti-entropy exchange. Flooding them would
    // defeat the digest mechanism entirely.
    Check("digest and entry-push are not flooded",
          !CaTblIsFlooded(CA_TBL_MSG_DIGEST) && !CaTblIsFlooded(CA_TBL_MSG_ENTRY_PUSH));
    Check("discovery, triggered-update and query are flooded",
          CaTblIsFlooded(CA_TBL_MSG_DISCOVERY) && CaTblIsFlooded(CA_TBL_MSG_TRIGGERED_UPDATE) &&
              CaTblIsFlooded(CA_TBL_MSG_QUERY));

    std::ofstream out((outdir + "/s4_taxonomy.csv").c_str());
    out << "msg_type,msg_class,access_category,cw_min,cw_max,ack_policy,flooded,carries_position_fix\n";
    const CaTblMsgType types[] = {CA_TBL_MSG_DISCOVERY,
                                  CA_TBL_MSG_DIGEST,
                                  CA_TBL_MSG_ENTRY_PUSH,
                                  CA_TBL_MSG_TRIGGERED_UPDATE,
                                  CA_TBL_MSG_ESC_REQUEST,
                                  CA_TBL_MSG_ESC_GRANT,
                                  CA_TBL_MSG_QUERY,
                                  CA_TBL_MSG_RESPONSE,
                                  CA_TBL_MSG_ACK};
    for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
    {
        const CaTblAccessCategory ac = CaTblAccessCategoryFor(types[i]);
        const CaTblCwBounds cw = CaTblCwFor(ac);
        out << CaTblMsgTypeName(types[i]) << "," << CaTblMsgClassName(types[i]) << ","
            << CaTblAccessCategoryName(ac) << "," << cw.cwMin << "," << cw.cwMax << ","
            << CaTblAckPolicyName(CaTblAckPolicyFor(types[i])) << ","
            << (CaTblIsFlooded(types[i]) ? 1 : 0) << ","
            << (CaTblCarriesPositionFix(types[i]) ? 1 : 0) << "\n";
    }
    out.close();
    Report("wrote", outdir + "/s4_taxonomy.csv");
}

// ---------------------------------------------------------------------------
// Test 3 -- duty cycle is enforced, not merely tracked
// ---------------------------------------------------------------------------

void
TestDutyCycle()
{
    std::cout << "\n[3] WPC G.S.R. 853(E) Table-II duty cycle\n";

    for (int isAp = 0; isAp <= 1; ++isAp)
    {
        StubPhy phy(StubPhy::FULL_MESH, 2, 1);
        CaTblChannel channel;
        channel.SetPhyProvider(&phy);

        CaTblMac a;
        CaTblMac b;
        CaTblMacConfig ca;
        ca.nodeId = 1;
        ca.isAccessPoint = (isAp == 1);
        ca.sf = 12;
        ca.bandwidthHz = 125e3;
        ca.dutyWindowS = 3600.0;
        ca.maxCadRetries = 0; // isolate the duty gate from contention
        CaTblMacConfig cb = ca;
        cb.nodeId = 2;

        a.Configure(ca);
        b.Configure(cb);
        Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
        a.SetRng(rng);
        b.SetRng(rng);
        channel.Register(&a);
        channel.Register(&b);

        // Hammer the band: one 36-byte SF12 frame every 10 s for an hour. At
        // ~1.6 s ToA that is ~16% duty if unconstrained -- above both caps.
        for (int k = 0; k < 360; ++k)
        {
            Simulator::Schedule(Seconds(1.0 + 10.0 * k),
                                &CaTblMac::Send,
                                &a,
                                CA_TBL_MSG_ENTRY_PUSH,
                                CA_TBL_TIER_868,
                                1u,
                                static_cast<uint16_t>(0));
        }

        Simulator::Stop(Seconds(3700.0));
        Simulator::Run();

        const double duty = a.GetDutyCycle(CA_TBL_TIER_868);
        const double limit = a.GetDutyCycleLimit();
        const uint64_t refused = a.GetCounters(CA_TBL_MSG_ENTRY_PUSH).droppedDutyCycle;
        const std::string tag = (isAp == 1) ? "access point (10%)" : "field node (2.5%)";

        Report(tag + " achieved duty", std::to_string(duty * 100.0) + "%");
        Report(tag + " frames refused", std::to_string(refused));
        Check(tag + ": duty stays within the regulatory cap",
              duty <= limit + 1e-6,
              std::to_string(duty) + " > " + std::to_string(limit));
        Check(tag + ": the gate actually refused frames (offered load exceeds the cap)",
              refused > 0,
              "nothing was refused, so the gate was never exercised");

        Simulator::Destroy();
    }

    // The asymmetry the design exploits: 2.4 GHz and UWB are license-exempt with
    // no comparable cap, which is why routine gossip belongs there.
    StubPhy phy(StubPhy::FULL_MESH, 2, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac a;
    CaTblMacConfig ca;
    ca.nodeId = 1;
    ca.sf = 7;
    ca.bandwidthHz = 800e3;
    a.Configure(ca);
    a.SetRng(CreateObject<UniformRandomVariable>());
    channel.Register(&a);
    Check("no duty-cycle gate on tier 2 (license-exempt ISM)",
          !a.WouldViolateDutyCycle(CA_TBL_TIER_2P4, Seconds(100.0)));
    Check("no duty-cycle gate on UWB",
          !a.WouldViolateDutyCycle(CA_TBL_TIER_UWB, Seconds(100.0)));

    // Regression guard. The gate previously normalised trailing airtime by
    // ELAPSED time rather than by the observation window, which made the first
    // frame of any run look like ~100% duty and refused legitimate startup
    // traffic -- it is what broke the mobility handshake test. The cap is an
    // airtime budget per observation period (0.025 * 3600 = 90 s for a field
    // node), so a single frame at t=0 must be permitted and a frame larger than
    // the whole budget must not be.
    Check("startup frame is permitted (duty is a per-window budget, not "
          "instantaneous occupancy)",
          !a.WouldViolateDutyCycle(CA_TBL_TIER_868, Seconds(2.0)));
    Check("a frame exceeding the entire window budget is refused",
          a.WouldViolateDutyCycle(CA_TBL_TIER_868, Seconds(200.0)),
          "field-node budget is 0.025 * dutyWindowS = 90 s; a 200 s frame must not pass");

    Simulator::Destroy();
}

// ---------------------------------------------------------------------------
// Test 4 -- do priority contention windows actually separate access delay?
// ---------------------------------------------------------------------------

void
TestPriorityContention(uint32_t nContenders, const std::string& outdir)
{
    std::cout << "\n[4] Priority contention windows (design doc 2.3.1)\n";
    std::cout << "      NOTE: this is an empirical question, not a spec requirement.\n"
              << "      The numbers are reported; only the mechanism is asserted.\n";

    // Measured on backoff-per-deferral rather than end-to-end delivery delay.
    // Delivery delay would need packet_id correlation across the channel, and in
    // an all-destructive equal-power stub most frames never arrive at all -- so
    // the delay sample would be biased toward whichever class happened to win,
    // which is the very thing under test. Backoff per deferral is the direct,
    // unbiased measurement of what the contention window controls.
    StubPhy phy(StubPhy::FULL_MESH, nContenders + 1, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);

    std::vector<CaTblMac*> macs;
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(4242);

    for (uint32_t i = 0; i < nContenders; ++i)
    {
        CaTblMac* m = new CaTblMac();
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(1 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.maxCadRetries = 8;
        c.isAccessPoint = true; // remove the duty gate; this test is about contention
        m->Configure(c);
        m->SetRng(rng);
        channel.Register(m);
        macs.push_back(m);
    }

    // Alternate URGENT and LOW offers, all at the same instants, repeatedly. Under
    // the stub's identical-power channel every overlap is destructive, so the only
    // thing that can separate the two classes is the backoff draw.
    const uint32_t rounds = 40;
    for (uint32_t r = 0; r < rounds; ++r)
    {
        const double t = 1.0 + 0.5 * r;
        for (uint32_t i = 0; i < nContenders; ++i)
        {
            const CaTblMsgType type =
                (i % 2 == 0) ? CA_TBL_MSG_TRIGGERED_UPDATE : CA_TBL_MSG_DISCOVERY;
            Simulator::Schedule(Seconds(t),
                                &CaTblMac::Send,
                                macs[i],
                                type,
                                CA_TBL_TIER_868,
                                1u,
                                static_cast<uint16_t>(0));
        }
    }

    Simulator::Stop(Seconds(1.0 + 0.5 * rounds + 30.0));
    Simulator::Run();

    // Aggregate the per-class backoff the MAC recorded. This is the direct
    // measurement of the mechanism and does not depend on the delivery matching
    // above, which is why the assertion uses it.
    double urgentBackoff = 0.0;
    double lowBackoff = 0.0;
    uint64_t urgentSent = 0;
    uint64_t lowSent = 0;
    uint64_t urgentBusy = 0;
    uint64_t lowBusy = 0;
    for (size_t i = 0; i < macs.size(); ++i)
    {
        const CaTblMsgCounters& u = macs[i]->GetCounters(CA_TBL_MSG_TRIGGERED_UPDATE);
        const CaTblMsgCounters& l = macs[i]->GetCounters(CA_TBL_MSG_DISCOVERY);
        urgentBackoff += u.totalBackoffS;
        lowBackoff += l.totalBackoffS;
        urgentSent += u.sent;
        lowSent += l.sent;
        urgentBusy += u.cadBusyEvents;
        lowBusy += l.cadBusyEvents;
    }

    const double uPer = (urgentBusy > 0) ? urgentBackoff / static_cast<double>(urgentBusy) : 0.0;
    const double lPer = (lowBusy > 0) ? lowBackoff / static_cast<double>(lowBusy) : 0.0;

    Report("URGENT: sent / cad-busy / mean backoff per deferral",
           std::to_string(urgentSent) + " / " + std::to_string(urgentBusy) + " / " +
               std::to_string(uPer) + " s");
    Report("LOW:    sent / cad-busy / mean backoff per deferral",
           std::to_string(lowSent) + " / " + std::to_string(lowBusy) + " / " +
               std::to_string(lPer) + " s");
    Report("channel: tx / collisions / captures / deliveries",
           std::to_string(channel.GetTotalTransmissions()) + " / " +
               std::to_string(channel.GetTotalCollisions()) + " / " +
               std::to_string(channel.GetTotalCaptures()) + " / " +
               std::to_string(channel.GetTotalDeliveries()));

    // The mechanism assertion: URGENT must back off less per deferral than LOW.
    // If this fails the CW mapping is not wired to the access category at all,
    // which is a defect. Whether the difference is large enough to matter for
    // end-to-end latency is the empirical part, reported above.
    Check("URGENT backs off less per deferral than LOW",
          urgentBusy > 0 && lowBusy > 0 && uPer < lPer,
          "urgent " + std::to_string(uPer) + " vs low " + std::to_string(lPer));

    // Contention must actually have happened, or the test proved nothing.
    Check("contention occurred (CAD saw a busy channel)", urgentBusy + lowBusy > 0);
    Check("collisions are emergent, not assumed (at least one occurred)",
          channel.GetTotalCollisions() > 0,
          "no collision in a full-mesh equal-power channel means the overlap "
          "detection is not working");

    std::ofstream out((outdir + "/s4_contention.csv").c_str());
    out << "access_category,frames_sent,cad_busy_events,total_backoff_s,mean_backoff_per_deferral_s\n";
    out << "URGENT," << urgentSent << "," << urgentBusy << "," << urgentBackoff << "," << uPer
        << "\n";
    out << "LOW," << lowSent << "," << lowBusy << "," << lowBackoff << "," << lPer << "\n";
    out.close();
    Report("wrote", outdir + "/s4_contention.csv");

    for (size_t i = 0; i < macs.size(); ++i)
    {
        delete macs[i];
    }
    Simulator::Destroy();
}

// ---------------------------------------------------------------------------
// Test 5 -- flood dedup terminates, and hop_limit bounds the reach
// ---------------------------------------------------------------------------

void
TestFlood(uint32_t nNodes)
{
    std::cout << "\n[5] Flood dedup and hop limit (design doc 2.6)\n";

    StubPhy phy(StubPhy::LINE, nNodes, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);

    std::vector<CaTblMac*> macs;
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(99);

    const uint8_t hopLimit = 3;
    for (uint32_t i = 0; i < nNodes; ++i)
    {
        CaTblMac* m = new CaTblMac();
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(1 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.isAccessPoint = true;
        c.defaultHopLimit = hopLimit;
        c.maxCadRetries = 8;
        m->Configure(c);
        m->SetRng(rng);
        channel.Register(m);
        macs.push_back(m);
    }

    // One flood from the head of the line.
    Simulator::Schedule(Seconds(1.0),
                        &CaTblMac::Send,
                        macs[0],
                        CA_TBL_MSG_QUERY,
                        CA_TBL_TIER_868,
                        2u,
                        static_cast<uint16_t>(0));

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();

    uint64_t totalRx = 0;
    uint64_t totalRebroadcast = 0;
    uint64_t totalDedupDrops = 0;
    uint64_t totalHopDrops = 0;
    uint32_t reached = 0;
    for (size_t i = 0; i < macs.size(); ++i)
    {
        const CaTblMsgCounters& q = macs[i]->GetCounters(CA_TBL_MSG_QUERY);
        totalRx += q.received;
        totalRebroadcast += q.rebroadcast;
        totalDedupDrops += q.droppedDedup;
        totalHopDrops += q.droppedHopLimit;
        if (q.received > 0)
        {
            ++reached;
        }
    }

    Report("nodes reached", std::to_string(reached) + " of " + std::to_string(nNodes));
    Report("receptions / rebroadcasts", std::to_string(totalRx) + " / " +
                                            std::to_string(totalRebroadcast));
    Report("dedup drops / hop-limit drops",
           std::to_string(totalDedupDrops) + " / " + std::to_string(totalHopDrops));

    // Termination: the flood must stop. Without dedup a line topology
    // ping-pongs forever, so a finite rebroadcast count IS the property.
    Check("flood terminated (finite rebroadcasts)",
          totalRebroadcast < nNodes * 8,
          std::to_string(totalRebroadcast) + " rebroadcasts for " + std::to_string(nNodes) +
              " nodes suggests a broadcast storm");

    // Dedup must have actually fired -- in a line every interior node hears the
    // same frame from both neighbours.
    Check("dedup suppressed duplicate copies", totalDedupDrops > 0);

    // hop_limit must bound the reach. With hopLimit=3 from node 0, the frame can
    // travel at most 3 hops, so nodes beyond index 3 must not be reached.
    const uint32_t expectedMax = static_cast<uint32_t>(hopLimit) + 1;
    Check("hop limit bounds the reach",
          reached <= expectedMax,
          std::to_string(reached) + " nodes reached but hop_limit=" +
              std::to_string(static_cast<uint32_t>(hopLimit)) + " permits at most " +
              std::to_string(expectedMax));

    for (size_t i = 0; i < macs.size(); ++i)
    {
        delete macs[i];
    }
    Simulator::Destroy();
}

// ---------------------------------------------------------------------------
// Test 6 -- the 2.9 handshake sends different messages per mobility state
// ---------------------------------------------------------------------------

std::vector<CaTblMsgType> g_seen;

void
OnRxRecord(uint16_t rxId, const CaTblHeader& hdr, const CaTblLinkOutcome& out)
{
    (void)rxId;
    (void)out;
    g_seen.push_back(hdr.GetMsgType());
}

bool
SawType(CaTblMsgType t)
{
    for (size_t i = 0; i < g_seen.size(); ++i)
    {
        if (g_seen[i] == t)
        {
            return true;
        }
    }
    return false;
}

void
RunHandshake(CaTblMobilityState state)
{
    g_seen.clear();
    StubPhy phy(StubPhy::FULL_MESH, 2, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);

    CaTblMac a;
    CaTblMac b;
    CaTblMacConfig ca;
    ca.nodeId = 1;
    ca.sf = 7;
    ca.bandwidthHz = 125e3;
    ca.isAccessPoint = true;
    CaTblMacConfig cb = ca;
    cb.nodeId = 2;
    a.Configure(ca);
    b.Configure(cb);
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    a.SetRng(rng);
    b.SetRng(rng);
    b.SetRxCallback(MakeCallback(&OnRxRecord));
    channel.Register(&a);
    channel.Register(&b);

    a.SetMobilityState(state);
    Simulator::Schedule(Seconds(1.0), &CaTblMac::StartHandshake, &a, CA_TBL_TIER_868, uint16_t(2), 4u);
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    Simulator::Destroy();
}

void
TestHandshake()
{
    std::cout << "\n[6] Mobility-state handshake (design doc 2.9) -- this is claim 4\n";

    RunHandshake(CA_TBL_MOB_APPROACHING);
    const bool apprBeacon = SawType(CA_TBL_MSG_DISCOVERY);
    const bool apprDigest = SawType(CA_TBL_MSG_DIGEST);
    const bool apprTrig = SawType(CA_TBL_MSG_TRIGGERED_UPDATE);
    Report("approaching sent", std::string(apprBeacon ? "DISCOVERY " : "") +
                                   (apprDigest ? "DIGEST " : "") +
                                   (apprTrig ? "TRIGGERED_UPDATE" : ""));

    RunHandshake(CA_TBL_MOB_RECEDING);
    const bool recBeacon = SawType(CA_TBL_MSG_DISCOVERY);
    const bool recDigest = SawType(CA_TBL_MSG_DIGEST);
    const bool recTrig = SawType(CA_TBL_MSG_TRIGGERED_UPDATE);
    Report("receding sent", std::string(recBeacon ? "DISCOVERY " : "") +
                                (recDigest ? "DIGEST " : "") +
                                (recTrig ? "TRIGGERED_UPDATE" : ""));

    Check("approaching runs the full Beacon->Digest exchange", apprBeacon && apprDigest);
    Check("receding skips the Digest", !recDigest);
    Check("receding sends an urgent one-shot push instead", recTrig);
    Check("the two states are genuinely different message sequences",
          (apprDigest != recDigest) || (apprTrig != recTrig),
          "both mobility states produced the same messages -- the handshake is "
          "not conditioned on anything");
}

} // namespace

int
main(int argc, char* argv[])
{
    uint32_t nodes = 8;
    uint32_t contenders = 8;
    std::string outdir = "results/s4";

    CommandLine cmd;
    cmd.AddValue("nodes", "Nodes in the flood line topology", nodes);
    cmd.AddValue("contenders", "Nodes contending in the priority test", contenders);
    cmd.AddValue("outdir", "Output directory (must already exist)", outdir);
    cmd.Parse(argc, argv);

    std::cout << "CA-TBL S4 packet/MAC validator\n"
              << "  deterministic stub PHY -- isolates protocol defects from channel draws\n";

    TestToa(outdir);
    TestTaxonomy(outdir);
    TestDutyCycle();
    TestPriorityContention(contenders, outdir);
    TestFlood(nodes);
    TestHandshake();

    std::cout << "\n" << g_pass << " passed, " << g_fail << " failed\n";
    if (g_fail > 0)
    {
        std::cout << "\nFailures:\n";
        for (size_t i = 0; i < g_failures.size(); ++i)
        {
            std::cout << "  X " << g_failures[i] << "\n";
        }
        std::cout << "\nThe MAC is not trustworthy until these clear. Do not build the\n"
                  << "physics-backed scenario on top of a protocol that fails its own spec.\n";
        return 1;
    }

    std::cout << "\nAll MAC invariants hold. Next: wire CaTblPhyProvider to the three\n"
              << "physics engines so interference becomes emergent on the real channel.\n";
    return 0;
}
