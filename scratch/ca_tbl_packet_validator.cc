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

// ---------------------------------------------------------------------------
// Test 7 -- MC-1: ACK matched by packet id, and a denied request retried
// ---------------------------------------------------------------------------

int g_drops = 0;

void
OnDropCount(uint16_t nodeId, CaTblMsgType t, std::string why)
{
    (void)nodeId;
    (void)t;
    (void)why;
    ++g_drops;
}

void
RequestWithTerms(CaTblMac* m, uint8_t degradation, uint8_t battery)
{
    m->SetRequestTerms(degradation, battery);
    m->SetRequestTargetTier(CA_TBL_TIER_UWB);
    m->Send(CA_TBL_MSG_ESC_REQUEST, CA_TBL_TIER_868, 0, 0);
}

CaTblMacConfig
Mc1Config(uint16_t id, CaTblIdentityClass cls, bool fixes)
{
    CaTblMacConfig c;
    c.nodeId = id;
    c.identityClass = cls;
    c.sf = 7;
    c.bandwidthHz = 125e3;
    c.isAccessPoint = true;
    c.sendHandshakeFrames = true;
    c.uwbArbitration = true;
    c.handshakeFixes = fixes;
    return c;
}

struct AckByIdResult
{
    uint64_t retx;
    int drops;
    uint64_t matched;
    uint64_t unmatched;
};

// Node 1 sends a RESPONSE (an ACK'd frame) to node 2, which is silenced and so never ACKs,
// then one to node 3, which does. Node 3's ACK names the second frame.
AckByIdResult
RunAckById(bool fixes)
{
    g_drops = 0;
    StubPhy phy(StubPhy::FULL_MESH, 3, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac a, c, d;
    a.Configure(Mc1Config(1, CA_TBL_ID_TARGET, fixes));
    c.Configure(Mc1Config(2, CA_TBL_ID_TARGET, fixes));
    d.Configure(Mc1Config(3, CA_TBL_ID_TARGET, fixes));
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(7);
    a.SetRng(rng);
    c.SetRng(rng);
    d.SetRng(rng);
    a.SetDropCallback(MakeCallback(&OnDropCount));
    channel.Register(&a);
    channel.Register(&c);
    channel.Register(&d);
    c.SetSilent(true);
    Simulator::Schedule(Seconds(1.0), &CaTblMac::Send, &a, CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868,
                        1u, static_cast<uint16_t>(2));
    Simulator::Schedule(Seconds(1.3), &CaTblMac::Send, &a, CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868,
                        1u, static_cast<uint16_t>(3));
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    AckByIdResult r;
    r.retx = a.GetCounters(CA_TBL_MSG_RESPONSE).retransmissions;
    r.drops = g_drops;
    r.matched = a.GetHsAcksMatched();
    r.unmatched = a.GetHsAcksUnmatched();
    Simulator::Destroy();
    return r;
}

struct DenyResult
{
    uint64_t reqRetx;
    uint64_t arbDenials;
    uint64_t deniesSent;
    uint64_t deniesHeard;
    uint64_t grantsHeardByWinner;
    uint64_t winnerRetx;
    uint64_t skipped;
};

// Node 3 asks first with high terms and takes the slot (held 10 s); node 1 asks next with
// zero terms and loses to the holder.
DenyResult
RunDenyRetry(bool fixes)
{
    StubPhy phy(StubPhy::FULL_MESH, 3, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac a, b, e;
    a.Configure(Mc1Config(1, CA_TBL_ID_TARGET, fixes));
    b.Configure(Mc1Config(2, CA_TBL_ID_ANCHOR, fixes));
    e.Configure(Mc1Config(3, CA_TBL_ID_TARGET, fixes));
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(11);
    a.SetRng(rng);
    b.SetRng(rng);
    e.SetRng(rng);
    channel.Register(&a);
    channel.Register(&b);
    channel.Register(&e);
    Simulator::Schedule(Seconds(1.0), &RequestWithTerms, &e, uint8_t(255), uint8_t(255));
    Simulator::Schedule(Seconds(1.6), &RequestWithTerms, &a, uint8_t(0), uint8_t(0));
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    DenyResult r;
    r.reqRetx = a.GetCounters(CA_TBL_MSG_ESC_REQUEST).retransmissions;
    r.arbDenials = b.GetArbDenials();
    r.deniesSent = b.GetHsDeniesSent();
    r.deniesHeard = a.GetHsDeniesHeard();
    r.grantsHeardByWinner = e.GetHsGrantsHeard();
    r.winnerRetx = e.GetCounters(CA_TBL_MSG_ESC_REQUEST).retransmissions;
    r.skipped = b.GetHsRequestAcksSkipped() + a.GetHsRequestAcksSkipped() +
                e.GetHsRequestAcksSkipped();
    Simulator::Destroy();
    return r;
}

void
TestHandshakeFixes()
{
    std::cout << "\n[7] MC-1 handshake fixes (MAC sect 8: an ACK'd UWB request)\n";

    const AckByIdResult a0 = RunAckById(false);
    const AckByIdResult a1 = RunAckById(true);
    Report("ACK by id, default: retransmissions / drops",
           std::to_string(a0.retx) + " / " + std::to_string(a0.drops));
    Report("ACK by id, fixed: retransmissions / drops / matched / unmatched",
           std::to_string(a1.retx) + " / " + std::to_string(a1.drops) + " / " +
               std::to_string(a1.matched) + " / " + std::to_string(a1.unmatched));
    // Default: node 3's ACK clears the OLDEST pending frame -- the one node 2 never
    // acknowledged -- so that frame is never retried, and the acknowledged one is.
    Check("default: an ACK clears the oldest frame (the unACKed one is never retried)",
          a0.retx == 1 && a0.drops == 0,
          "retx " + std::to_string(a0.retx) + ", drops " + std::to_string(a0.drops));
    // Fixed: the ACK clears the frame it names; the unACKed one is retried to the limit
    // (maxRetransmissions = 2) and then dropped.
    Check("fixed: an ACK clears the frame it names; the unACKed one is retried and dropped",
          a1.retx == 2 && a1.drops == 1 && a1.matched == 1 && a1.unmatched == 0,
          "retx " + std::to_string(a1.retx) + ", drops " + std::to_string(a1.drops) +
              ", matched " + std::to_string(a1.matched));

    const DenyResult d0 = RunDenyRetry(false);
    const DenyResult d1 = RunDenyRetry(true);
    Report("deny, default: loser retransmissions / arbiter denials",
           std::to_string(d0.reqRetx) + " / " + std::to_string(d0.arbDenials));
    Report("deny, fixed: loser retransmissions / denials scored / sent / heard",
           std::to_string(d1.reqRetx) + " / " + std::to_string(d1.arbDenials) + " / " +
               std::to_string(d1.deniesSent) + " / " + std::to_string(d1.deniesHeard));
    // Default: the anchor ACKs before arbitrating, so the denied request is settled and
    // the requester never asks again.
    Check("default: a denied request is ACKed first and never retried",
          d0.arbDenials == 1 && d0.reqRetx == 0,
          "denials " + std::to_string(d0.arbDenials) + ", retx " + std::to_string(d0.reqRetx));
    // Fixed: each denial is answered and the request is retried on its ACK timeout; with
    // the slot held 10 s, the original and both retries are scored and denied.
    Check("fixed: a denial is answered, and the request is retried and re-scored",
          d1.reqRetx == 2 && d1.arbDenials == 3 && d1.deniesSent == 3 && d1.deniesHeard == 3,
          "retx " + std::to_string(d1.reqRetx) + ", denials " + std::to_string(d1.arbDenials) +
              ", sent " + std::to_string(d1.deniesSent) + ", heard " +
              std::to_string(d1.deniesHeard));
    Check("fixed: the winner's request is settled by its GRANT, not retried",
          d1.grantsHeardByWinner == 1 && d1.winnerRetx == 0,
          "grants heard " + std::to_string(d1.grantsHeardByWinner) + ", retx " +
              std::to_string(d1.winnerRetx));
    // 4 request transmissions (the winner's, and the loser's original and two retries),
    // each heard by the 2 other nodes.
    Check("fixed: no node answers a request with a bare ACK", d1.skipped == 8,
          "withheld " + std::to_string(d1.skipped) + " (need 4 transmissions x 2 hearers)");
}

// ---------------------------------------------------------------------------
// Test 8 -- MC-2: a last gasp leaves before the silence; a large correction repeats
// ---------------------------------------------------------------------------

void
GaspThenSilence(CaTblMac* m)
{
    m->SendTriggeredUpdate(CA_TBL_TIER_868, 2);
    m->SetSilent(true);
    // Anything after the silence must not leave.
    m->Send(CA_TBL_MSG_DIGEST, CA_TBL_TIER_868, 1, 0);
}

void
TestTriggeredUpdates()
{
    std::cout << "\n[8] MC-2 triggered updates (MAC sect 3 and 8)\n";

    uint64_t heard[3] = {0, 0, 0};
    uint64_t sentFrames[3] = {0, 0, 0};
    uint64_t suppressed = 0;
    uint64_t digestsHeard = 0;
    for (uint8_t reason = 1; reason <= 2; ++reason)
    {
        StubPhy phy(StubPhy::FULL_MESH, 2, 1);
        CaTblChannel channel;
        channel.SetPhyProvider(&phy);
        CaTblMac a, b;
        CaTblMacConfig ca;
        ca.nodeId = 1;
        ca.sf = 7;
        ca.bandwidthHz = 125e3;
        ca.isAccessPoint = true;
        ca.defaultHopLimit = 1; // no rebroadcast, so every reception is an original
        CaTblMacConfig cb = ca;
        cb.nodeId = 2;
        a.Configure(ca);
        b.Configure(cb);
        Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
        rng->SetStream(5);
        a.SetRng(rng);
        b.SetRng(rng);
        channel.Register(&a);
        channel.Register(&b);
        a.PublishSelfEntry(10.0, 20.0, 4.0, 4.0, 0.0, 0.0, 0.0, 0.5);
        if (reason == 2)
        {
            Simulator::Schedule(Seconds(1.0), &GaspThenSilence, &a);
        }
        else
        {
            Simulator::Schedule(Seconds(1.0), &CaTblMac::SendTriggeredUpdate, &a,
                                CA_TBL_TIER_868, uint8_t(1));
        }
        Simulator::Stop(Seconds(20.0));
        Simulator::Run();
        heard[reason] = b.GetTrigHeard(reason);
        sentFrames[reason] = a.GetTrigFramesSent();
        if (reason == 2)
        {
            suppressed = a.GetSuppressedTx();
            digestsHeard = b.GetCounters(CA_TBL_MSG_DIGEST).received;
        }
        Simulator::Destroy();
    }
    Report("large correction: frames sent / heard",
           std::to_string(sentFrames[1]) + " / " + std::to_string(heard[1]));
    Report("last gasp: frames sent / heard / later frames suppressed",
           std::to_string(sentFrames[2]) + " / " + std::to_string(heard[2]) + " / " +
               std::to_string(suppressed));
    // The redundant class: the first frame and two jittered repeats, all one packet id, so
    // the receiver's dedup keeps one.
    Check("a large correction goes with the redundant class's 3 frames",
          sentFrames[1] == 3 && heard[1] == 1,
          "sent " + std::to_string(sentFrames[1]) + ", heard " + std::to_string(heard[1]));
    // Queued before SetSilent(true), so it leaves; unrepeated; and what follows does not.
    Check("a last gasp leaves once, before the silence, and nothing after it",
          sentFrames[2] == 1 && heard[2] == 1 && suppressed == 1 && digestsHeard == 0,
          "sent " + std::to_string(sentFrames[2]) + ", heard " + std::to_string(heard[2]) +
              ", suppressed " + std::to_string(suppressed) + ", digests heard " +
              std::to_string(digestsHeard));
}

// [9] NW-8. A four-node line 1-2-3-4: node 1 (an anchor) asks for manifest 7, which
// only node 4 carries. The QUERY floods out over two relays; the RESPONSE must come back
// hop by hop, each node sending it to the previous hop IT cached.
struct RpRun
{
    uint64_t answered = 0;
    uint64_t hops[4] = {0, 0, 0, 0};
    uint64_t forwarded[4] = {0, 0, 0, 0};
    uint64_t floods[4] = {0, 0, 0, 0};
    uint64_t expired[4] = {0, 0, 0, 0};
    uint64_t delivered = 0;
    uint64_t respHeardAtRequester = 0;
    uint64_t duplicates = 0;
};

RpRun
RunReversePathLine(bool hop, double node3WindowS)
{
    RpRun r;
    StubPhy phy(StubPhy::LINE, 4, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac m[4];
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(9);
    for (int i = 0; i < 4; ++i)
    {
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(1 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.isAccessPoint = true; // 10 % duty: nothing here is throttled
        c.identityClass = (i == 0) ? CA_TBL_ID_ANCHOR : CA_TBL_ID_TARGET;
        c.targetManifestId = (i == 3) ? 7 : static_cast<uint8_t>(20 + i);
        c.reversePath = true;
        c.reversePathHop = hop;
        if (i == 2)
        {
            c.reversePathWindowS = node3WindowS;
        }
        m[i].Configure(c);
        m[i].SetRng(rng);
        channel.Register(&m[i]);
    }
    Simulator::Schedule(Seconds(1.0), &CaTblMac::SendQuery, &m[0], CA_TBL_TIER_868, uint8_t(7));
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    r.answered = m[3].GetQueriesAnswered();
    for (int i = 0; i < 4; ++i)
    {
        r.hops[i] = m[i].GetRpUnicastHops();
        r.forwarded[i] = m[i].GetRpForwarded();
        r.floods[i] = m[i].GetRpFloods();
        r.expired[i] = m[i].GetRpExpired();
    }
    r.delivered = m[0].GetRpDelivered();
    for (int i = 0; i < 4; ++i)
    {
        r.duplicates += m[i].GetRpDuplicates();
    }
    r.respHeardAtRequester = m[0].GetCounters(CA_TBL_MSG_RESPONSE).received;
    Simulator::Destroy();
    return r;
}

void
TestReversePathHop()
{
    std::cout << "\n[9] NW-8 reverse path, hop by hop (Transport sect 1)\n";
    const RpRun a = RunReversePathLine(true, 300.0);
    Report("chain: unicast hops at nodes 4/3/2, forwarded at 3/2, delivered at 1",
           std::to_string(a.hops[3]) + "/" + std::to_string(a.hops[2]) + "/" +
               std::to_string(a.hops[1]) + ", " + std::to_string(a.forwarded[2]) + "/" +
               std::to_string(a.forwarded[1]) + ", " + std::to_string(a.delivered));
    Check("the RESPONSE follows the reverse path: one unicast hop from each of 4, 3, 2",
          a.answered == 1 && a.hops[3] == 1 && a.hops[2] == 1 && a.hops[1] == 1 &&
              a.hops[0] == 0 && a.forwarded[2] == 1 && a.forwarded[1] == 1,
          "answered " + std::to_string(a.answered));
    // A lost ACK makes a hop retransmit under a fresh packet id, which dedup cannot
    // catch: those copies must be absorbed as duplicates, not forwarded again (the first
    // build forwarded every copy, 1 -> 3 -> 5).
    Report("frames heard at the requester / duplicates absorbed on the path",
           std::to_string(a.respHeardAtRequester) + " / " + std::to_string(a.duplicates));
    Check("it is delivered once, with no flood, whatever the retransmissions",
          a.delivered == 1 && a.respHeardAtRequester >= 1 &&
              a.floods[0] + a.floods[1] + a.floods[2] + a.floods[3] == 0,
          "delivered " + std::to_string(a.delivered) + ", heard " +
              std::to_string(a.respHeardAtRequester));
    // Node 3's entry is past its window when the answer comes back: the hop must flood,
    // and node 2 then carries the flood on rather than unicasting it.
    const RpRun b = RunReversePathLine(true, 0.0);
    Report("node 3 window 0: expired / floods at 3, forwarded at 2, delivered at 1",
           std::to_string(b.expired[2]) + " / " + std::to_string(b.floods[2]) + ", " +
               std::to_string(b.forwarded[1]) + ", " + std::to_string(b.delivered));
    Check("a missing (expired) entry floods, and the flood still reaches the requester",
          b.expired[2] == 1 && b.floods[2] == 1 && b.hops[2] == 0 && b.forwarded[1] == 0 &&
              b.delivered == 1,
          "expired " + std::to_string(b.expired[2]) + ", floods " + std::to_string(b.floods[2]));
    // The pre-NW-8 behaviour, kept under reversePath alone: the cache names the
    // originator, node 1, which is not node 4's neighbour -- the answer never arrives.
    const RpRun c = RunReversePathLine(false, 300.0);
    Check("without reversePathHop the one-hop answer to a 3-hop originator is lost",
          c.answered == 1 && c.respHeardAtRequester == 0 && c.delivered == 0,
          "heard " + std::to_string(c.respHeardAtRequester));
    // The route fields are billed under the flag and only there.
    CaTblMac on, off;
    CaTblMacConfig co;
    co.nodeId = 1;
    co.sf = 7;
    co.bandwidthHz = 125e3;
    co.reversePath = true;
    CaTblMacConfig cn = co;
    cn.reversePathHop = true;
    off.Configure(co);
    on.Configure(cn);
    const double tOff = off.FrameAirtimeS(CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868, 1);
    const double tOn = on.FrameAirtimeS(CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868, 1);
    const double qOff = off.FrameAirtimeS(CA_TBL_MSG_QUERY, CA_TBL_TIER_868, 1);
    const double qOn = on.FrameAirtimeS(CA_TBL_MSG_QUERY, CA_TBL_TIER_868, 1);
    Check("a RESPONSE's route fields are billed under reversePathHop, and nothing else is",
          tOn > tOff && qOn == qOff,
          "RESPONSE " + std::to_string(tOff) + " -> " + std::to_string(tOn) + " s");
}

// [10] NW-9. The same four-node line 1-2-3-4 under queryV2 (and reversePathHop, so an
// answer from node 4 can come home). Node 1 asks; nodes 2, 3, 4 carry manifests 8, 22, 7
// -- or, for the proxy case, 8, 7, 22, with node 3's entry pushed to node 2 first.
//
// The relay jitter is widened to 1.5 s because a node can put two frames on the air at
// once (TECH_DEBT N.6): node 2's answer and its relay of the rest of the list otherwise
// overlap and destroy each other at node 3. That defect is not what this case tests.
struct V2Run
{
    uint64_t answered[4] = {0, 0, 0, 0};
    uint64_t proxied[4] = {0, 0, 0, 0};
    uint64_t suppressed[4] = {0, 0, 0, 0};
    uint64_t reduced[4] = {0, 0, 0, 0};
    uint64_t queriesHeard[4] = {0, 0, 0, 0};
    uint64_t heard = 0;
    uint64_t withEntry = 0;
    std::vector<std::vector<int>> answers; // {manifest, answerer, entryNode, pcsQ, batteryQ}
};

V2Run
RunV2Line(const std::vector<uint8_t>& list, bool proxyAt2)
{
    V2Run r;
    StubPhy phy(StubPhy::LINE, 4, 1);
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac m[4];
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(10);
    const uint8_t midList[4] = {0, 8, 22, 7};
    const uint8_t midProxy[4] = {0, 8, 7, 22};
    const uint8_t* mid = proxyAt2 ? midProxy : midList;
    for (int i = 0; i < 4; ++i)
    {
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(1 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.isAccessPoint = true;
        c.identityClass = (i == 0) ? CA_TBL_ID_ANCHOR : CA_TBL_ID_TARGET;
        c.targetManifestId = mid[i];
        c.reversePath = true;
        c.reversePathHop = true;
        c.queryV2 = true;
        c.queryProxy = (i == 1) && proxyAt2;
        c.rebroadcastJitterMaxS = 1.5;
        m[i].Configure(c);
        m[i].SetRng(rng);
        channel.Register(&m[i]);
        if (i > 0)
        {
            m[i].SetSelfStatus(200, 0, 150, 10, false, 0);
            m[i].PublishSelfEntry(100.0 * i, 50.0 * i, 9.0, 9.0, 0.0, 0.0, 0.0, 0.5);
        }
    }
    m[0].SetResponseCallback([&r](const CaTblHeader& h) {
        r.answers.push_back({static_cast<int>(h.GetRespManifest()),
                             static_cast<int>(h.GetRespAnswererId()),
                             static_cast<int>(h.HasSelfEntry() ? h.GetSelfEntry().nodeId : 0),
                             static_cast<int>(h.GetSelfEntry().pcsQ),
                             static_cast<int>(h.GetSelfEntry().batteryQ)});
    });
    if (proxyAt2)
    {
        // Node 3's entry reaches its neighbour node 2 first, by its own update.
        Simulator::Schedule(Seconds(0.6), &CaTblMac::SendTriggeredUpdate, &m[2], CA_TBL_TIER_868,
                            uint8_t(1));
    }
    Simulator::Schedule(Seconds(5.0), &CaTblMac::SendQueryV2, &m[0], CA_TBL_TIER_868, list,
                        uint8_t(0));
    Simulator::Stop(Seconds(40.0));
    Simulator::Run();
    for (int i = 0; i < 4; ++i)
    {
        r.answered[i] = m[i].GetV2Answered();
        r.proxied[i] = m[i].GetV2Proxied();
        r.suppressed[i] = m[i].GetV2Suppressed();
        r.reduced[i] = m[i].GetV2Reduced();
        r.queriesHeard[i] = m[i].GetCounters(CA_TBL_MSG_QUERY).received;
    }
    {
        const CaTblMsgCounters& q = m[1].GetCounters(CA_TBL_MSG_QUERY);
            const CaTblMsgCounters& q3 = m[2].GetCounters(CA_TBL_MSG_QUERY);
        }
    {
        CaTblTableEntry e4;
        }
    r.heard = m[0].GetV2ResponsesHeard();
    r.withEntry = m[0].GetV2EntriesHeard();
    Simulator::Destroy();
    return r;
}

void
TestQueryV2()
{
    std::cout << "\n[10] NW-9 QUERY and RESPONSE carry what the spec says (Network sect 4)\n";
    const V2Run a = RunV2Line({7, 8, 9}, false);
    Report("list {7,8,9}: answered at 2/3/4, reduced at 2/3/4, answers heard / with entry",
           std::to_string(a.answered[1]) + "/" + std::to_string(a.answered[2]) + "/" +
               std::to_string(a.answered[3]) + ", " + std::to_string(a.reduced[1]) + "/" +
               std::to_string(a.reduced[2]) + "/" + std::to_string(a.reduced[3]) + ", " +
               std::to_string(a.heard) + " / " + std::to_string(a.withEntry));
    bool got7 = false, got8 = false;
    for (size_t k = 0; k < a.answers.size(); ++k)
    {
        got7 = got7 || (a.answers[k][0] == 7 && a.answers[k][1] == 4 && a.answers[k][2] == 4);
        got8 = got8 || (a.answers[k][0] == 8 && a.answers[k][1] == 2 && a.answers[k][2] == 2);
    }
    Check("every listed manifest a node carries is answered, by its target, with its entry",
          a.answered[1] == 1 && a.answered[3] == 1 && a.answered[2] == 0 && got7 && got8 &&
              a.heard == 2 && a.withEntry == 2,
          "answers heard " + std::to_string(a.heard));
    Check("a node that answered part of the list relays the rest (batch-safe), and one that "
          "answered nothing relays it whole",
          a.reduced[1] == 1 && a.reduced[2] == 0 && a.queriesHeard[3] >= 1 &&
              a.suppressed[1] == 0,
          "reduced at 2: " + std::to_string(a.reduced[1]));
    const V2Run b = RunV2Line({8}, false);
    Check("a node that answered every listed manifest does not rebroadcast",
          b.answered[1] == 1 && b.suppressed[1] == 1 && b.queriesHeard[2] == 0 && b.heard == 1,
          "suppressed " + std::to_string(b.suppressed[1]) + ", node 3 heard " +
              std::to_string(b.queriesHeard[2]));
    // The answer's header is billed under queryV2 (12 bytes over the 28-byte entry), alone
    // and with reversePathHop (then only the answerer's 2 more), and a QUERY is not.
    CaTblMacConfig cb;
    cb.nodeId = 1;
    cb.sf = 7;
    cb.bandwidthHz = 125e3;
    CaTblMacConfig cv = cb;
    cv.queryV2 = true;
    CaTblMacConfig cvh = cv;
    cvh.reversePathHop = true;
    CaTblMac mb, mv, mvh;
    mb.Configure(cb);
    mv.Configure(cv);
    mvh.Configure(cvh);
    const double rb = mb.FrameAirtimeS(CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868, 1);
    const double rv = mv.FrameAirtimeS(CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868, 1);
    const double rvh = mvh.FrameAirtimeS(CA_TBL_MSG_RESPONSE, CA_TBL_TIER_868, 1);
    Check("the v2 RESPONSE header is billed, with NW-8's fields not billed twice, and a QUERY "
          "is unchanged",
          rv > rb && rvh >= rv &&
              mv.FrameAirtimeS(CA_TBL_MSG_QUERY, CA_TBL_TIER_868, 3) ==
                  mb.FrameAirtimeS(CA_TBL_MSG_QUERY, CA_TBL_TIER_868, 3),
          "RESPONSE " + std::to_string(rb) + " -> " + std::to_string(rv) + " -> " +
              std::to_string(rvh) + " s");
    const V2Run c = RunV2Line({7}, true);
    bool proxy7 = false;
    for (size_t k = 0; k < c.answers.size(); ++k)
    {
        // ... with the status node 3 published (PCS 200, battery 150), carried unchanged.
        proxy7 = proxy7 || (c.answers[k][0] == 7 && c.answers[k][1] == 2 && c.answers[k][2] == 3 &&
                            c.answers[k][3] == 200 && c.answers[k][4] == 150);
    }
    Check("a proxy answers on behalf of the entry it holds (answerer 2, entry node 3), and "
          "having answered the whole list does not pass the query on to the target",
          c.proxied[1] == 1 && proxy7 && c.suppressed[1] == 1 && c.answered[2] == 0 &&
              c.queriesHeard[2] == 0,
          "proxied " + std::to_string(c.proxied[1]));
}

// [11] NW-5. One arbiter (an anchor, id 10) and three requesters (ids 11-13), all in
// range of each other. Requester 1 takes the slot; 2 and 3 queue; when the slot frees the
// queue's argmax is granted. Each requester's outcome is recorded by its grant callback.
struct ArbRun
{
    uint64_t aged = 0;
    uint64_t queued = 0;
    uint64_t outOfRange = 0;
    int outcome[4] = {9, 9, 9, 9}; // last callback per requester: 1 grant, 2 queued, 0 deny
    int queuedSeen[4] = {0, 0, 0, 0};
    int deniedSeen[4] = {0, 0, 0, 0};
};

ArbRun
RunArbiter(uint8_t deg2, uint8_t rem2, double t2, uint8_t deg3, uint8_t rem3, double t3,
           bool evidenceFor3)
{
    ArbRun r;
    StubPhy phy(StubPhy::FULL_MESH, 4, 10); // ids 10..13 (0 would read as broadcast)
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac m[4];
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(11);
    for (int i = 0; i < 4; ++i)
    {
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(10 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.isAccessPoint = true;
        c.identityClass = (i == 0) ? CA_TBL_ID_ANCHOR : CA_TBL_ID_TARGET;
        c.sendHandshakeFrames = true;
        c.handshakeFixes = true;
        c.uwbArbitration = true;
        c.arbitrationV2 = true;
        m[i].Configure(c);
        m[i].SetRng(rng);
        channel.Register(&m[i]);
        if (i > 0)
        {
            m[i].SetUwbGrantCallback([&r, i](uint16_t, int o) {
                r.outcome[i] = o;
                if (o == 2)
                {
                    r.queuedSeen[i] = 1;
                }
                if (o == 0)
                {
                    r.deniedSeen[i] = 1;
                }
            });
        }
    }
    m[0].NoteUwbEvidence(11, 0.0);
    m[0].NoteUwbEvidence(12, 0.0);
    if (evidenceFor3)
    {
        m[0].NoteUwbEvidence(13, 0.0);
    }
    // Evidence stays fresh for the run (it is renewed each second, as ranging would).
    for (int k = 1; k < 30; ++k)
    {
        Simulator::Schedule(Seconds(k), &CaTblMac::NoteUwbEvidence, &m[0], uint16_t(11),
                            double(k));
        Simulator::Schedule(Seconds(k), &CaTblMac::NoteUwbEvidence, &m[0], uint16_t(12),
                            double(k));
        if (evidenceFor3)
        {
            Simulator::Schedule(Seconds(k), &CaTblMac::NoteUwbEvidence, &m[0], uint16_t(13),
                                double(k));
        }
        Simulator::Schedule(Seconds(k + 0.5), &CaTblMac::ArbitrationCycle, &m[0]);
    }
    auto ask = [&](int i, uint8_t deg, uint8_t rem, double at) {
        Simulator::Schedule(Seconds(at), &CaTblMac::SetRequestTerms, &m[i], deg, rem);
        Simulator::Schedule(Seconds(at), &CaTblMac::SetRequestTargetTier, &m[i],
                            uint8_t(CA_TBL_TIER_UWB));
        Simulator::Schedule(Seconds(at), &CaTblMac::Send, &m[i], CA_TBL_MSG_ESC_REQUEST,
                            CA_TBL_TIER_868, uint32_t(0), uint16_t(0));
    };
    ask(1, 255, 255, 0.2);
    ask(2, deg2, rem2, t2);
    ask(3, deg3, rem3, t3);
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    r.aged = m[0].GetArbAgedGrants();
    r.queued = m[0].GetArbQueued();
    r.outOfRange = m[0].GetArbOutOfRange();
    Simulator::Destroy();
    return r;
}

void
TestArbitrationV2()
{
    std::cout << "\n[11] NW-5 arbitration as Network sect 5 specifies\n";
    // Aging: 2 asks early with the lower degradation, 3 asks just before the slot frees
    // with a higher one. Scored on the time waited, 2 has aged to the top; scored on time
    // since last grant (C4, never granted = 1 for both), 3 would win.
    const ArbRun a = RunArbiter(128, 255, 1.0, 153, 255, 9.8, true);
    Report("aging: outcomes 1/2/3, queued, granted from the queue",
           std::to_string(a.outcome[1]) + "/" + std::to_string(a.outcome[2]) + "/" +
               std::to_string(a.outcome[3]) + ", " + std::to_string(a.queued) + ", " +
               std::to_string(a.aged));
    Check("a denied request stays queued, told its place, and aging lifts the one that waited",
          a.queuedSeen[2] == 1 && a.queuedSeen[3] == 1 && a.outcome[2] == 1 && a.aged >= 1,
          "outcome of 2: " + std::to_string(a.outcome[2]));
    // Battery: same degradation, same moment; 3 has 10 % left, 2 has 90 %.
    const ArbRun b = RunArbiter(128, 230, 2.0, 128, 26, 2.0, true);
    Check("the inverse-remaining battery term orders two otherwise equal requests: the "
          "nearly flat one is granted first",
          b.outcome[3] == 1 && b.queuedSeen[2] == 1,
          "outcomes 2/3: " + std::to_string(b.outcome[2]) + "/" + std::to_string(b.outcome[3]));
    // Range: no evidence for 3, so it is refused, not queued. The refusal does not settle a
    // broadcast request (another anchor may be in reach): MC-1 retries it, each retry is
    // refused, and it ends when the retries run out (-1).
    const ArbRun c = RunArbiter(128, 255, 1.0, 200, 255, 2.0, false);
    Check("a request without the arbiter's own UWB-range evidence is refused, not queued, "
          "and the refusal does not settle it: it times out",
          c.outOfRange >= 2 && c.deniedSeen[3] == 0 && c.queuedSeen[3] == 0 && c.outcome[3] == -1,
          "out of range " + std::to_string(c.outOfRange));
}

// [12] NW-6. Two arbiters (anchors 20 and 21, each with its grant coordinator) and two
// requesters (22, 23), all in range. 22 asks first and both arbiters grant it (the double
// grant L.1.4 settles by gossip); 23 asks while both slots are held, and both queue it.
// The anchors then gossip. The lower id, 20, must give up its grant to 22 and its queued
// 23, with a final denial; 23 stays queued at 21 and is granted from 21's queue.
struct QgRun
{
    uint64_t yieldsLow = 0;
    uint64_t agedLow = 0;
    uint64_t agedHigh = 0;
    uint64_t heardLow = 0;
    int last23 = 9;
    uint16_t grantedBy23 = 0;
    int queued23 = 0;
};

QgRun
RunQueueGossip(bool gossipQueue)
{
    QgRun r;
    StubPhy phy(StubPhy::FULL_MESH, 4, 20); // ids 20..23
    CaTblChannel channel;
    channel.SetPhyProvider(&phy);
    CaTblMac m[4];
    CaTblGrantCoordinator c20(20), c21(21);
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(12);
    for (int i = 0; i < 4; ++i)
    {
        CaTblMacConfig c;
        c.nodeId = static_cast<uint16_t>(20 + i);
        c.sf = 7;
        c.bandwidthHz = 125e3;
        c.isAccessPoint = true;
        c.identityClass = (i < 2) ? CA_TBL_ID_ANCHOR : CA_TBL_ID_TARGET;
        c.sendHandshakeFrames = true;
        c.handshakeFixes = true;
        c.uwbArbitration = true;
        c.arbitrationV2 = true;
        c.gossipQueue = gossipQueue;
        m[i].Configure(c);
        m[i].SetRng(rng);
        channel.Register(&m[i]);
    }
    m[0].SetGrantCoordinator(&c20);
    m[1].SetGrantCoordinator(&c21);
    m[3].SetUwbGrantCallback([&r](uint16_t a, int o) {
        r.last23 = o;
        if (o == 2)
        {
            r.queued23 = 1;
        }
        if (o == 1)
        {
            r.grantedBy23 = a;
        }
    });
    for (int k = 0; k < 40; ++k)
    {
        for (int a = 0; a < 2; ++a)
        {
            Simulator::Schedule(Seconds(k), &CaTblMac::NoteUwbEvidence, &m[a], uint16_t(22),
                                double(k));
            Simulator::Schedule(Seconds(k), &CaTblMac::NoteUwbEvidence, &m[a], uint16_t(23),
                                double(k));
            Simulator::Schedule(Seconds(k + 0.5), &CaTblMac::ArbitrationCycle, &m[a]);
        }
    }
    auto ask = [&](int i, double at) {
        Simulator::Schedule(Seconds(at), &CaTblMac::SetRequestTerms, &m[i], uint8_t(128),
                            uint8_t(255));
        Simulator::Schedule(Seconds(at), &CaTblMac::SetRequestTargetTier, &m[i],
                            uint8_t(CA_TBL_TIER_UWB));
        Simulator::Schedule(Seconds(at), &CaTblMac::Send, &m[i], CA_TBL_MSG_ESC_REQUEST,
                            CA_TBL_TIER_868, uint32_t(0), uint16_t(0));
    };
    ask(2, 0.2);
    ask(3, 1.2);
    // Gossip from both, every 2 s, at offsets that do not collide.
    for (int k = 2; k < 40; k += 2)
    {
        Simulator::Schedule(Seconds(k + 0.1), &CaTblMac::SendGrantGossip, &m[0], CA_TBL_TIER_868);
        Simulator::Schedule(Seconds(k + 0.9), &CaTblMac::SendGrantGossip, &m[1], CA_TBL_TIER_868);
    }
    Simulator::Stop(Seconds(40.0));
    Simulator::Run();
    r.yieldsLow = m[0].GetQueueYields();
    r.agedLow = m[0].GetArbAgedGrants();
    r.agedHigh = m[1].GetArbAgedGrants();
    r.heardLow = m[0].GetQueueItemsHeard();
    Simulator::Destroy();
    return r;
}

void
TestQueueGossip()
{
    std::cout << "\n[12] NW-6 anchors gossip their queue and the lower id gives it up\n";
    const QgRun a = RunQueueGossip(true);
    Report("with the queue gossiped: yields at 20, grants from the queue at 20/21, 23's grantor",
           std::to_string(a.yieldsLow) + ", " + std::to_string(a.agedLow) + "/" +
               std::to_string(a.agedHigh) + ", " + std::to_string(a.grantedBy23));
    Check("the lower id hears the higher id's queue and gives the double-queued request up",
          a.heardLow >= 1 && a.yieldsLow >= 1 && a.agedLow == 0,
          "heard " + std::to_string(a.heardLow) + ", yields " + std::to_string(a.yieldsLow));
    Check("the request stays queued at the higher id and is granted from its queue",
          a.queued23 == 1 && a.last23 == 1 && a.grantedBy23 == 21 && a.agedHigh >= 1,
          "23 granted by " + std::to_string(a.grantedBy23));
    const QgRun b = RunQueueGossip(false);
    Check("CONTROL: with grants-only gossip nothing is yielded from a queue",
          b.yieldsLow == 0 && b.heardLow == 0,
          "yields " + std::to_string(b.yieldsLow));
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
    TestHandshakeFixes();
    TestTriggeredUpdates();
    TestReversePathHop();
    TestQueryV2();
    TestArbitrationV2();
    TestQueueGossip();

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
