#include "ca-tbl-session.h"

#include <algorithm>

namespace ns3
{

const char*
CaTblSessionStateName(CaTblSessionState s)
{
    switch (s)
    {
    case CA_TBL_SESSION_SEARCHING:
        return "SEARCHING";
    case CA_TBL_SESSION_LOCATED_COARSE:
        return "LOCATED_COARSE";
    case CA_TBL_SESSION_LOCATED_PRECISE:
        return "LOCATED_PRECISE";
    case CA_TBL_SESSION_TRACKING:
        return "TRACKING";
    case CA_TBL_SESSION_STALE:
        return "STALE";
    default:
        return "CLOSED";
    }
}

CaTblResult::CaTblResult()
    : targetManifestId(0), x(0.0), y(0.0), varX(0.0), varY(0.0), covXY(0.0),
      tierSource(CA_TBL_TIER_868), pcs(0.0), hopCountAtFix(0), suspect(false),
      timestampS(0.0)
{
}

CaTblRenderClass
CaTblRenderFor(const CaTblResult& r)
{
    // SUSPECT FIRST, AND UNCONDITIONALLY. The spec: "render as 'position uncertain --
    // verify' regardless of tier_source or PCS". The dangerous case is precisely a
    // sub-metre UWB fix that failed the integrity gate -- its tier would otherwise earn
    // it the most confident marker on the screen, which is the one a rescuer would act on
    // without hesitating.
    if (r.suspect)
    {
        return CA_TBL_RENDER_VERIFY;
    }
    if (r.tierSource == CA_TBL_TIER_UWB)
    {
        return CA_TBL_RENDER_HIGH;
    }
    if (r.tierSource == CA_TBL_TIER_2P4)
    {
        return CA_TBL_RENDER_MEDIUM;
    }
    // 868: the ellipse is drawn from varX/varY/covXY by the consumer, not from a fixed
    // radius. Discarding the covariance at the last step would throw away the statistical
    // work every layer below spent effort producing.
    return CA_TBL_RENDER_COARSE;
}

const char*
CaTblRenderClassName(CaTblRenderClass c)
{
    switch (c)
    {
    case CA_TBL_RENDER_VERIFY:
        return "VERIFY";
    case CA_TBL_RENDER_HIGH:
        return "HIGH";
    case CA_TBL_RENDER_MEDIUM:
        return "MEDIUM";
    default:
        return "COARSE";
    }
}

// ---------------------------------------------------------------------------
// CaTblSession
// ---------------------------------------------------------------------------

CaTblSession::CaTblSession()
    : m_sessionId(0), m_requesterId(0), m_priority(CA_TBL_PRIORITY_ROUTINE),
      m_state(CA_TBL_SESSION_CLOSED), m_openedAtS(0.0), m_lastResultAtS(-1.0),
      m_accepted(0), m_suspect(0), m_staleEpisodes(0), m_recoveries(0)
{
}

void
CaTblSession::Open(uint32_t sessionId,
                   const std::vector<uint8_t>& targets,
                   uint16_t requesterId,
                   CaTblSearchPriority priority,
                   double nowS)
{
    m_sessionId = sessionId;
    m_targets = targets;
    m_requesterId = requesterId;
    m_priority = priority;
    m_state = CA_TBL_SESSION_SEARCHING;
    m_openedAtS = nowS;
    // NOT nowS. A session that has heard nothing has no last result, and seeding this
    // with the open time would make an unanswered search look merely stale rather than
    // unanswered -- the two are different outcomes and the staleness clock must not
    // conflate them.
    m_lastResultAtS = -1.0;
    m_firstCoarseS.clear();
    m_firstPreciseS.clear();
    m_accepted = 0;
    m_suspect = 0;
    m_staleEpisodes = 0;
    m_recoveries = 0;
}

bool
CaTblSession::Wants(uint8_t manifestId) const
{
    return std::find(m_targets.begin(), m_targets.end(), manifestId) != m_targets.end();
}

void
CaTblSession::OnResult(const CaTblResult& r, double nowS)
{
    if (m_state == CA_TBL_SESSION_CLOSED || !Wants(r.targetManifestId))
    {
        return;
    }

    ++m_accepted;
    m_lastResultAtS = nowS;

    // A SUSPECT FIX ADVANCES NOTHING. It is delivered to the operator -- flagged, per the
    // render rule -- but it must not move the session forward or stamp a time-to-fix. The
    // integrity check exists to say this fix should not be acted on; letting it satisfy
    // "we have located the target" would act on it at the session level while warning
    // about it at the display level, which is worse than either alone.
    if (r.suspect)
    {
        ++m_suspect;
        return;
    }

    // TA-3: under SetPreciseOnGrant, precision is a confirmed grant, not a UWB fix.
    const bool preciseNow = m_preciseOnGrant ? r.uwbGranted : (r.tierSource == CA_TBL_TIER_UWB);
    if (preciseNow)
    {
        if (m_firstPreciseS.find(r.targetManifestId) == m_firstPreciseS.end())
        {
            m_firstPreciseS[r.targetManifestId] = nowS - m_openedAtS;
        }
        // A precise fix implies a coarse one for time-to-coarse purposes: the target was
        // located, and by a better means. Without this a session whose first answer
        // arrived on UWB would report "never located coarsely", which is false.
        if (m_firstCoarseS.find(r.targetManifestId) == m_firstCoarseS.end())
        {
            m_firstCoarseS[r.targetManifestId] = nowS - m_openedAtS;
        }
    }
    else if (m_firstCoarseS.find(r.targetManifestId) == m_firstCoarseS.end())
    {
        m_firstCoarseS[r.targetManifestId] = nowS - m_openedAtS;
    }

    // ---- state advance ----------------------------------------------------
    const bool precise = preciseNow;
    const CaTblSessionState before = m_state;
    switch (m_state)
    {
    case CA_TBL_SESSION_SEARCHING:
        m_state = precise ? CA_TBL_SESSION_LOCATED_PRECISE : CA_TBL_SESSION_LOCATED_COARSE;
        break;
    case CA_TBL_SESSION_LOCATED_COARSE:
        if (precise)
        {
            m_state = CA_TBL_SESSION_LOCATED_PRECISE;
        }
        break;
    case CA_TBL_SESSION_LOCATED_PRECISE:
        // Held open past the first precise fix: the requester is now tracking rather
        // than searching. Subsequent fixes stream in as ordinary traffic.
        m_state = CA_TBL_SESSION_TRACKING;
        break;
    case CA_TBL_SESSION_STALE:
        // RECOVERS, DOES NOT REOPEN. Restarting from SEARCHING would discard the
        // escalation history and the time-to-first-fix already earned, and would report a
        // second "first fix" for a target that was never actually lost.
        m_state = CA_TBL_SESSION_TRACKING;
        ++m_recoveries;
        break;
    default:
        break;
    }

    // Record the transition, not just the destination. A session that climbed
    // SEARCHING > COARSE > PRECISE > TRACKING and one that limped
    // SEARCHING > COARSE > STALE > TRACKING both end in TRACKING and are not the same
    // event; the path is what tells them apart.
    if (m_state != before)
    {
        if (m_path.empty())
        {
            m_path = CaTblSessionStateName(before);
        }
        m_path += ">";
        m_path += CaTblSessionStateName(m_state);
    }
    if (m_state == CA_TBL_SESSION_LOCATED_PRECISE)
    {
        m_everPrecise = true;
    }
}

void
CaTblSession::Tick(double nowS, double stalenessBoundS, double sessionTimeoutS)
{
    if (m_state == CA_TBL_SESSION_CLOSED)
    {
        return;
    }

    // A session that has never heard anything ages against its OPEN time. Otherwise an
    // unanswered search would sit in SEARCHING forever and never time out, which is the
    // one outcome an operator most needs told.
    const double since = (m_lastResultAtS >= 0.0) ? (nowS - m_lastResultAtS)
                                                  : (nowS - m_openedAtS);

    // TIMEOUT FIRST, AND UNCONDITIONALLY. Checking it only once the session was
    // already STALE made the terminal state depend on how often Tick was called
    // rather than on how long the silence had lasted: a session polled once an
    // hour would sit in STALE forever, while one polled every second closed
    // correctly. Silence longer than the timeout closes the session from
    // whatever state it was in.
    if (since > sessionTimeoutS)
    {
        m_state = CA_TBL_SESSION_CLOSED;
        return;
    }

    // A session that has heard nothing at all only ever closes -- there is no
    // meaningful 'stale' for a fix that never existed.
    if (m_state == CA_TBL_SESSION_SEARCHING || m_state == CA_TBL_SESSION_STALE)
    {
        return;
    }

    if (since > stalenessBoundS)
    {
        m_state = CA_TBL_SESSION_STALE;
        ++m_staleEpisodes;
    }
}

void
CaTblSession::Close(double)
{
    m_state = CA_TBL_SESSION_CLOSED;
}

double
CaTblSession::TimeToCoarse(uint8_t manifestId) const
{
    std::map<uint8_t, double>::const_iterator it = m_firstCoarseS.find(manifestId);
    return (it == m_firstCoarseS.end()) ? -1.0 : it->second;
}

double
CaTblSession::TimeToPrecise(uint8_t manifestId) const
{
    std::map<uint8_t, double>::const_iterator it = m_firstPreciseS.find(manifestId);
    return (it == m_firstPreciseS.end()) ? -1.0 : it->second;
}

// ---------------------------------------------------------------------------
// CaTblSessionManager
// ---------------------------------------------------------------------------

CaTblSessionManager::CaTblSessionManager()
    : m_nextId(1), m_stalenessBoundS(120.0), m_sessionTimeoutS(600.0)
{
}

uint32_t
CaTblSessionManager::SearchAndLocalize(uint8_t target,
                                       uint16_t requesterId,
                                       CaTblSearchPriority priority,
                                       double nowS)
{
    // The single-target case IS the batch case. Implementing it twice would let the two
    // drift, and the batch path is the one that exercises arbitration, so it must not be
    // the afterthought.
    std::vector<uint8_t> one(1, target);
    return BatchSearch(one, requesterId, priority, nowS);
}

uint32_t
CaTblSessionManager::BatchSearch(const std::vector<uint8_t>& targets,
                                 uint16_t requesterId,
                                 CaTblSearchPriority priority,
                                 double nowS)
{
    const uint32_t id = m_nextId++;
    CaTblSession s;
    s.Open(id, targets, requesterId, priority, nowS);
    m_sessions[id] = s;
    return id;
}

void
CaTblSessionManager::OnResult(const CaTblResult& r, double nowS)
{
    // Every open session that asked about this target, not just the first. A single flood
    // can legitimately answer more than one outstanding search, and dropping the rest
    // would make a second search look slower than it was.
    for (std::map<uint32_t, CaTblSession>::iterator it = m_sessions.begin();
         it != m_sessions.end(); ++it)
    {
        it->second.SetPreciseOnGrant(m_preciseOnGrant);
        it->second.OnResult(r, nowS);
    }
}

void
CaTblSessionManager::Tick(double nowS)
{
    for (std::map<uint32_t, CaTblSession>::iterator it = m_sessions.begin();
         it != m_sessions.end(); ++it)
    {
        it->second.Tick(nowS, m_stalenessBoundS, m_sessionTimeoutS);
    }
}

void
CaTblSessionManager::Close(uint32_t sessionId, double nowS)
{
    std::map<uint32_t, CaTblSession>::iterator it = m_sessions.find(sessionId);
    if (it != m_sessions.end())
    {
        it->second.Close(nowS);
    }
}

size_t
CaTblSessionManager::OpenCount() const
{
    size_t n = 0;
    for (std::map<uint32_t, CaTblSession>::const_iterator it = m_sessions.begin();
         it != m_sessions.end(); ++it)
    {
        n += it->second.IsOpen() ? 1 : 0;
    }
    return n;
}

double
CaTblSessionManager::BatchPrecisionSpread(uint32_t sessionId) const
{
    std::map<uint32_t, CaTblSession>::const_iterator it = m_sessions.find(sessionId);
    if (it == m_sessions.end())
    {
        return -1.0;
    }
    // The spread between the first and last target to reach precise. This is the number
    // the spec calls a headline result, and it is the direct evidence that arbitration
    // orders targets rather than serving them first-come-first-served. A single-target
    // session cannot produce it, which is exactly why a single-target demo proves nothing
    // about arbitration.
    double lo = -1.0, hi = -1.0;
    int n = 0;
    const std::vector<uint8_t>& targets = it->second.Targets();
    for (size_t i = 0; i < targets.size(); ++i)
    {
        const double t = it->second.TimeToPrecise(targets[i]);
        if (t < 0.0)
        {
            continue;
        }
        if (n == 0 || t < lo)
        {
            lo = t;
        }
        if (n == 0 || t > hi)
        {
            hi = t;
        }
        ++n;
    }
    return (n >= 2) ? (hi - lo) : -1.0;
}

} // namespace ns3
