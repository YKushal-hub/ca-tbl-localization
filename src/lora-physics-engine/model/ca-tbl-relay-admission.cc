#include "ca-tbl-relay-admission.h"

#include <cmath>

namespace ns3
{

// See the header: this is OURS, chosen so the neglected second-order term stays below
// 10 % of sigma_eff^2. It is not a cited constant and must not be reported as one.
const double CA_TBL_RELAY_GEOMETRY_RATIO = 0.25;

CaTblRelayedRange::CaTblRelayedRange()
    :
      maxRelayDepth(1),
      relayVarBudgetM2(0.0), relayId(0),
      tier(CA_TBL_TIER_868),
      hopDepth(0),
      sigmaRangeM(0.0),
      relayAgeS(0.0),
      relayFixIntervalS(0.0),
      ux(0.0),
      uy(0.0),
      distToRelayM(0.0)
{
    relayP[0][0] = 0.0;
    relayP[0][1] = 0.0;
    relayP[1][0] = 0.0;
    relayP[1][1] = 0.0;
}

double
CaTblProjectRelayCovariance(const double P[2][2], double ux, double uy)
{
    // u^T P u for the symmetric 2x2 case. P[0][1] and P[1][0] are averaged rather than
    // one of them being trusted: a covariance assembled from a wire format can arrive
    // very slightly asymmetric, and silently taking [0][1] would hide that. Averaging
    // is the symmetric part, which is the only part a quadratic form can see anyway.
    const double pxy = 0.5 * (P[0][1] + P[1][0]);
    return ux * ux * P[0][0] + 2.0 * ux * uy * pxy + uy * uy * P[1][1];
}

CaTblRelayVerdict
CaTblAdmitRelayedRange(const CaTblRelayedRange& m,
                       const std::set<uint16_t>& relaysUsed,
                       double* sigmaEffMOut)
{
    // A1. Tier 1 is inadmissible because its error is drawn from a spatially correlated
    // shadowing field: two nearby tags' errors are correlated BEFORE any relaying
    // happens, so the independence the measurement equation assumes is absent in the
    // medium. That is a property of the channel, not a simplification for convenience.
    if (m.tier == CA_TBL_TIER_868)
    {
        return CA_TBL_RELAY_REJECT_TIER;
    }

    // A2. At depth 2 the relay's own position is itself relayed, so P_B understates its
    // error by exactly the term this model adds -- the correction would need the joint
    // state of relay and target, which is a different estimator.
    // A2, ADAPTIVE. The old rule rejected any depth but exactly 1, which made the
    // mesh single-hop by construction: a relay three kilometres out was invisible
    // even with another node standing between. What actually matters is not how
    // many hops a measurement crossed but how much uncertainty it accumulated
    // doing so, because that is what decides whether it is worth fusing.
    //
    // Depth 0 is still refused: that is a direct measurement, not a relayed one,
    // and admitting it here would double-count it.
    if (m.hopDepth < 1 || m.hopDepth > m.maxRelayDepth)
    {
        return CA_TBL_RELAY_REJECT_DEPTH;
    }
    if (m.relayVarBudgetM2 > 0.0)
    {
        const double carried = m.relayP[0][0] + m.relayP[1][1];
        if (carried > m.relayVarBudgetM2)
        {
            return CA_TBL_RELAY_REJECT_DEPTH;
        }
    }

    // A3. The linearisation fails when the relay is close relative to its own
    // uncertainty. Guard the degenerate distance explicitly: at zero separation the
    // direction u is undefined, so there is no projection to compute and the condition
    // cannot be evaluated rather than merely failing.
    const double trP = m.relayP[0][0] + m.relayP[1][1];
    if (m.distToRelayM <= 0.0 ||
        trP >= CA_TBL_RELAY_GEOMETRY_RATIO * m.distToRelayM * m.distToRelayM)
    {
        return CA_TBL_RELAY_REJECT_GEOMETRY;
    }

    // A4. A stale covariance describes a position the relay has since left. Equality is
    // rejected: a covariance exactly one fix interval old is already describing the
    // previous fix.
    if (m.relayAgeS >= m.relayFixIntervalS)
    {
        return CA_TBL_RELAY_REJECT_STALE;
    }

    // A5. Two measurements through the same relay share that relay's position error. A
    // filter treating them as independent shrinks its covariance twice on one piece of
    // information and converges to a BIASED estimate -- the failure is in the mean, not
    // only in the covariance, which is why this is an admission condition and not a
    // weighting. One per relay per update discards information rather than inventing
    // it, which is the conservative direction.
    if (relaysUsed.count(m.relayId) != 0)
    {
        return CA_TBL_RELAY_REJECT_DUPLICATE;
    }

    const double proj = CaTblProjectRelayCovariance(m.relayP, m.ux, m.uy);
    // A covariance is positive semi-definite, so the projection cannot be negative. If
    // it is, the matrix that arrived is not a covariance; clamping at zero would let a
    // corrupt P_B through as if it were merely precise, so the geometry verdict is
    // returned instead -- refusing the measurement rather than trusting it.
    if (proj < 0.0)
    {
        return CA_TBL_RELAY_REJECT_GEOMETRY;
    }

    if (sigmaEffMOut != nullptr)
    {
        *sigmaEffMOut = std::sqrt(m.sigmaRangeM * m.sigmaRangeM + proj);
    }
    return CA_TBL_RELAY_ADMIT;
}

const char*
CaTblRelayVerdictName(CaTblRelayVerdict v)
{
    switch (v)
    {
    case CA_TBL_RELAY_ADMIT:
        return "ADMIT";
    case CA_TBL_RELAY_REJECT_TIER:
        return "REJECT_TIER(A1)";
    case CA_TBL_RELAY_REJECT_DEPTH:
        return "REJECT_DEPTH(A2)";
    case CA_TBL_RELAY_REJECT_GEOMETRY:
        return "REJECT_GEOMETRY(A3)";
    case CA_TBL_RELAY_REJECT_STALE:
        return "REJECT_STALE(A4)";
    case CA_TBL_RELAY_REJECT_DUPLICATE:
        return "REJECT_DUPLICATE(A5)";
    default:
        return "UNKNOWN";
    }
}


bool
CaTblComposeRelayChain(const double relayP[2][2],
                       double inheritedVarM2,
                       double ux,
                       double uy,
                       double outP[2][2])
{
    // Positive-definite check on the 2x2. A relay reporting a degenerate covariance is
    // reporting that it knows its position exactly along some axis, which no relayed
    // fix ever does.
    const double det = relayP[0][0] * relayP[1][1] - relayP[0][1] * relayP[1][0];
    if (!(relayP[0][0] > 0.0) || !(relayP[1][1] > 0.0) || !(det > 0.0))
    {
        return false;
    }

    outP[0][0] = relayP[0][0];
    outP[0][1] = relayP[0][1];
    outP[1][0] = relayP[1][0];
    outP[1][1] = relayP[1][1];

    if (!(inheritedVarM2 > 0.0))
    {
        // Depth 1: the relay measured against surveyed anchors and inherited nothing.
        // P_B is already the whole story, and adding a term here would double-count.
        return true;
    }

    const double n = std::sqrt(ux * ux + uy * uy);
    if (!(n > 1e-9))
    {
        // No direction to inflate along. Fall back to isotropic rather than refusing:
        // the inherited uncertainty is real and dropping it silently is the failure
        // this function exists to prevent. Conservative in the wrong place beats
        // absent in the right one.
        outP[0][0] += inheritedVarM2;
        outP[1][1] += inheritedVarM2;
        return true;
    }

    // Rank-1 update along the inherited direction: P += var * u u^T.
    const double x = ux / n;
    const double y = uy / n;
    outP[0][0] += inheritedVarM2 * x * x;
    outP[0][1] += inheritedVarM2 * x * y;
    outP[1][0] += inheritedVarM2 * x * y;
    outP[1][1] += inheritedVarM2 * y * y;
    return true;
}

} // namespace ns3
