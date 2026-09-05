/*
 * C5 / G15 — the relayed-range admission suite, with a negative fixture per condition.
 *
 * RULE 6: no gate is trusted without forced-failure evidence. A suite in which every
 * case passes proves that admission accepts things; it proves nothing about the five
 * conditions, which exist to REJECT. So each of A1–A5 gets a case built to violate that
 * condition and only that condition, and the suite fails if the verdict is anything
 * other than the specific rejection expected. A case that returned a DIFFERENT rejection
 * would also fail — otherwise one over-broad condition could mask the absence of the
 * other four.
 *
 * The positive control is what makes the negatives meaningful: it is the same
 * measurement with the violation removed, so each negative differs from an admitted
 * measurement in exactly one field.
 */
#include "ns3/ca-tbl-relay-admission.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>

using namespace ns3;

static int g_pass = 0;
static int g_fail = 0;

static void
Check(const std::string& name, CaTblRelayVerdict got, CaTblRelayVerdict want)
{
    if (got == want)
    {
        std::printf("  G15 ok    %-52s %s\n", name.c_str(), CaTblRelayVerdictName(got));
        ++g_pass;
    }
    else
    {
        std::printf("  G15 FAIL  %-52s got %s, expected %s\n",
                    name.c_str(),
                    CaTblRelayVerdictName(got),
                    CaTblRelayVerdictName(want));
        ++g_fail;
    }
}

static void
CheckClose(const std::string& name, double got, double want, double tol)
{
    if (std::fabs(got - want) <= tol)
    {
        std::printf("  G15 ok    %-52s %.9f\n", name.c_str(), got);
        ++g_pass;
    }
    else
    {
        std::printf("  G15 FAIL  %-52s got %.9f, expected %.9f\n", name.c_str(), got, want);
        ++g_fail;
    }
}

//! A measurement that satisfies every condition. Each fixture below breaks exactly one.
static CaTblRelayedRange
Valid()
{
    CaTblRelayedRange m;
    m.relayId = 2;
    m.tier = CA_TBL_TIER_UWB;
    m.hopDepth = 1;
    m.sigmaRangeM = 0.10; // UWB DS-TWR floor
    m.relayP[0][0] = 4.0;
    m.relayP[0][1] = 0.0;
    m.relayP[1][0] = 0.0;
    m.relayP[1][1] = 4.0;
    m.relayAgeS = 1.0;
    m.relayFixIntervalS = 10.0;
    m.ux = 1.0;
    m.uy = 0.0;
    m.distToRelayM = 100.0; // tr(P)=8 << 0.25*100^2=2500
    return m;
}

int
main(int, char**)
{
    std::printf("--- G15 relayed-range admission: A1-A5 with negative fixtures ---\n");
    const std::set<uint16_t> none;
    double s = 0.0;

    // 0. POSITIVE CONTROL. Without this the five rejections below could all be produced
    //    by a function that rejects everything.
    Check("positive control admits", CaTblAdmitRelayedRange(Valid(), none, &s),
          CA_TBL_RELAY_ADMIT);

    // The inflation is the whole point: sigma_eff^2 = sigma_range^2 + u^T P u.
    // u = (1,0), P[0][0] = 4 -> sqrt(0.01 + 4) = 2.002498439...
    CheckClose("sigma_eff = sqrt(sigma^2 + u^T P u)", s, std::sqrt(0.01 + 4.0), 1e-12);
    if (s <= 0.10)
    {
        std::printf("  G15 FAIL  inflated sigma is not larger than the direct sigma\n");
        ++g_fail;
    }
    else
    {
        std::printf("  G15 ok    %-52s %.6f > %.6f\n", "inflation actually inflates", s, 0.10);
        ++g_pass;
    }

    // A1 — tier 1. Correlated shadowing field, so independence is absent in the medium.
    {
        CaTblRelayedRange m = Valid();
        m.tier = CA_TBL_TIER_868;
        Check("A1 violated: tier 1 offered", CaTblAdmitRelayedRange(m, none, &s),
              CA_TBL_RELAY_REJECT_TIER);
    }

    // A2 — depth 2. P_B would understate its own error by the term this model adds.
    {
        CaTblRelayedRange m = Valid();
        m.hopDepth = 2;
        Check("A2 violated: hop depth 2", CaTblAdmitRelayedRange(m, none, &s),
              CA_TBL_RELAY_REJECT_DEPTH);
    }
    // Depth 0 is not a relayed measurement at all and must not be admitted here either.
    {
        CaTblRelayedRange m = Valid();
        m.hopDepth = 0;
        Check("A2 violated: hop depth 0 (direct, not relayed)",
              CaTblAdmitRelayedRange(m, none, &s), CA_TBL_RELAY_REJECT_DEPTH);
    }

    // A3 — relay close relative to its own uncertainty: the linearisation fails.
    {
        CaTblRelayedRange m = Valid();
        m.distToRelayM = 5.0; // 0.25*25 = 6.25, tr(P) = 8 >= 6.25
        Check("A3 violated: tr(P_B) >= 0.25*d^2", CaTblAdmitRelayedRange(m, none, &s),
              CA_TBL_RELAY_REJECT_GEOMETRY);
    }
    // Degenerate separation: u is undefined, so the condition cannot be evaluated.
    {
        CaTblRelayedRange m = Valid();
        m.distToRelayM = 0.0;
        Check("A3 violated: zero separation, direction undefined",
              CaTblAdmitRelayedRange(m, none, &s), CA_TBL_RELAY_REJECT_GEOMETRY);
    }
    // A P_B that is not a covariance must be refused, not clamped into looking precise.
    {
        CaTblRelayedRange m = Valid();
        m.relayP[0][0] = -9.0;
        m.relayP[1][1] = 10.0; // trace still small enough to pass A3's threshold
        Check("not a covariance: negative projection refused",
              CaTblAdmitRelayedRange(m, none, &s), CA_TBL_RELAY_REJECT_GEOMETRY);
    }

    // A4 — stale covariance describes a position the relay has left.
    {
        CaTblRelayedRange m = Valid();
        m.relayAgeS = 12.0;
        Check("A4 violated: P_B older than the fix interval",
              CaTblAdmitRelayedRange(m, none, &s), CA_TBL_RELAY_REJECT_STALE);
    }
    {
        CaTblRelayedRange m = Valid();
        m.relayAgeS = m.relayFixIntervalS; // exactly one interval old
        Check("A4 boundary: age == interval is stale",
              CaTblAdmitRelayedRange(m, none, &s), CA_TBL_RELAY_REJECT_STALE);
    }

    // A5 — the same relay twice in one update shares its position error.
    {
        CaTblRelayedRange m = Valid();
        std::set<uint16_t> used;
        used.insert(m.relayId);
        Check("A5 violated: relay already used this update",
              CaTblAdmitRelayedRange(m, used, &s), CA_TBL_RELAY_REJECT_DUPLICATE);
        // A DIFFERENT relay in the same update is fine -- A5 is per relay, not per update.
        CaTblRelayedRange other = Valid();
        other.relayId = 3;
        Check("A5 control: a different relay is still admitted",
              CaTblAdmitRelayedRange(other, used, &s), CA_TBL_RELAY_ADMIT);
    }

    // The projection must see off-diagonal terms, or a correlated P_B would be treated
    // as if it were diagonal -- silently under-inflating exactly when the relay's error
    // is directional.
    {
        double P[2][2] = {{4.0, 2.0}, {2.0, 4.0}};
        const double u = std::sqrt(0.5);
        // u^T P u with u=(0.707,0.707): 0.5*4 + 2*0.5*2 + 0.5*4 = 6
        CheckClose("projection includes the off-diagonal term",
                   CaTblProjectRelayCovariance(P, u, u), 6.0, 1e-12);
    }

    std::printf("  G15 admission suite: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
