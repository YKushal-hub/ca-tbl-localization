/**
 * \file ca_tbl_imu_validator.cc
 * \brief Monte-Carlo harness for CaTblImuModel. Produces SAMPLE statistics only.
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

#include "ns3/ca-tbl-imu-model.h"
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

CaTblImuParams
Zeroed()
{
    CaTblImuParams p;
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
CaTblImuParams
Broken(CaTblImuParams p, const std::string& fixture)
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
PrintParams(const std::string& name, const CaTblImuParams& p)
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
        const CaTblImuParams& printed,
        const CaTblImuParams& model,
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
        CaTblImuModel imu;
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
            const CaTblImuIncrement inc = imu.Step(dx, dy, dt);
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
                 "stride_scale_drop|stride_step_x2var|gscale_drop",
                 fixture);
    cmd.AddValue("adevOut",
                 "If set, write a stationary gyro-rate stream (ARW only) here for the "
                 "Allan-deviation check",
                 adevOut);
    cmd.AddValue("adevSamples", "Samples in the Allan stream", adevSamples);
    cmd.AddValue("adevDt", "Sample interval of the Allan stream, s", adevDt);
    cmd.Parse(argc, argv);

    std::printf("fixture,%s\n", fixture.c_str());

    const CaTblImuParams nominal;

    struct Spec
    {
        const char* name;
        double turnRate;
        void (*enable)(CaTblImuParams&, const CaTblImuParams&);
    };

    const Spec specs[] = {
        {"arw", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.gyroNoiseDpsRtHz = d.gyroNoiseDpsRtHz;
         }},
        {"bias0", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.gyroBiasInitDps = d.gyroBiasInitDps;
         }},
        // The GM case is run at 100x the nominal sigma: at 5 deg/h its contribution
        // over 120 s is ~1e-5 rad and a defect in it would be invisible to any
        // variance test. Scaling the magnitude leaves the correlation structure --
        // the thing that can be wrong -- intact.
        {"gm", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.gyroBiasGmDps = 100.0 * d.gyroBiasGmDps;
             p.gyroBiasTauS = d.gyroBiasTauS;
         }},
        {"align", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.headingInitDeg = d.headingInitDeg;
         }},
        {"sscale", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.strideScaleErr = d.strideScaleErr;
         }},
        {"sstep", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.strideStepErr = d.strideStepErr;
         }},
        // Scale factor acts on real rotation only, so it needs a turning path.
        {"gscale", 0.1, [](CaTblImuParams& p, const CaTblImuParams& d) {
             p.gyroScaleErr = d.gyroScaleErr;
         }},
        {"all", 0.0, [](CaTblImuParams& p, const CaTblImuParams& d) { p = d; }},
    };

    uint64_t seedBase = 1000003;
    for (const Spec& s : specs)
    {
        CaTblImuParams printed = Zeroed();
        s.enable(printed, nominal);
        const CaTblImuParams model = Broken(printed, fixture);
        RunCase(s.name, printed, model, mc, n, dt, speed, s.turnRate, seedBase);
        seedBase += 10000019;
    }

    if (!adevOut.empty())
    {
        CaTblImuParams printed = Zeroed();
        printed.gyroNoiseDpsRtHz = nominal.gyroNoiseDpsRtHz;
        const CaTblImuParams model = Broken(printed, fixture);
        PrintParams("adev", printed);
        std::printf("case,adev,n,%u,dt,%.17g,speed,0,turnRate,0,mc,1\n", adevSamples, adevDt);
        std::ofstream out(adevOut.c_str());
        if (!out)
        {
            std::cerr << "FATAL: cannot open " << adevOut << "\n";
            return 1;
        }
        CaTblImuModel imu;
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
