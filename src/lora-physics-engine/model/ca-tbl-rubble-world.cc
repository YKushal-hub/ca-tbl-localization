/*
 * CA-TBL collapse extension -- WORLD side. See ca-tbl-rubble-world.h.
 * Row IDs refer to docs/SOURCES_COLLAPSE.md.
 */
#include "ca-tbl-rubble-world.h"

#include "ns3/abort.h"

#include <algorithm>
#include <cmath>

namespace ns3
{

namespace
{
// --- Sub-3 GHz rubble brackets: A(f) = A2 * (f / 2 GHz)^k --------------------
// A2 (dB/m at 2 GHz): low OES layered without ferro-concrete "8 and 20 dB/m";
// central OES with ferro "14 and 31 dB/m"; high OES measured "72 and 80 dB/m"
// (80 measured). Rows OES-LAYER-PLAIN, OES-LAYER-FERRO, OES-MEAS-80.
const double kA2DbPerM[3] = {20.0, 31.0, 80.0};
// k = log2(A2/A1): low log2(20/8) = 1.32193; central/high log2(31/14) = 1.14684.
// Rows OES-LAYER-PLAIN, OES-LAYER-FERRO (k column). Computed, not typed, so the code equals the row exactly.
double KExp(CaTblRubbleBracket b)
{
    return (b == CA_TBL_RUBBLE_LOW) ? std::log2(20.0 / 8.0) : std::log2(31.0 / 14.0);
}

// --- ITU-R P.2040-4 concrete (row ITU2040-T3-CONC): a = 5.24, b = 0, c = 0.0462,
// d = 0.7822, f in GHz; A = 1636 * sigma / sqrt(eps') (eq. (27a), row ITU2040-E27a).
const double kItuA = 5.24;
const double kItuC = 0.0462;
const double kItuD = 0.7822;
const double kE27a = 1636.0;

// --- NISTIR 6055 solid plain concrete slope at 6.5 GHz (row NIST6055-HIGH).
const double kNist65DbPerM = 242.34;
const double kNistRefHz = 6.5e9;

// --- Solid fraction (rows ETA-LOW / ETA-CEN / ETA-HIGH): A_rubble(2 GHz) /
// A_solid(2 GHz). Low and central divide by ITU at 2 GHz; high by NIST 103.82
// (row NIST6055-LOW). Computed so the code equals the derivation.
const double kNist2DbPerM = 103.82;

// --- Relative permittivity paired with each bracket's solid-loss source
// (row SLOW-BR): low/central ITU 5.24, high Oestges concrete 9 (row OES-EPS-CONC).
const double kEpsR[3] = {5.24, 5.24, 9.0};

// Band edges with a sourced value. Sub-GHz: the Oestges law is anchored at
// 1-2 GHz and used down to 868 MHz (13% extrapolation, stated in the paper);
// 2.4 GHz is a 20-22% extrapolation (stated). Above 3 GHz: NIST/ITU data cover
// 3.0-8.0 GHz; DW3000 channel 9 is 7.99 GHz.
const double kSubLoHz = 0.8e9;
const double kSubHiHz = 2.5e9;
const double kUwbLoHz = 3.0e9;
const double kUwbHiHz = 8.5e9;

double Clamp01(double x)
{
    return std::max(0.0, std::min(1.0, x));
}

bool Overlap(const CaTblRubbleBox& p, const CaTblRubbleBox& q)
{
    return p.lo.x < q.hi.x && q.lo.x < p.hi.x && p.lo.y < q.hi.y && q.lo.y < p.hi.y &&
           p.lo.z < q.hi.z && q.lo.z < p.hi.z;
}

bool Inside(const CaTblRubbleBox& inner, const CaTblRubbleBox& outer)
{
    return inner.lo.x >= outer.lo.x && inner.hi.x <= outer.hi.x && inner.lo.y >= outer.lo.y &&
           inner.hi.y <= outer.hi.y && inner.lo.z >= outer.lo.z && inner.hi.z <= outer.hi.z;
}

bool Degenerate(const CaTblRubbleBox& b)
{
    return !(b.hi.x > b.lo.x && b.hi.y > b.lo.y && b.hi.z > b.lo.z);
}

bool PointIn(const Vector& p, const CaTblRubbleBox& b)
{
    return p.x >= b.lo.x && p.x <= b.hi.x && p.y >= b.lo.y && p.y <= b.hi.y && p.z >= b.lo.z &&
           p.z <= b.hi.z;
}
} // namespace

bool
ParseRubbleBracket(const std::string& s, CaTblRubbleBracket& out)
{
    if (s == "low")
    {
        out = CA_TBL_RUBBLE_LOW;
        return true;
    }
    if (s == "central")
    {
        out = CA_TBL_RUBBLE_CENTRAL;
        return true;
    }
    if (s == "high")
    {
        out = CA_TBL_RUBBLE_HIGH;
        return true;
    }
    return false;
}

double
SegmentBoxLength(const Vector& a, const Vector& b, const CaTblRubbleBox& box)
{
    const double d[3] = {b.x - a.x, b.y - a.y, b.z - a.z};
    const double o[3] = {a.x, a.y, a.z};
    const double lo[3] = {box.lo.x, box.lo.y, box.lo.z};
    const double hi[3] = {box.hi.x, box.hi.y, box.hi.z};
    double t0 = 0.0;
    double t1 = 1.0;
    for (int i = 0; i < 3; ++i)
    {
        if (d[i] == 0.0)
        {
            if (o[i] < lo[i] || o[i] > hi[i])
            {
                return 0.0;
            }
            continue;
        }
        double ta = (lo[i] - o[i]) / d[i];
        double tb = (hi[i] - o[i]) / d[i];
        if (ta > tb)
        {
            std::swap(ta, tb);
        }
        t0 = std::max(t0, ta);
        t1 = std::min(t1, tb);
        if (t0 >= t1)
        {
            return 0.0;
        }
    }
    const double len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    return (Clamp01(t1) - Clamp01(t0)) * len;
}

CaTblRubbleWorld::CaTblRubbleWorld(CaTblRubbleBracket b)
    : m_bracket(b)
{
}

bool
CaTblRubbleWorld::AddRubble(const Vector& lo, const Vector& hi)
{
    CaTblRubbleBox box{lo, hi};
    if (Degenerate(box))
    {
        return false;
    }
    for (const auto& r : m_rubble)
    {
        if (Overlap(box, r))
        {
            return false;
        }
    }
    m_rubble.push_back(box);
    return true;
}

bool
CaTblRubbleWorld::AddVoid(const Vector& lo, const Vector& hi)
{
    CaTblRubbleBox box{lo, hi};
    if (Degenerate(box))
    {
        return false;
    }
    bool contained = false;
    for (const auto& r : m_rubble)
    {
        if (Inside(box, r))
        {
            contained = true;
            break;
        }
    }
    if (!contained)
    {
        return false;
    }
    for (const auto& v : m_voids)
    {
        if (Overlap(box, v))
        {
            return false;
        }
    }
    m_voids.push_back(box);
    return true;
}

double
CaTblRubbleWorld::PileMetres(const Vector& a, const Vector& b) const
{
    double m = 0.0;
    for (const auto& r : m_rubble)
    {
        m += SegmentBoxLength(a, b, r);
    }
    for (const auto& v : m_voids)
    {
        m -= SegmentBoxLength(a, b, v);
    }
    return std::max(0.0, m);
}

bool
CaTblRubbleWorld::InRubble(const Vector& p) const
{
    for (const auto& v : m_voids)
    {
        if (PointIn(p, v))
        {
            return false;
        }
    }
    for (const auto& r : m_rubble)
    {
        if (PointIn(p, r))
        {
            return true;
        }
    }
    return false;
}

double
CaTblRubbleWorld::ItuConcreteDbPerM(double frequencyHz)
{
    const double fGhz = frequencyHz / 1e9;
    const double sigma = kItuC * std::pow(fGhz, kItuD); // eq. (58)
    return kE27a * sigma / std::sqrt(kItuA);            // eq. (27a), eps' = a (b = 0)
}

double
CaTblRubbleWorld::Eta(CaTblRubbleBracket b)
{
    const double solid2 = (b == CA_TBL_RUBBLE_HIGH) ? kNist2DbPerM : ItuConcreteDbPerM(2.0e9);
    return kA2DbPerM[b] / solid2;
}

double
CaTblRubbleWorld::SpecificLossDbPerM(double frequencyHz) const
{
    if (frequencyHz >= kSubLoHz && frequencyHz <= kSubHiHz)
    {
        return kA2DbPerM[m_bracket] * std::pow(frequencyHz / 2.0e9, KExp(m_bracket));
    }
    if (frequencyHz >= kUwbLoHz && frequencyHz <= kUwbHiHz)
    {
        double solid;
        if (m_bracket == CA_TBL_RUBBLE_HIGH)
        {
            // NIST 6.5 GHz scaled by the ITU frequency ratio (row UWB8-HIGH).
            solid = kNist65DbPerM * ItuConcreteDbPerM(frequencyHz) / ItuConcreteDbPerM(kNistRefHz);
        }
        else
        {
            solid = ItuConcreteDbPerM(frequencyHz);
        }
        return Eta(m_bracket) * solid;
    }
    NS_ABORT_MSG("CaTblRubbleWorld: no sourced rubble loss at " << frequencyHz << " Hz");
    return 0.0;
}

double
CaTblRubbleWorld::BiasPerM() const
{
    return Eta(m_bracket) * (std::sqrt(kEpsR[m_bracket]) - 1.0);
}

double
CaTblRubbleWorld::LossDb(double frequencyHz, const Vector& a, const Vector& b) const
{
    const double m = PileMetres(a, b);
    return (m > 0.0) ? SpecificLossDbPerM(frequencyHz) * m : 0.0;
}

double
CaTblRubbleWorld::BiasM(const Vector& a, const Vector& b) const
{
    const double m = PileMetres(a, b);
    return (m > 0.0) ? BiasPerM() * m : 0.0;
}

} // namespace ns3
