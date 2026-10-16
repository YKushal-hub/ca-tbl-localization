#include "ca-tbl-imu-sensor-model.h"

#include <cmath>

namespace ns3
{

namespace
{
const double kDegToRad = M_PI / 180.0;

inline uint64_t
Rotl(uint64_t x, int k)
{
    return (x << k) | (x >> (64 - k));
}

inline uint64_t
SplitMix64(uint64_t& x)
{
    uint64_t z = (x += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// Mixed into the seed so the IMU's sequence is not a function of the seed alone in a
// way some other private generator might also use. The value is arbitrary.
const uint64_t kImuSalt = 0x494d555052494f52ULL; // "IMUPRIOR"
} // namespace

CaTblImuSensorParams::CaTblImuSensorParams()
    : gyroNoiseDpsRtHz(0.007),
      gyroBiasInitDps(0.15),
      gyroBiasGmDps(5.0 / 3600.0),
      gyroBiasTauS(300.0),
      gyroScaleErr(0.004),
      headingInitDeg(10.0),
      strideLengthM(0.75),
      strideScaleErr(0.03),
      strideStepErr(0.05),
      currentMa(0.970)
{
}

CaTblImuSensorIncrement::CaTblImuSensorIncrement()
    : distM(0.0),
      rawHeadingRad(0.0),
      valid(false)
{
}

CaTblImuSensorModel::CaTblImuSensorModel()
    : m_b0(0.0),
      m_sg(0.0),
      m_sd(0.0),
      m_align(0.0),
      m_bGm(0.0),
      m_rawHeading(0.0),
      m_psiTruePrev(0.0),
      m_started(false),
      m_steps(0)
{
    m_s[0] = m_s[1] = m_s[2] = m_s[3] = 0;
}

double
CaTblImuSensorModel::Wrap(double a)
{
    a = std::fmod(a + M_PI, 2.0 * M_PI);
    if (a <= 0.0)
    {
        a += 2.0 * M_PI;
    }
    return a - M_PI;
}

uint64_t
CaTblImuSensorModel::NextU64()
{
    // xoshiro256** (Blackman & Vigna). Output is specified bit-for-bit, unlike the
    // distributions in <random>, whose algorithms are implementation-defined.
    const uint64_t result = Rotl(m_s[1] * 5, 7) * 9;
    const uint64_t t = m_s[1] << 17;
    m_s[2] ^= m_s[0];
    m_s[3] ^= m_s[1];
    m_s[1] ^= m_s[2];
    m_s[0] ^= m_s[3];
    m_s[2] ^= t;
    m_s[3] = Rotl(m_s[3], 45);
    return result;
}

double
CaTblImuSensorModel::Normal()
{
    // Box-Muller, one normal per call. Uniforms are drawn on the open interval
    // (0,1) so log() never sees zero. Two uniforms per normal, always -- the draw
    // count is independent of every parameter value.
    const double u1 = (static_cast<double>(NextU64() >> 11) + 0.5) * (1.0 / 9007199254740992.0);
    const double u2 = (static_cast<double>(NextU64() >> 11) + 0.5) * (1.0 / 9007199254740992.0);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
}

void
CaTblImuSensorModel::Configure(const CaTblImuSensorParams& p, uint64_t seed)
{
    m_p = p;
    uint64_t x = seed ^ kImuSalt;
    for (int i = 0; i < 4; ++i)
    {
        m_s[i] = SplitMix64(x);
    }

    // Per-run constants, drawn in a fixed order. A part has one turn-on bias, one
    // scale factor and one alignment for the whole run; drawing them per cycle
    // would make them white noise, which averages out -- the error the Python model
    // made in the other direction by not having them at all.
    m_b0 = p.gyroBiasInitDps * kDegToRad * Normal();
    m_sg = p.gyroScaleErr * Normal();
    m_sd = p.strideScaleErr * Normal();
    m_align = p.headingInitDeg * kDegToRad * Normal();
    // Start the Gauss-Markov process in its stationary distribution, so the bias
    // statistics do not depend on how long the run has been going.
    m_bGm = p.gyroBiasGmDps * kDegToRad * Normal();

    m_rawHeading = 0.0;
    m_psiTruePrev = 0.0;
    m_started = false;
    m_steps = 0;
}

CaTblImuSensorIncrement
CaTblImuSensorModel::Step(double dxTrue, double dyTrue, double dt)
{
    const double d = std::hypot(dxTrue, dyTrue);
    double psiTrue = (d > 1e-9) ? std::atan2(dyTrue, dxTrue) : m_psiTruePrev;

    if (!m_started)
    {
        // The first displacement defines the frame the tag starts in. Its integrated
        // heading begins at the true heading plus the alignment error, and no turn is
        // integrated on this cycle -- there is no previous heading to turn from.
        m_psiTruePrev = psiTrue;
        m_rawHeading = psiTrue + m_align;
        m_started = true;
    }

    // Heading actually turned through this cycle. Tourist moves along its heading
    // AFTER updating it, so the displacement direction is the end-of-cycle heading.
    const double turn = Wrap(psiTrue - m_psiTruePrev);

    // Gyro: integrated heading increment over the cycle.
    //   (1 + s_g) * turn      scale factor acts on real rotation only
    //   b * dt                bias held piecewise-constant across the cycle
    //   N * sqrt(dt) * w      white rate noise integrates to angle random walk;
    //                         the noise density in deg/s/sqrt(Hz) IS the ARW
    //                         coefficient in deg/sqrt(s) (IEEE Std 952 convention)
    const double b = m_b0 + m_bGm;
    const double wArw = Normal();
    const double wGm = Normal();
    const double wStep = Normal();

    const double nArw = m_p.gyroNoiseDpsRtHz * kDegToRad * std::sqrt(dt);
    m_rawHeading += (1.0 + m_sg) * turn + b * dt + nArw * wArw;

    // Advance the Gauss-Markov bias AFTER it has been applied: exact discretisation,
    // b[k+1] = beta b[k] + sigma sqrt(1 - beta^2) w, stationary variance sigma^2.
    if (m_p.gyroBiasTauS > 0.0)
    {
        const double beta = std::exp(-dt / m_p.gyroBiasTauS);
        m_bGm = beta * m_bGm +
                m_p.gyroBiasGmDps * kDegToRad * std::sqrt(1.0 - beta * beta) * wGm;
    }

    // Stride model: a per-run scale error on the whole distance, plus independent
    // per-step errors. n = d / L steps, each with sigma = e * L, sum sigma =
    // e * L * sqrt(n) = e * sqrt(L * d).
    double dHat = (1.0 + m_sd) * d;
    if (m_p.strideLengthM > 0.0 && d > 0.0)
    {
        dHat += m_p.strideStepErr * std::sqrt(m_p.strideLengthM * d) * wStep;
    }
    if (dHat < 0.0)
    {
        dHat = 0.0; // a negative distance is not a measurement a step counter can make
    }

    m_psiTruePrev = psiTrue;
    m_steps++;

    CaTblImuSensorIncrement inc;
    inc.distM = dHat;
    inc.rawHeadingRad = m_rawHeading;
    inc.valid = true;
    return inc;
}

double
CaTblImuSensorModel::TrueHeadingErrRad() const
{
    return Wrap(m_rawHeading - m_psiTruePrev);
}

double
CaTblImuSensorModel::TrueBiasRadS() const
{
    return m_b0 + m_bGm;
}

} // namespace ns3
