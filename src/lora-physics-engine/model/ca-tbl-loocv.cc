#include "ca-tbl-loocv.h"

#include <cmath>

namespace ns3
{

namespace
{

//! Solve a 3x3 symmetric normal system by Gaussian elimination with partial pivoting.
//! False if singular, which is what a constant term produces.
bool
Solve3(double A[3][3], double b[3], double x[3])
{
    for (int c = 0; c < 3; ++c)
    {
        int piv = c;
        for (int r = c + 1; r < 3; ++r)
        {
            if (std::fabs(A[r][c]) > std::fabs(A[piv][c]))
            {
                piv = r;
            }
        }
        if (std::fabs(A[piv][c]) < 1e-12)
        {
            return false;
        }
        if (piv != c)
        {
            for (int k = 0; k < 3; ++k)
            {
                const double t = A[c][k];
                A[c][k] = A[piv][k];
                A[piv][k] = t;
            }
            const double tb = b[c];
            b[c] = b[piv];
            b[piv] = tb;
        }
        for (int r = c + 1; r < 3; ++r)
        {
            const double f = A[r][c] / A[c][c];
            for (int k = c; k < 3; ++k)
            {
                A[r][k] -= f * A[c][k];
            }
            b[r] -= f * b[c];
        }
    }
    for (int r = 2; r >= 0; --r)
    {
        double s = b[r];
        for (int k = r + 1; k < 3; ++k)
        {
            s -= A[r][k] * x[k];
        }
        x[r] = s / A[r][r];
    }
    return true;
}

//! Ordinary least squares of realisedError on the three terms, over a sample subset with
//! one index omitted. omit < 0 fits on everything.
bool
FitOls(const std::vector<CaTblCalSample>& s, int omit, double w[3])
{
    double A[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
    double b[3] = {0, 0, 0};
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (static_cast<int>(i) == omit)
        {
            continue;
        }
        const double f[3] = {s[i].term1, s[i].term2, s[i].term3};
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                A[r][c] += f[r] * f[c];
            }
            b[r] += f[r] * s[i].realisedErrorM;
        }
    }
    return Solve3(A, b, w);
}

double
Predict(const CaTblCalSample& x, const double w[3])
{
    return w[0] * x.term1 + w[1] * x.term2 + w[2] * x.term3;
}

} // namespace

CaTblCalResult
CaTblCalibrateLoocv(const std::vector<CaTblCalSample>& samples, bool requireImprovement)
{
    CaTblCalResult r;
    r.samplesUsed = static_cast<uint32_t>(samples.size());

    // Three free parameters need at least four samples for leave-one-out to leave
    // anything to fit on.
    if (samples.size() < 4)
    {
        r.verdict = CA_TBL_CAL_TOO_FEW_SAMPLES;
        return r;
    }

    // A term that never varies cannot have its weight identified: any value fits equally.
    // This is not hypothetical here -- hopWeight is 0 and the hop term is structurally
    // constant, so this branch is the expected outcome on real data today.
    for (int k = 0; k < 3; ++k)
    {
        double lo = 0.0, hi = 0.0;
        for (size_t i = 0; i < samples.size(); ++i)
        {
            const double v = (k == 0) ? samples[i].term1
                             : (k == 1) ? samples[i].term2
                                        : samples[i].term3;
            if (i == 0 || v < lo) { lo = v; }
            if (i == 0 || v > hi) { hi = v; }
        }
        if (hi - lo < 1e-12)
        {
            r.verdict = CA_TBL_CAL_DEGENERATE;
            return r;
        }
    }

    // LOOCV: fit on n-1, predict the omitted one, accumulate. Every sample validates
    // exactly once, which is what makes this usable on a set too small to split.
    double sse = 0.0;
    double sseControl = 0.0;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        double w[3];
        if (!FitOls(samples, static_cast<int>(i), w))
        {
            r.verdict = CA_TBL_CAL_DEGENERATE;
            return r;
        }
        const double e = samples[i].realisedErrorM - Predict(samples[i], w);
        sse += e * e;

        // THE CONTROL: term 1 alone, refitted the same way. Term 1 is a monotone
        // function of the covariance trace, so this is the quantity the score is
        // measured to be a re-encoding of. Fitting it identically is what makes the
        // comparison fair -- a control given fewer fitting opportunities would lose for
        // the wrong reason.
        //
        // WITH AN INTERCEPT (TECH_DEBT M.7). The three-term model has none, but term 3
        // (the LOS fraction, ~0.92 and nearly constant) gives it one for free. A control
        // fitted through the origin then loses for exactly the wrong reason, and every
        // CAL-01 cell read OK on that offset alone. c0 + c1 * term1 levels it.
        double n1 = 0.0, sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
        for (size_t j = 0; j < samples.size(); ++j)
        {
            if (j == i) { continue; }
            const double x = samples[j].term1;
            const double y = samples[j].realisedErrorM;
            n1 += 1.0;
            sx += x;
            sy += y;
            sxx += x * x;
            sxy += x * y;
        }
        const double det = n1 * sxx - sx * sx;
        const double c1 = (std::fabs(det) > 1e-12) ? (n1 * sxy - sx * sy) / det : 0.0;
        const double c0 = (n1 > 0.0) ? (sy - c1 * sx) / n1 : 0.0;
        const double ec = samples[i].realisedErrorM - (c0 + c1 * samples[i].term1);
        sseControl += ec * ec;
    }
    r.loocvRmse = std::sqrt(sse / static_cast<double>(samples.size()));
    r.controlRmse = std::sqrt(sseControl / static_cast<double>(samples.size()));

    double wf[3];
    if (!FitOls(samples, -1, wf))
    {
        r.verdict = CA_TBL_CAL_DEGENERATE;
        return r;
    }

    // Normalised to sum to 3, so the fitted weights sit on the same scale as the
    // hand-set ones and the thresholds those were chosen against. A silent rescale would
    // move every escalation threshold with it.
    const double sum = wf[0] + wf[1] + wf[2];
    if (std::fabs(sum) > 1e-12)
    {
        const double k = 3.0 / sum;
        r.w1 = wf[0] * k;
        r.w2 = wf[1] * k;
        r.w3 = wf[2] * k;
    }
    else
    {
        r.w1 = wf[0];
        r.w2 = wf[1];
        r.w3 = wf[2];
    }

    if (requireImprovement && !(r.loocvRmse < r.controlRmse))
    {
        // Not an error. It is the ANSWER, and it is the one K.8 says to expect: three
        // fitted weights that do not beat one term the score is computed from have not
        // earned two extra parameters.
        r.verdict = CA_TBL_CAL_NO_IMPROVEMENT;
        return r;
    }

    r.verdict = CA_TBL_CAL_OK;
    return r;
}

const char*
CaTblCalVerdictName(CaTblCalVerdict v)
{
    switch (v)
    {
    case CA_TBL_CAL_OK:               return "OK";
    case CA_TBL_CAL_TOO_FEW_SAMPLES:  return "TOO_FEW_SAMPLES";
    case CA_TBL_CAL_DEGENERATE:       return "DEGENERATE";
    default:                          return "NO_IMPROVEMENT";
    }
}

} // namespace ns3
