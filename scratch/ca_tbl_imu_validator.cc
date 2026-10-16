/**
 * \file ca_tbl_imu_validator.cc
 * \brief Monte-Carlo harness for CaTblImuSensorModel. Produces SAMPLE statistics only.
 *
 * What a pass means is decided elsewhere, by scripts/imu_closed_form_check.py, which
 * computes the expected values from the printed parameters in closed form. The split
 * is deliberate: this program and the model share a compiler, a units convention and
 * an author, so a check written here would share their mistakes. The six-decibel
 * sensitivity defect passed every check in this project that was written beside the
 * code it checked; it was found by an independent model refusing to reconcile.
 *
 * Each case enables ONE error term and zeroes the rest, so a defect in a term shows
 * up in that term's case and is attributable. The "all" case enables every term at
 * once, which is what catches terms that are each right alone but share a draw.
 *
 * --fixture breaks the MODEL while the PRINTED parameters stay nominal -- exactly the
 * failure mode where an implementation silently disagrees with its own spec. Every
 * fixture must make the checker fail, on the case that names the broken term. That is
 * asserted by scripts/catbl_imu_fixture.sh, not assumed.
 */

#include "ns3/ca-tbl-imu-ekf.h"
#include "ns3/ca-tbl-imu-sensor-model.h"
#include "ns3/command-line.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace ns3;

namespace
{

CaTblImuSensorParams
Zeroed()
{
    CaTblImuSensorParams p;
    p.gyroNoiseDpsRtHz = 0.0;
    p.gyroBiasInitDps = 0.0;
    p.gyroBiasGmDps = 0.0;
    p.gyroScaleErr = 0.0;
    p.headingInitDeg = 0.0;
    p.strideScaleErr = 0.0;
    p.strideStepErr = 0.0;
    // gyroBiasTauS, strideLengthM and currentMa stay at their defaults: they are
    // shape parameters, not error magnitudes, and zeroing them changes the model.
    return p;
}

// Applies a fixture to the parameters the MODEL runs with. The printed parameters are
// never touched, so the checker's expectation stays nominal.
CaTblImuSensorParams
Broken(CaTblImuSensorParams p, const std::string& fixture)
{
    if (fixture == "arw_var_x2")
    {
        p.gyroNoiseDpsRtHz *= std::sqrt(2.0); // white rate noise variance doubled
    }
    else if (fixture == "bias_drop")
    {
        p.gyroBiasInitDps = 0.0; // the Python IMU's error: no bias term at all
    }
    else if (fixture == "bias_x2")
    {
        p.gyroBiasInitDps *= 2.0; // the Python IMU's other error: gyro term 2x too large
    }
    else if (fixture == "gm_white")
    {
        p.gyroBiasTauS = 1e-3; // Gauss-Markov collapsed to white: correlation lost
    }
    else if (fixture == "align_drop")
    {
        p.headingInitDeg = 0.0;
    }
    else if (fixture == "stride_scale_drop")
    {
        p.strideScaleErr = 0.0;
    }
    else if (fixture == "stride_step_x2var")
    {
        p.strideStepErr *= std::sqrt(2.0);
    }
    else if (fixture == "gscale_drop")
    {
        p.gyroScaleErr = 0.0;
    }
    return p;
}

void
PrintParams(const std::string& name, const CaTblImuSensorParams& p)
{
    std::printf("param,%s,gyroNoiseDpsRtHz,%.17g\n", name.c_str(), p.gyroNoiseDpsRtHz);
    std::printf("param,%s,gyroBiasInitDps,%.17g\n", name.c_str(), p.gyroBiasInitDps);
    std::printf("param,%s,gyroBiasGmDps,%.17g\n", name.c_str(), p.gyroBiasGmDps);
    std::printf("param,%s,gyroBiasTauS,%.17g\n", name.c_str(), p.gyroBiasTauS);
    std::printf("param,%s,gyroScaleErr,%.17g\n", name.c_str(), p.gyroScaleErr);
    std::printf("param,%s,headingInitDeg,%.17g\n", name.c_str(), p.headingInitDeg);
    std::printf("param,%s,strideLengthM,%.17g\n", name.c_str(), p.strideLengthM);
    std::printf("param,%s,strideScaleErr,%.17g\n", name.c_str(), p.strideScaleErr);
    std::printf("param,%s,strideStepErr,%.17g\n", name.c_str(), p.strideStepErr);
}

struct Acc
{
    double s1 = 0.0;
    double s2 = 0.0;
    uint64_t n = 0;

    void Add(double v)
    {
        s1 += v;
        s2 += v * v;
        n++;
    }

    double Mean() const { return s1 / static_cast<double>(n); }

    double Var() const
    {
        const double m = Mean();
        return (s2 - static_cast<double>(n) * m * m) / static_cast<double>(n - 1);
    }
};

// One case: M realisations of an n-cycle walk, dead-reckoned from the raw IMU output
// with no filter. Straight along +x, or turning at a constant rate.
void
RunCase(const std::string& name,
        const CaTblImuSensorParams& printed,
        const CaTblImuSensorParams& model,
        uint32_t mc,
        uint32_t n,
        double dt,
        double speed,
        double turnRateRadS,
        uint64_t seedBase)
{
    PrintParams(name, printed);
    std::printf("case,%s,n,%u,dt,%.17g,speed,%.17g,turnRate,%.17g,mc,%u\n",
                name.c_str(),
                n,
                dt,
                speed,
                turnRateRadS,
                mc);
    Acc ey;
    Acc ex;
    Acc th;
    for (uint32_t r = 0; r < mc; ++r)
    {
        CaTblImuSensorModel imu;
        imu.Configure(model, seedBase + r);
        double xT = 0.0;
        double yT = 0.0;
        double xH = 0.0;
        double yH = 0.0;
        for (uint32_t k = 1; k <= n; ++k)
        {
            const double h = turnRateRadS * static_cast<double>(k) * dt;
            const double dx = speed * dt * std::cos(h);
            const double dy = speed * dt * std::sin(h);
            xT += dx;
            yT += dy;
            const CaTblImuSensorIncrement inc = imu.Step(dx, dy, dt);
            xH += inc.distM * std::cos(inc.rawHeadingRad);
            yH += inc.distM * std::sin(inc.rawHeadingRad);
        }
        // Errors in the path frame. For the straight path that is along/cross track;
        // for the turning path only the heading error is scored.
        ex.Add(xH - xT);
        ey.Add(yH - yT);
        th.Add(imu.TrueHeadingErrRad());
    }
    std::printf("stat,%s,ex,%.17g,%.17g\n", name.c_str(), ex.Mean(), ex.Var());
    std::printf("stat,%s,ey,%.17g,%.17g\n", name.c_str(), ey.Mean(), ey.Var());
    std::printf("stat,%s,theta,%.17g,%.17g\n", name.c_str(), th.Mean(), th.Var());
}

} // namespace

// ---- Filter consistency --------------------------------------------------------
//
// The sensor cases above test what the IMU MEASURES. This tests what the filter
// BELIEVES about it: CaTblImuEkf run against the sensor, with ranging in alternating
// blocks of fixes and outages, scoring the position NEES e^T P^-1 e. A filter whose
// covariance is honest gives chi-square with 2 DOF, mean 2.0, in BOTH blocks. An
// overconfident one -- the Python IMU's defect, which took the realised error as its
// variance -- reads far above 2 in outages, which is where the integrity check meets
// the first fix after a gap.
//
// The band [1.6, 2.6] is fixed here, before any run, as +/-~25 % about 2.0: an EKF
// linearises a heading that reaches tens of degrees, so exact consistency is not
// expected, and a band tighter than the linearisation error would gate on it.

namespace
{
struct PathRng
{
    uint64_t s;

    double U()
    {
        uint64_t z = (s += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        z ^= z >> 31;
        return (static_cast<double>(z >> 11) + 0.5) * (1.0 / 9007199254740992.0);
    }

    double N()
    {
        const double u1 = U();
        const double u2 = U();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    }
};
} // namespace

static void
RunFilterTest(const std::string& fixture, uint32_t mc, uint32_t n, double dt, double speed)
{
    const CaTblImuSensorParams truth;
    CaTblImuSensorParams assumed;
    const bool dropStride = (fixture == "filter_no_stride_state");
    const bool zeroQ = (fixture == "filter_q_zero");
    const double sigR = 2.0;
    const uint32_t block = 20;
    const uint32_t burn = 40;
    const double ax[4] = {-300.0, 300.0, 300.0, -300.0};
    const double ay[4] = {-300.0, -300.0, 300.0, 300.0};
    const double deg = M_PI / 180.0;

    PrintParams("filter", truth);
    std::printf("case,filter,n,%u,dt,%.17g,speed,%.17g,turnRate,0,mc,%u,sigR,%.17g,block,%u,"
                "burn,%u\n",
                n,
                dt,
                speed,
                mc,
                sigR,
                block,
                burn);
    double neesFix = 0.0;
    double neesOut = 0.0;
    uint64_t nFix = 0;
    uint64_t nOut = 0;
    for (uint32_t r = 0; r < mc; ++r)
    {
        PathRng rng{0xC0FFEEULL + 7919ULL * r};
        CaTblImuSensorModel imu;
        imu.Configure(truth, 4242 + r);
        double x = 0.0;
        double y = 0.0;
        double h = 2.0 * M_PI * rng.U();

        CaTblImuEkf f;
        const double b0 = assumed.gyroBiasInitDps * deg;
        const double bg = assumed.gyroBiasGmDps * deg;
        const double th = assumed.headingInitDeg * deg;
        // Initial position drawn from the filter's own prior, so the start is
        // consistent by construction rather than flattered by starting on the truth.
        f.Init(x + rng.N(),
               y + rng.N(),
               1.0,
               th * th,
               b0 * b0 + bg * bg,
               dropStride ? 0.0 : assumed.strideScaleErr * assumed.strideScaleErr);

        double prevRaw = 0.0;
        bool haveRaw = false;
        for (uint32_t k = 1; k <= n; ++k)
        {
            // A wandering walk that turns back toward the middle of the anchor box.
            if (std::hypot(x, y) > 220.0)
            {
                h = std::atan2(-y, -x) + 0.3 * rng.N();
            }
            else
            {
                h += 0.15 * rng.N();
            }
            const double dx = speed * dt * std::cos(h);
            const double dy = speed * dt * std::sin(h);
            x += dx;
            y += dy;
            const CaTblImuSensorIncrement inc = imu.Step(dx, dy, dt);
            const double turn = haveRaw ? CaTblImuSensorModel::Wrap(inc.rawHeadingRad - prevRaw) : 0.0;
            prevRaw = inc.rawHeadingRad;
            haveRaw = true;

            double qStep2 = 0.0;
            double qTheta2 = 0.0;
            double qBias2 = 0.0;
            CaTblImuEkf::ProcessNoise(assumed, dt, inc.distM, turn, qStep2, qTheta2, qBias2);
            if (zeroQ)
            {
                qStep2 = qTheta2 = qBias2 = 0.0;
            }
            f.Predict(dt, inc.distM, inc.rawHeadingRad, qStep2, qTheta2, qBias2);

            const bool fix = ((k - 1) / block) % 2 == 0;
            if (fix)
            {
                std::vector<CaTblRangeMeasurement> meas;
                std::vector<double> var;
                for (int a = 0; a < 4; ++a)
                {
                    CaTblRangeMeasurement m;
                    m.anchorX = ax[a];
                    m.anchorY = ay[a];
                    m.rangeM = std::hypot(x - ax[a], y - ay[a]) + sigR * rng.N();
                    meas.push_back(m);
                    var.push_back(sigR * sigR);
                }
                f.Update(meas, var);
            }
            if (k <= burn)
            {
                continue;
            }
            double pxx = 0.0;
            double pxy = 0.0;
            double pyy = 0.0;
            f.PosCov(pxx, pxy, pyy);
            const double ex = f.X() - x;
            const double ey = f.Y() - y;
            const double det = pxx * pyy - pxy * pxy;
            const double nees = (pyy * ex * ex - 2.0 * pxy * ex * ey + pxx * ey * ey) / det;
            if (fix)
            {
                neesFix += nees;
                ++nFix;
            }
            else
            {
                neesOut += nees;
                ++nOut;
            }
        }
    }
    std::printf("filt,filter,fix,%.17g,%llu\n", neesFix / nFix, static_cast<unsigned long long>(nFix));
    std::printf("filt,filter,outage,%.17g,%llu\n",
                neesOut / nOut,
                static_cast<unsigned long long>(nOut));
}

// ---- Diagnostic (not a gate): the site-dwell regime ------------------------------
// static_uwb gets every fix while milling about inside a 10 m ring: 1.1 m steps, heading
// noise up to +/-3 rad per cycle, a projection back onto the ring with a pi flip, and
// 0.1 m UWB ranging. G91's filter test never exercised that regime. This replicates it
// with ranging that matches the filter's R exactly, so any inconsistency it shows belongs
// to the filter or the sensor model, not to the scenario's measurement errors.
static bool g_dwellUkf = false;
static bool g_dwellMM = false;
static bool g_dwellMMNoShift = false;

static void
RunDwellDiagnostic(uint32_t mc, double dt, uint32_t nDwell, uint32_t nTransit)
{
    const CaTblImuSensorParams truth;
    const CaTblImuSensorParams assumed;
    const double deg = M_PI / 180.0;
    const double ring = 10.0;
    const double sigR = 0.1;
    double zTh = 0.0, zB = 0.0, zS = 0.0, zTh2 = 0.0, zB2 = 0.0, zS2 = 0.0;
    double neesDwell = 0.0, neesEnd = 0.0, errEnd = 0.0;
    double eTh2 = 0.0, eB2 = 0.0, eS2 = 0.0, thErrDeg2 = 0.0, thSdDeg2 = 0.0;
    uint64_t nNd = 0;
    for (uint32_t r = 0; r < mc; ++r)
    {
        PathRng rng{0xD5E11ULL + 104729ULL * r};
        CaTblImuSensorModel imu;
        imu.Configure(truth, 9000 + r);
        double x = 0.0, y = 0.0, h = 2.0 * M_PI * rng.U();
        CaTblImuEkf f;
        f.SetMomentMatched(g_dwellMM);
        f.SetMomentMatchedMeanShift(!g_dwellMMNoShift);
        const double b0 = assumed.gyroBiasInitDps * deg, bg = assumed.gyroBiasGmDps * deg;
        const double th = assumed.headingInitDeg * deg;
        f.Init(x + 0.1 * rng.N(), y + 0.1 * rng.N(), 0.01, th * th, b0 * b0 + bg * bg,
               assumed.strideScaleErr * assumed.strideScaleErr);
        double prevRaw = 0.0;
        bool haveRaw = false;
        for (uint32_t k = 1; k <= nDwell + nTransit; ++k)
        {
            const double px = x, py = y;
            if (k <= nDwell)
            {
                h += (2.0 * rng.U() - 1.0) * 1.2 * dt;
                x += 0.45 * std::cos(h) * dt;
                y += 0.45 * std::sin(h) * dt;
                const double dr = std::hypot(x, y);
                if (dr > ring)
                {
                    x *= ring / dr;
                    y *= ring / dr;
                    h += M_PI;
                }
            }
            else
            {
                h += (2.0 * rng.U() - 1.0) * 0.10 * dt;
                x += 1.5 * std::cos(h) * dt;
                y += 1.5 * std::sin(h) * dt;
            }
            const CaTblImuSensorIncrement inc = imu.Step(x - px, y - py, dt);
            const double turn = haveRaw ? CaTblImuSensorModel::Wrap(inc.rawHeadingRad - prevRaw) : 0.0;
            prevRaw = inc.rawHeadingRad;
            haveRaw = true;
            double qs = 0.0, qt = 0.0, qb = 0.0;
            CaTblImuEkf::ProcessNoise(assumed, dt, inc.distM, turn, qs, qt, qb);
            if (g_dwellUkf)
            {
                f.PredictUnscented(dt, inc.distM, inc.rawHeadingRad, qs, qt, qb);
            }
            else
            {
                f.Predict(dt, inc.distM, inc.rawHeadingRad, qs, qt, qb);
            }
            if (k <= nDwell)
            {
                std::vector<CaTblRangeMeasurement> meas;
                std::vector<double> var;
                for (int a = 0; a < 5; ++a)
                {
                    CaTblRangeMeasurement m;
                    m.anchorX = ring * std::cos(2.0 * M_PI * a / 5.0);
                    m.anchorY = ring * std::sin(2.0 * M_PI * a / 5.0);
                    m.rangeM = std::hypot(x - m.anchorX, y - m.anchorY) + sigR * rng.N();
                    meas.push_back(m);
                    var.push_back(sigR * sigR);
                }
                f.Update(meas, var);
                double pxx, pxy, pyy;
                f.PosCov(pxx, pxy, pyy);
                const double ex = f.X() - x, ey = f.Y() - y, det = pxx * pyy - pxy * pxy;
                neesDwell += (pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det;
                ++nNd;
            }
            if (k == nDwell)
            {
                const double a = (CaTblImuSensorModel::Wrap(f.HeadingOffset() - imu.TrueHeadingErrRad())) / std::sqrt(f.Var(2));
                const double b = (f.GyroBias() - imu.TrueBiasRadS()) / std::sqrt(f.Var(3));
                const double c = (f.StrideScale() - imu.TrueStrideScale()) / std::sqrt(f.Var(4));
                zTh += a; zB += b; zS += c; zTh2 += a * a; zB2 += b * b; zS2 += c * c;
            }
        }
        double pxx, pxy, pyy;
        f.PosCov(pxx, pxy, pyy);
        const double ex = f.X() - x, ey = f.Y() - y, det = pxx * pyy - pxy * pxy;
        neesEnd += (pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det;
        errEnd += std::hypot(ex, ey);
        // State z-scores at the END of the outage: which state's belief has drifted from
        // its own variance.
        const double ta = CaTblImuSensorModel::Wrap(f.HeadingOffset() - imu.TrueHeadingErrRad());
        eTh2 += ta * ta / f.Var(2);
        eB2 += std::pow(f.GyroBias() - imu.TrueBiasRadS(), 2) / f.Var(3);
        eS2 += std::pow(f.StrideScale() - imu.TrueStrideScale(), 2) / f.Var(4);
        thErrDeg2 += std::pow(ta / deg, 2);
        thSdDeg2 += f.Var(2) / (deg * deg);
    }
    std::printf("dwelldiag,end_z_theta_rms,%.4f,end_z_bias_rms,%.4f,end_z_stride_rms,%.4f\n",
                std::sqrt(eTh2 / mc), std::sqrt(eB2 / mc), std::sqrt(eS2 / mc));
    std::printf("dwelldiag,end_theta_err_rms_deg,%.3f,end_theta_sigma_rms_deg,%.3f\n",
                std::sqrt(thErrDeg2 / mc), std::sqrt(thSdDeg2 / mc));
    const double n = mc;
    std::printf("dwelldiag,mc,%u,nDwell,%u,nTransit,%u\n", mc, nDwell, nTransit);
    std::printf("dwelldiag,neesDwellMean,%.4f\n", neesDwell / nNd);
    std::printf("dwelldiag,z_theta_mean,%.4f,z_theta_rms,%.4f\n", zTh / n, std::sqrt(zTh2 / n));
    std::printf("dwelldiag,z_bias_mean,%.4f,z_bias_rms,%.4f\n", zB / n, std::sqrt(zB2 / n));
    std::printf("dwelldiag,z_stride_mean,%.4f,z_stride_rms,%.4f\n", zS / n, std::sqrt(zS2 / n));
    std::printf("dwelldiag,neesEndOfOutageMean,%.4f,errEndMeanM,%.2f\n", neesEnd / n, errEnd / n);
}

int
main(int argc, char* argv[])
{
    uint32_t mc = 20000;
    uint32_t n = 48; // 120 s at dt = 2.5 s: the longest outage the facility tour produces
    double dt = 2.5;
    double speed = 1.5;
    std::string fixture = "none";
    std::string adevOut = "";
    uint32_t adevSamples = 1u << 18;
    double adevDt = 0.01;

    CommandLine cmd(__FILE__);
    cmd.AddValue("mc", "Monte-Carlo realisations per case", mc);
    cmd.AddValue("n", "Cycles per realisation", n);
    cmd.AddValue("dt", "Cycle length, s", dt);
    cmd.AddValue("speed", "Walking speed, m/s", speed);
    cmd.AddValue("fixture",
                 "Break one term in the MODEL while printing nominal parameters: "
                 "none|arw_var_x2|bias_drop|bias_x2|gm_white|align_drop|"
                 "stride_scale_drop|stride_step_x2var|gscale_drop|"
                 "filter_no_stride_state|filter_q_zero",
                 fixture);
    cmd.AddValue("adevOut",
                 "If set, write a stationary gyro-rate stream (ARW only) here for the "
                 "Allan-deviation check",
                 adevOut);
    cmd.AddValue("adevSamples", "Samples in the Allan stream", adevSamples);
    cmd.AddValue("adevDt", "Sample interval of the Allan stream, s", adevDt);
    uint32_t filterMc = 200;
    uint32_t filterN = 400;
    cmd.AddValue("filterMc", "Realisations in the filter consistency test (0 skips it)", filterMc);
    cmd.AddValue("filterN", "Cycles per realisation in the filter consistency test", filterN);
    uint32_t dwellDiag = 0;
    uint32_t dwellTransit = 200;
    cmd.AddValue("dwellTransit", "Outage cycles after the dwell in the diagnostic", dwellTransit);
    cmd.AddValue("dwellUkf", "Diagnostic only: use the unscented predict", g_dwellUkf);
    cmd.AddValue("dwellMM", "Diagnostic only: moment-matched coasting", g_dwellMM);
    cmd.AddValue("dwellMMNoShift", "Diagnostic only: moment-matched variance, no mean shift", g_dwellMMNoShift);
    cmd.AddValue("dwellDiag", "Run ONLY the site-dwell diagnostic with this many realisations", dwellDiag);
    cmd.Parse(argc, argv);

    std::printf("fixture,%s\n", fixture.c_str());

    const CaTblImuSensorParams nominal;

    struct Spec
    {
        const char* name;
        double turnRate;
        void (*enable)(CaTblImuSensorParams&, const CaTblImuSensorParams&);
    };

    const Spec specs[] = {
        {"arw", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.gyroNoiseDpsRtHz = d.gyroNoiseDpsRtHz;
         }},
        {"bias0", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.gyroBiasInitDps = d.gyroBiasInitDps;
         }},
        // The GM case is run at 100x the nominal sigma: at 5 deg/h its contribution
        // over 120 s is ~1e-5 rad and a defect in it would be invisible to any
        // variance test. Scaling the magnitude leaves the correlation structure --
        // the thing that can be wrong -- intact.
        {"gm", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.gyroBiasGmDps = 100.0 * d.gyroBiasGmDps;
             p.gyroBiasTauS = d.gyroBiasTauS;
         }},
        {"align", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.headingInitDeg = d.headingInitDeg;
         }},
        {"sscale", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.strideScaleErr = d.strideScaleErr;
         }},
        {"sstep", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.strideStepErr = d.strideStepErr;
         }},
        // Scale factor acts on real rotation only, so it needs a turning path.
        {"gscale", 0.1, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) {
             p.gyroScaleErr = d.gyroScaleErr;
         }},
        {"all", 0.0, [](CaTblImuSensorParams& p, const CaTblImuSensorParams& d) { p = d; }},
    };

    uint64_t seedBase = 1000003;
    for (const Spec& s : specs)
    {
        CaTblImuSensorParams printed = Zeroed();
        s.enable(printed, nominal);
        const CaTblImuSensorParams model = Broken(printed, fixture);
        RunCase(s.name, printed, model, mc, n, dt, speed, s.turnRate, seedBase);
        seedBase += 10000019;
    }

    if (dwellDiag > 0)
    {
        RunDwellDiagnostic(dwellDiag, dt, 36, dwellTransit);
        return 0;
    }
    if (filterMc > 0)
    {
        RunFilterTest(fixture, filterMc, filterN, dt, speed);
    }

    if (!adevOut.empty())
    {
        CaTblImuSensorParams printed = Zeroed();
        printed.gyroNoiseDpsRtHz = nominal.gyroNoiseDpsRtHz;
        const CaTblImuSensorParams model = Broken(printed, fixture);
        PrintParams("adev", printed);
        std::printf("case,adev,n,%u,dt,%.17g,speed,0,turnRate,0,mc,1\n", adevSamples, adevDt);
        std::ofstream out(adevOut.c_str());
        if (!out)
        {
            std::cerr << "FATAL: cannot open " << adevOut << "\n";
            return 1;
        }
        CaTblImuSensorModel imu;
        imu.Configure(model, 777);
        // A stationary sensor: zero displacement, so the integrated heading moves only
        // by noise. The first Step defines the frame and carries no turn.
        double prev = imu.Step(0.0, 0.0, adevDt).rawHeadingRad;
        out.precision(17);
        for (uint32_t i = 0; i < adevSamples; ++i)
        {
            const double h = imu.Step(0.0, 0.0, adevDt).rawHeadingRad;
            out << (h - prev) / adevDt << "\n";
            prev = h;
        }
        if (!out)
        {
            std::cerr << "FATAL: write failed on " << adevOut << "\n";
            return 1;
        }
    }
    return 0;
}
