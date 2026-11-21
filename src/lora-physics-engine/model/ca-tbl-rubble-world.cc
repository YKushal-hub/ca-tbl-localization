/*
 * CA-TBL collapse extension -- WORLD side. See ca-tbl-rubble-world.h.
 * Row IDs refer to docs/SOURCES_COLLAPSE.md.
 */
#include "ca-tbl-rubble-world.h"

#include "ns3/abort.h"

#include <algorithm>
#include <cmath>
#include <limits>

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
CaTblRubbleWorld::PileIndex() const
{
    // Effective refractive index of the pile, the same one the slowing model uses:
    // n_eff = 1 + eta (sqrt(eps') - 1), so that n_eff - 1 = BiasPerM() (rows SLOW-BR, FRESNEL).
    return 1.0 + BiasPerM();
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


namespace
{
double Dist(const Vector& a, const Vector& b)
{
    const double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool InBox(const Vector& p, const CaTblRubbleBox& b)
{
    return p.x > b.lo.x && p.x < b.hi.x && p.y > b.lo.y && p.y < b.hi.y && p.z > b.lo.z && p.z < b.hi.z;
}

// A face of a box: a fixed axis/value and the rectangle on the other two axes.
struct Face
{
    int axis;   // 0 x, 1 y, 2 z
    double val; // plane coordinate
};

Vector OnFace(const Vector& p, const Face& f, const CaTblRubbleBox& b)
{
    double c[3] = {p.x, p.y, p.z};
    const double lo[3] = {b.lo.x, b.lo.y, b.lo.z};
    const double hi[3] = {b.hi.x, b.hi.y, b.hi.z};
    for (int i = 0; i < 3; ++i)
    {
        // On the face plane, nudged 1 mm outward so the air leg never grazes the box
        // (a segment lying on a closed face would count as inside).
        const double out = (f.val >= hi[i]) ? 1e-3 : -1e-3;
        c[i] = (i == f.axis) ? f.val + out : std::max(lo[i], std::min(hi[i], c[i]));
    }
    return Vector(c[0], c[1], c[2]);
}

Vector Lerp(const Vector& a, const Vector& b, double t)
{
    return Vector(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y), a.z + t * (b.z - a.z));
}

// Power transmission loss (dB, >= 0) across a plane interface from index n1 to n2, for a ray
// whose direction makes cos1 with the interface normal, averaged over TE and TM (unpolarised).
// ITU-R P.2040-4 eqs. (30), (31a), (31b), (33) and, for lossless media, Ts = 1 - |R|^2 (eq. (35)
// with substitution b), §2.2.1.3). Total internal reflection returns +inf (row FRESNEL).
double TransLossDb(double n1, double n2, double cos1)
{
    cos1 = std::min(1.0, std::max(0.0, std::abs(cos1)));
    const double sin1 = std::sqrt(std::max(0.0, 1.0 - cos1 * cos1));
    const double s2 = n1 / n2 * sin1; // Snell, eq. (30)
    if (s2 >= 1.0)
    {
        return std::numeric_limits<double>::infinity();
    }
    const double cos2 = std::sqrt(1.0 - s2 * s2); // eq. (33)
    const double rTe = (n1 * cos1 - n2 * cos2) / (n1 * cos1 + n2 * cos2); // eq. (31a)
    const double rTm = (n2 * cos1 - n1 * cos2) / (n2 * cos1 + n1 * cos2); // eq. (31b)
    const double t = 0.5 * ((1.0 - rTe * rTe) + (1.0 - rTm * rTm));
    return (t > 0.0) ? -10.0 * std::log10(t) : std::numeric_limits<double>::infinity();
}

// cos of the angle between segment p->q and the axis of a face (the face normal).
double CosToAxis(const Vector& p, const Vector& q, int axis)
{
    const double d[3] = {q.x - p.x, q.y - p.y, q.z - p.z};
    const double n = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    return (n > 0.0) ? std::abs(d[axis]) / n : 1.0;
}

// The axis (0/1/2) whose slab bounds the entry (t0) or exit (t1) of segment a->b in a box.
// Returns -1 if the segment misses the box or that end lies inside the box.
void CrossAxes(const Vector& a, const Vector& b, const CaTblRubbleBox& box, int& inAxis, int& outAxis)
{
    const double d[3] = {b.x - a.x, b.y - a.y, b.z - a.z};
    const double o[3] = {a.x, a.y, a.z};
    const double lo[3] = {box.lo.x, box.lo.y, box.lo.z};
    const double hi[3] = {box.hi.x, box.hi.y, box.hi.z};
    double t0 = 0.0, t1 = 1.0;
    inAxis = -1;
    outAxis = -1;
    for (int i = 0; i < 3; ++i)
    {
        if (d[i] == 0.0)
        {
            if (o[i] < lo[i] || o[i] > hi[i])
            {
                inAxis = outAxis = -1;
                return;
            }
            continue;
        }
        double ta = (lo[i] - o[i]) / d[i], tb = (hi[i] - o[i]) / d[i];
        if (ta > tb)
        {
            std::swap(ta, tb);
        }
        if (ta > t0)
        {
            t0 = ta;
            inAxis = i;
        }
        if (tb < t1)
        {
            t1 = tb;
            outAxis = i;
        }
    }
    if (t0 >= t1)
    {
        inAxis = outAxis = -1;
    }
}
} // namespace

CaTblRubblePath
CaTblRubbleWorld::Path(double frequencyHz, const Vector& a, const Vector& b) const
{
    CaTblRubblePath best;
    const double d = std::max(Dist(a, b), 1e-3);
    const double straightM = PileMetres(a, b);
    if (straightM <= 0.0)
    {
        best.lengthM = d;
        return best;
    }
    const double A = SpecificLossDbPerM(frequencyHz);
    const double k = BiasPerM();
    const double nP = PileIndex();
    // Interface loss of every pile-boundary crossing on a polyline, at the actual angles.
    auto interfaceDb = [&](const std::vector<Vector>& pts) {
        double db = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            for (const auto& bx : m_rubble)
            {
                int inAx, outAx;
                CrossAxes(pts[i], pts[i + 1], bx, inAx, outAx);
                const bool startIn = InBox(pts[i], bx);
                const bool endIn = InBox(pts[i + 1], bx);
                if (inAx >= 0 && !startIn) // air -> rubble
                {
                    db += TransLossDb(1.0, nP, CosToAxis(pts[i], pts[i + 1], inAx));
                }
                if (outAx >= 0 && !endIn) // rubble -> air
                {
                    db += TransLossDb(nP, 1.0, CosToAxis(pts[i], pts[i + 1], outAx));
                }
            }
        }
        return db;
    };
    auto pathCost = [&](const std::vector<Vector>& pts, double& len, double& rub) {
        len = 0.0;
        rub = 0.0;
        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            len += Dist(pts[i], pts[i + 1]);
            rub += PileMetres(pts[i], pts[i + 1]);
        }
        return A * rub + 20.0 * std::log10(std::max(len, d) / d) + interfaceDb(pts);
    };
    auto consider = [&](const std::vector<Vector>& via, int kind) {
        std::vector<Vector> pts;
        pts.push_back(a);
        pts.insert(pts.end(), via.begin(), via.end());
        pts.push_back(b);
        double len, rub;
        const double cost = pathCost(pts, len, rub);
        if (!std::isfinite(cost))
        {
            return;
        }
        if (best.kind == 0 || cost < best.extraLossDb)
        {
            best.extraLossDb = cost;
            best.rubbleM = rub;
            best.lengthM = len;
            best.biasM = (len - d) + k * rub;
            best.kind = kind;
        }
    };
    consider({}, 1);

    // The pile box holding each endpoint (if any).
    const CaTblRubbleBox* boxA = nullptr;
    const CaTblRubbleBox* boxB = nullptr;
    for (const auto& r : m_rubble)
    {
        if (InBox(a, r))
        {
            boxA = &r;
        }
        if (InBox(b, r))
        {
            boxB = &r;
        }
    }
    const int kSamples = 48;
    // One exit: the buried endpoint u leaves its box through a face, then air to the other.
    auto oneExit = [&](const Vector& u, const Vector& o, const CaTblRubbleBox& bx, bool uIsA) {
        const Face faces[5] = {{2, bx.hi.z}, {0, bx.lo.x}, {0, bx.hi.x}, {1, bx.lo.y}, {1, bx.hi.y}};
        for (const Face& f : faces)
        {
            const Vector p1 = OnFace(u, f, bx); // nearest way out through this face
            const Vector p2 = OnFace(o, f, bx); // the face point nearest the other end
            double bestT = 0.0, bestC = 1e300;
            for (int pass = 0; pass < 2; ++pass)
            {
                const double t0 = (pass == 0) ? 0.0 : std::max(0.0, bestT - 1.0 / kSamples);
                const double t1 = (pass == 0) ? 1.0 : std::min(1.0, bestT + 1.0 / kSamples);
                for (int i = 0; i <= kSamples; ++i)
                {
                    const double t = t0 + (t1 - t0) * i / kSamples;
                    const Vector e = Lerp(p1, p2, t);
                    double len, rub;
                    const double c = pathCost({u, e, o}, len, rub);
                    if (c < bestC)
                    {
                        bestC = c;
                        bestT = t;
                    }
                }
            }
            const Vector e = Lerp(p1, p2, bestT);
            consider({e}, 2);
            (void)uIsA;
        }
    };
    if (boxA && !boxB)
    {
        oneExit(a, b, *boxA, true);
    }
    else if (boxB && !boxA)
    {
        oneExit(b, a, *boxB, false);
    }
    else if (boxA && boxB)
    {
        // Out of the top above a, through the air, back in above b.
        const Face top{2, boxA->hi.z};
        const Face topB{2, boxB->hi.z};
        const Vector p1 = OnFace(a, top, *boxA);
        const Vector p2 = OnFace(b, topB, *boxB);
        const int n = 24;
        double bi = 0, bj = 1, bc = 1e300;
        for (int i = 0; i <= n; ++i)
        {
            for (int j = 0; j <= n; ++j)
            {
                const Vector e1 = Lerp(p1, p2, static_cast<double>(i) / n);
                const Vector e2 = Lerp(p1, p2, static_cast<double>(j) / n);
                double len, rub;
                const double c = pathCost({a, e1, e2, b}, len, rub);
                if (c < bc)
                {
                    bc = c;
                    bi = static_cast<double>(i) / n;
                    bj = static_cast<double>(j) / n;
                }
            }
        }
        consider({Lerp(p1, p2, bi), Lerp(p1, p2, bj)}, 3);
    }
    return best;
}

} // namespace ns3
