/*
 * CA-TBL collapse extension -- WORLD side only.
 *
 * Ground truth of a collapsed structure: where the rubble is, where the
 * survivable voids are, and how much the rubble attenuates and slows a radio
 * signal at each band. The three physics engines read it (when it is set) to
 * add the rubble loss to the link budget and the rubble delay to time-of-flight
 * ranges. Nothing on the SYSTEM side (policy, filters, estimators, MAC) may
 * include this header: the system knows only its radio, IMU and GPS
 * measurements. scripts/rubble_gate.sh fails if it does.
 *
 * Every constant carries the ID of its row in docs/SOURCES_COLLAPSE.md.
 * Default: no world is set on any engine, so every earlier result is unchanged.
 */
#ifndef CA_TBL_RUBBLE_WORLD_H
#define CA_TBL_RUBBLE_WORLD_H

#include "ns3/vector.h"

#include <string>
#include <vector>

namespace ns3
{

enum CaTblRubbleBracket
{
    CA_TBL_RUBBLE_LOW = 0,
    CA_TBL_RUBBLE_CENTRAL = 1,
    CA_TBL_RUBBLE_HIGH = 2
};

//! Parse "low" / "central" / "high"; returns false on anything else.
bool ParseRubbleBracket(const std::string& s, CaTblRubbleBracket& out);

struct CaTblRubbleBox
{
    Vector lo; //!< min corner (m)
    Vector hi; //!< max corner (m)
};

//! The lowest-loss path between two points (DD 37).
struct CaTblRubblePath
{
    double extraLossDb = 0.0; //!< rubble loss + extra spreading of the bent path over the straight one
    double biasM = 0.0;       //!< apparent extra distance: bent-path excess + rubble slowing
    double rubbleM = 0.0;     //!< metres of rubble (not void) on the chosen path
    double lengthM = 0.0;     //!< total length of the chosen path
    int kind = 0;             //!< 0 none, 1 straight, 2 one exit, 3 out-and-back-in
};

class CaTblRubbleWorld
{
  public:
    explicit CaTblRubbleWorld(CaTblRubbleBracket b = CA_TBL_RUBBLE_CENTRAL);

    void SetBracket(CaTblRubbleBracket b) { m_bracket = b; }
    CaTblRubbleBracket GetBracket() const { return m_bracket; }

    //! Rubble boxes must not overlap each other; returns false (and adds
    //! nothing) if the new box overlaps an existing rubble box or is degenerate.
    bool AddRubble(const Vector& lo, const Vector& hi);
    //! A void must lie wholly inside one rubble box and must not overlap
    //! another void; returns false otherwise.
    bool AddVoid(const Vector& lo, const Vector& hi);

    const std::vector<CaTblRubbleBox>& Rubble() const { return m_rubble; }
    const std::vector<CaTblRubbleBox>& Voids() const { return m_voids; }

    //! Metres of the straight segment a-b that lie in rubble (not in a void).
    double PileMetres(const Vector& a, const Vector& b) const;

    //! True if p lies inside rubble (and not inside a void).
    bool InRubble(const Vector& p) const;

    //! Specific attenuation of the rubble pile at f (Hz), dB per metre of pile.
    //! Sub-3 GHz: Oestges-based power law (rows OES-LAYER-*, OES-MEAS-80). Above 3 GHz:
    //! derived solid fraction times solid concrete loss (rows ETA-*, UWB*-*).
    //! Aborts for 2.5 < f < 3 GHz and outside 0.8-8.5 GHz (no sourced value).
    double SpecificLossDbPerM(double frequencyHz) const;

    //! Extra apparent distance per metre of pile (m/m), row SLOW-BR.
    double BiasPerM() const;

    //! Effective refractive index of the pile, 1 + BiasPerM() (rows SLOW-BR, FRESNEL).
    double PileIndex() const;

    //! Straight-segment loss (dB) and time-of-flight bias (m) for a-b (DD 36). Kept for the
    //! gate's fixtures; the engines use Path().
    double LossDb(double frequencyHz, const Vector& a, const Vector& b) const;
    double BiasM(const Vector& a, const Vector& b) const;

    //! DD 37. The lowest-loss path between a and b at f: the straight segment, or for a device
    //! inside the pile, rubble to an exit point on the pile's surface (top or a side) and air
    //! from there; for two buried devices also out of the pile above one and back in above the
    //! other. Cost = rubble loss + 20 log10(path length / straight distance). Rubble refracts
    //! the ray towards the normal on exit (refractive index sqrt(eps') ~ 2.3-3), which is why
    //! the low-loss exit is close to the shortest way out (Snell's law,
    //! row SNELL). Each boundary crossing adds the Fresnel power-transmission loss at its angle
    //! (ITU-R P.2040-4 eqs. (30)-(35), unpolarised); rays beyond the critical angle are
    //! totally reflected and that path is not available (row FRESNEL).
    CaTblRubblePath Path(double frequencyHz, const Vector& a, const Vector& b) const;

    //! Static helpers, exposed for the gate's unit fixtures.
    static double ItuConcreteDbPerM(double frequencyHz); // ITU2040-T3-CONC, eq. (27a)
    static double Eta(CaTblRubbleBracket b);             // ETA-*

  private:
    CaTblRubbleBracket m_bracket;
    std::vector<CaTblRubbleBox> m_rubble;
    std::vector<CaTblRubbleBox> m_voids;
};

//! Length of segment a-b inside the axis-aligned box (slab method).
double SegmentBoxLength(const Vector& a, const Vector& b, const CaTblRubbleBox& box);

} // namespace ns3

#endif // CA_TBL_RUBBLE_WORLD_H
