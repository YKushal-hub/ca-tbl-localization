#include "ca-tbl-sparse-anchor.h"

#include <cmath>

namespace ns3
{

namespace
{

//! Mahalanobis distance of (x,y) from the prior, in the prior's own metric.
//! False if the covariance is singular or non-positive -- nothing can choose then.
bool
Mahalanobis(double x, double y, double px, double py,
            double pxx, double pxy, double pyy, double& out)
{
    const double det = pxx * pyy - pxy * pxy;
    if (!(det > 1e-12) || !(pxx > 0.0) || !(pyy > 0.0))
    {
        return false;
    }
    const double ix = pyy / det;
    const double ixy = -pxy / det;
    const double iy = pxx / det;
    const double dx = x - px;
    const double dy = y - py;
    const double q = dx * (ix * dx + ixy * dy) + dy * (ixy * dx + iy * dy);
    out = (q > 0.0) ? std::sqrt(q) : 0.0;
    return true;
}

CaTblSparseResult
TwoCircle(double x1, double y1, double d1,
          double x2, double y2, double d2,
          double priorX, double priorY,
          double pxx, double pxy, double pyy,
          double marginThreshold,
          CaTblSparseCase label)
{
    CaTblSparseResult r;
    r.solvedBy = CA_TBL_CASE_NONE;

    const double ex = x2 - x1;
    const double ey = y2 - y1;
    const double D = std::sqrt(ex * ex + ey * ey);

    // Coincident anchors carry one range between them, not two.
    if (!(D > 1e-6) || !(d1 >= 0.0) || !(d2 >= 0.0))
    {
        r.verdict = CA_TBL_SPARSE_DEGENERATE;
        return r;
    }

    // a is the distance along the baseline to the radical line.
    const double a = (d1 * d1 - d2 * d2 + D * D) / (2.0 * D);
    double h2 = d1 * d1 - a * a;

    // Circles that fall short of each other still carry direction. Pull them to the
    // radical line rather than refusing, but only when the shortfall is small against
    // the ranges themselves -- beyond that the two measurements disagree about the
    // world and averaging them would manufacture a position from a contradiction.
    if (h2 < 0.0)
    {
        const double shortfall = std::sqrt(-h2);
        const double scale = (d1 + d2) * 0.5;
        if (!(scale > 0.0) || shortfall > 0.5 * scale)
        {
            r.verdict = CA_TBL_SPARSE_DEGENERATE;
            return r;
        }
        h2 = 0.0;
    }
    const double h = std::sqrt(h2);

    const double mx = x1 + a * ex / D;
    const double my = y1 + a * ey / D;
    const double rx = -ey / D;
    const double ry = ex / D;

    const double c1x = mx + h * rx;
    const double c1y = my + h * ry;
    const double c2x = mx - h * rx;
    const double c2y = my - h * ry;

    double dm1 = 0.0;
    double dm2 = 0.0;
    if (!Mahalanobis(c1x, c1y, priorX, priorY, pxx, pxy, pyy, dm1) ||
        !Mahalanobis(c2x, c2y, priorX, priorY, pxx, pxy, pyy, dm2))
    {
        r.verdict = CA_TBL_SPARSE_NO_PRIOR;
        return r;
    }

    const bool firstWins = (dm1 <= dm2);
    r.dmChosen = firstWins ? dm1 : dm2;
    r.dmRejected = firstWins ? dm2 : dm1;

    // THE REFUSAL IS THE POINT. Two candidates the prior cannot separate is a coin
    // flip between positions that may be far apart, and a wrong one enters the filter
    // as evidence and is then defended by the covariance it shrank. Coasting knows it
    // knows nothing, which is strictly better.
    //
    // h == 0 is the tangent case: one point, nothing to disambiguate, always accepted.
    if (h > 0.0 && (r.dmRejected - r.dmChosen) < marginThreshold)
    {
        r.verdict = CA_TBL_SPARSE_AMBIGUOUS;
        return r;
    }

    r.x = firstWins ? c1x : c2x;
    r.y = firstWins ? c1y : c2y;
    r.solvedBy = label;
    r.verdict = CA_TBL_SPARSE_OK;
    return r;
}

} // namespace

CaTblSparseResult
CaTblSolveCaseA(double x1, double y1, double d1,
                double x2, double y2, double d2,
                double priorX, double priorY,
                double pxx, double pxy, double pyy,
                double marginThreshold)
{
    return TwoCircle(x1, y1, d1, x2, y2, d2, priorX, priorY, pxx, pxy, pyy,
                     marginThreshold, CA_TBL_CASE_A);
}

CaTblSparseResult
CaTblSolveCaseB(double anchorX, double anchorY, double dAnchor,
                double peerX, double peerY, double dPeer,
                double priorX, double priorY,
                double pxx, double pxy, double pyy,
                double marginThreshold)
{
    // Same geometry, different evidence. The label is what carries that distinction
    // upward: a fix that leaned on a peer's own estimate must not be reported as one
    // that leaned on two surveyed anchors.
    return TwoCircle(anchorX, anchorY, dAnchor, peerX, peerY, dPeer, priorX, priorY,
                     pxx, pxy, pyy, marginThreshold, CA_TBL_CASE_B);
}

CaTblSparseResult
CaTblSolveCaseC()
{
    // No surveyed anchor means no absolute frame. The peer set fixes a shape that can
    // be translated and rotated freely without contradicting any measurement, so there
    // is no position to return and never will be from this input.
    CaTblSparseResult r;
    r.solvedBy = CA_TBL_CASE_NONE;
    r.verdict = CA_TBL_SPARSE_DEGENERATE;
    return r;
}

} // namespace ns3
