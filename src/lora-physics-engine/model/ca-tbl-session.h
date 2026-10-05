/*
 * CA-TBL Transport & Application Layer -- session lifecycle, result provenance, and the
 * SEARCH_AND_LOCALIZE / BATCH_SEARCH orchestration.
 *
 * WHY THIS LAYER IS THIN, DELIBERATELY. The architecture spec makes the point directly:
 * the orchestration "does almost no work itself -- it's a thin orchestration layer that
 * calls down into mechanisms already fully specified at the physical/MAC/network layers."
 * Everything hard -- confidence estimation, integrity validation, arbitration, relay
 * admission -- was solved once, below. If this file starts growing logic of its own, that
 * is a signal something below it is missing, not that this layer needs to be cleverer.
 *
 * WHAT IT ADDS THAT THE LAYERS BELOW CANNOT: a question has state over time. A radio fix
 * is an instant; a SEARCH is a thing a rescuer opened, is still waiting on, and will
 * eventually close. Nothing below this layer has anywhere to put that.
 *
 * The operational distinction worth stating in any write-up: this system runs INWARD.
 * Comparable trackers follow one continuously-worn tag outward, where the operator is the
 * tracked node. Here a rescuer who is NOT the thing being located opens a multi-target
 * search session. That is an application-layer difference, not a radio one.
 */

#ifndef CA_TBL_SESSION_H
#define CA_TBL_SESSION_H

#include "ca-tbl-energy-model.h"
#include "ca-tbl-header.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ns3
{

/**
 * \brief Session lifecycle (Transport Layer §2).
 *
 * A query is not fire-and-forget. The states below are the spec's, unmodified.
 *
 * STALE RECOVERS RATHER THAN REOPENING. A session whose fixes dried up and then resumed
 * returns to TRACKING; it does not start again from SEARCHING. Reopening would discard
 * the reverse path, the escalation history and the time-to-first-fix already earned, and
 * would report a second "first fix" for a target that was never actually lost.
 */
enum CaTblSessionState : uint8_t
{
    CA_TBL_SESSION_SEARCHING = 0,       //!< opened, nothing heard back yet
    CA_TBL_SESSION_LOCATED_COARSE = 1,  //!< any response, tier 868 or 2.4
    CA_TBL_SESSION_LOCATED_PRECISE = 2, //!< a UWB fix arrived
    CA_TBL_SESSION_TRACKING = 3,        //!< held open past the first precise fix
    CA_TBL_SESSION_STALE = 4,           //!< no fresh fix within the staleness bound
    CA_TBL_SESSION_CLOSED = 5           //!< timed out, or cancelled because the subject was reached
};

const char* CaTblSessionStateName(CaTblSessionState s);

/**
 * \brief What the application is handed for one target (Application Layer §3).
 *
 * EVERY RESULT CARRIES ITS LINEAGE, not just coordinates. That is the point: a rescuer can
 * decide whether to trust what is on the screen without understanding the filter. Most
 * comparable systems surface a position; this surfaces a position plus how it was come by.
 */
struct CaTblResult
{
    uint8_t targetManifestId;
    double x;
    double y;
    //! Uncertainty as the filter actually holds it. The renderer draws the ellipse from
    //! these, not from a stylised radius -- discarding the covariance at the last step
    //! would throw away the statistical work every layer below spent effort producing.
    double varX;
    double varY;
    double covXY;
    CaTblTier tierSource;
    double pcs;
    uint8_t hopCountAtFix;
    //! TA-3. The producer's entry said it holds a confirmed UWB grant (NW-7's grantedBy).
    //! What makes a session LOCATED_PRECISE under SetPreciseOnGrant(true).
    bool uwbGranted = false;
    //! The switching-point integrity verdict, carried all the way up. Surfacing it to the
    //! operator is the entire reason that check exists.
    bool suspect;
    double timestampS;

    CaTblResult();
};

/**
 * \brief How a result should be presented (Application Layer §3).
 *
 * Decision logic, not a UI mock. Kept in the model so it is testable and so that a display
 * built later cannot quietly disagree with it.
 */
enum CaTblRenderClass : uint8_t
{
    CA_TBL_RENDER_VERIFY = 0,   //!< "position uncertain -- verify"
    CA_TBL_RENDER_HIGH = 1,     //!< UWB: high-confidence marker
    CA_TBL_RENDER_MEDIUM = 2,   //!< 2.4 GHz: medium marker, ~10%-of-distance radius
    CA_TBL_RENDER_COARSE = 3    //!< 868: coarse marker with an ellipse drawn from the covariance
};

/**
 * \brief Apply the spec's rendering rule.
 *
 * SUSPECT OVERRIDES EVERYTHING -- tier and PCS included. A fix the integrity check
 * distrusts must not be shown as trustworthy because it happened to arrive on the best
 * radio; a sub-metre UWB fix that failed the Mahalanobis gate is exactly the dangerous
 * case, because its tier would otherwise earn it the most confident marker on the screen.
 */
CaTblRenderClass CaTblRenderFor(const CaTblResult& r);

const char* CaTblRenderClassName(CaTblRenderClass c);

/** Search urgency (Network §4). URGENT bypasses the normal PCS escalation gate. */
enum CaTblSearchPriority : uint8_t
{
    CA_TBL_PRIORITY_ROUTINE = 0,
    CA_TBL_PRIORITY_URGENT = 1
};

/**
 * \brief One search session, possibly over several targets (Transport §2, Application §5).
 *
 * BATCH IS ONE SESSION, NOT N SESSIONS. The spec is explicit that a five-person search is
 * not five independent calls: that would flood the mesh redundantly and let each session's
 * escalation requests compete blindly. One session, one flood, one reverse-path cache, one
 * staleness clock -- and all five escalation requests land in the SAME arbiter queue,
 * which is the only configuration in which U(r) scoring does any observable work.
 */
class CaTblSession
{
  public:
    CaTblSession();

    void Open(uint32_t sessionId,
              const std::vector<uint8_t>& targets,
              uint16_t requesterId,
              CaTblSearchPriority priority,
              double nowS);

    /** Fold in a result for one of this session's targets. Ignores targets it did not ask
     *  about, so a shared flood does not corrupt an unrelated session's clock. */
    void OnResult(const CaTblResult& r, double nowS);
    //! TA-3. Set by the manager before each result.
    void SetPreciseOnGrant(bool on) { m_preciseOnGrant = on; }

    /** Advance time-driven transitions: TRACKING -> STALE -> CLOSED. Separate from
     *  OnResult because staleness is defined by the ABSENCE of results, and a transition
     *  that only ever runs when something arrives can never fire on nothing arriving. */
    void Tick(double nowS, double stalenessBoundS, double sessionTimeoutS);

    /** Requester cancelled -- typically because the subject was physically reached. */
    void Close(double nowS);

    CaTblSessionState State() const { return m_state; }
    uint32_t Id() const { return m_sessionId; }
    bool IsOpen() const { return m_state != CA_TBL_SESSION_CLOSED; }
    CaTblSearchPriority Priority() const { return m_priority; }
    const std::vector<uint8_t>& Targets() const { return m_targets; }

    /** Session-level metrics the spec asks to be reported instead of position-level ones
     *  alone: SEARCHING -> LOCATED_COARSE, and -> LOCATED_PRECISE, per target.
     *  NEGATIVE MEANS IT NEVER HAPPENED, which is a result and not a missing value --
     *  averaging a never into a mean would turn a failed search into a fast one. */
    double TimeToCoarse(uint8_t manifestId) const;
    double TimeToPrecise(uint8_t manifestId) const;

    uint64_t ResultsAccepted() const { return m_accepted; }
    uint64_t ResultsSuspect() const { return m_suspect; }
    uint64_t StaleEpisodes() const { return m_staleEpisodes; }
    uint64_t Recoveries() const { return m_recoveries; }

    /**
     * \brief Did this session EVER obtain a precise fix?
     *
     * TRACKING DOES NOT IMPLY PRECISION AND THE STATE NAME SUGGESTS IT DOES. The spec's
     * FSM recovers a STALE session to TRACKING rather than reopening it -- deliberately,
     * so a target that was found, lost and found again does not report a second "first
     * fix". A session that reached only LOCATED_COARSE, went stale, and recovered is
     * therefore in TRACKING having never once had a precise fix.
     *
     * That is correct per the FSM and misleading per the name, and it is exactly the
     * reading a viewer takes from a status line. This is the discriminator: the state
     * says what the session is doing, this says what it ever achieved.
     */
    bool EverPrecise() const { return m_everPrecise; }

    //! How the session arrived where it is, as a compact path -- "SEARCHING>COARSE>
    //! STALE>TRACKING". A final state alone cannot distinguish a session that climbed
    //! to precision from one that recovered into the same name.
    const std::string& Path() const { return m_path; }

    bool Wants(uint8_t manifestId) const;

  private:
    uint32_t m_sessionId;
    std::vector<uint8_t> m_targets;
    uint16_t m_requesterId;
    CaTblSearchPriority m_priority;
    CaTblSessionState m_state;
    double m_openedAtS;
    double m_lastResultAtS;
    std::map<uint8_t, double> m_firstCoarseS;
    std::map<uint8_t, double> m_firstPreciseS;
    uint64_t m_accepted;
    uint64_t m_suspect;
    uint64_t m_staleEpisodes;
    uint64_t m_recoveries;
    bool m_everPrecise = false;
    bool m_preciseOnGrant = false; // TA-3
    std::string m_path;
};

/**
 * \brief Opens sessions and routes results to them (Application §4 and §5).
 *
 * SEARCH_AND_LOCALIZE is the single-target case of BATCH_SEARCH, not a separate flow. The
 * spec writes them as two functions because a reader needs the single-target story first,
 * but implementing them twice would let them drift -- and the batch case is the one that
 * actually exercises arbitration, so it is the one that must not be the afterthought.
 */
class CaTblSessionManager
{
  public:
    CaTblSessionManager();

    void SetStalenessBound(double s) { m_stalenessBoundS = s; }
    void SetSessionTimeout(double s) { m_sessionTimeoutS = s; }
    /** \brief TA-3 (Transport sect 4: "ON UWB grant confirmed: session.state =
     *  LOCATED_PRECISE"). True: precision is a result whose entry carries a confirmed UWB
     *  grant, and a result from a UWB fix without one is coarse. False (default): a result
     *  from a UWB fix is precise. */
    void SetPreciseOnGrant(bool on) { m_preciseOnGrant = on; }

    /** Application §4. Returns the new session id. */
    uint32_t SearchAndLocalize(uint8_t target,
                               uint16_t requesterId,
                               CaTblSearchPriority priority,
                               double nowS);

    /** Application §5. One session, one flood, N targets. */
    uint32_t BatchSearch(const std::vector<uint8_t>& targets,
                         uint16_t requesterId,
                         CaTblSearchPriority priority,
                         double nowS);

    /** Deliver a result to every open session that asked about this target. A shared
     *  flood can legitimately answer more than one session at once. */
    void OnResult(const CaTblResult& r, double nowS);

    /** Drive time-based transitions for every open session. */
    void Tick(double nowS);

    void Close(uint32_t sessionId, double nowS);

    const std::map<uint32_t, CaTblSession>& Sessions() const { return m_sessions; }
    size_t OpenCount() const;

    /** The batch number the spec calls a headline result: the spread in time-to-precise
     *  between the first and last target of a batch. A single-target run cannot produce
     *  it, which is exactly why a single-target demo proves nothing about arbitration.
     *  Negative if fewer than two targets in that session ever reached precise. */
    double BatchPrecisionSpread(uint32_t sessionId) const;

  private:
    std::map<uint32_t, CaTblSession> m_sessions;
    bool m_preciseOnGrant = false; // TA-3
    uint32_t m_nextId;
    double m_stalenessBoundS;
    double m_sessionTimeoutS;
};

} // namespace ns3

#endif // CA_TBL_SESSION_H
