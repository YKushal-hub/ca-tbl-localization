/*
 * CA-TBL sect 24-27 -- node lifecycle: appear, degrade, disappear, come back.
 *
 * WHY THIS IS SEPARATE FROM THE SESSION FSM, because the two look similar and conflating
 * them would be a mistake. CaTblSession tracks what a RESCUER asked for -- a search was
 * opened, is running, will be closed. This tracks what the NETWORK observes about a node,
 * which continues whether or not anyone is searching for it and is not owned by any
 * request. A node can be lost while no session exists, and a session can be open for a
 * node that was never once heard.
 *
 * THE HARD CASE IS REAPPEARANCE, and it is the reason this is a state machine rather than
 * a boolean. A node that goes quiet and comes back is NOT the same event as a node seen
 * for the first time, and treating them alike is how a search reports a fresh discovery
 * for someone it had already found and lost. The distinction has an operational
 * consequence -- a reappearance means the last known position was probably still roughly
 * right, and a first sighting means nothing was known -- so it is carried explicitly.
 *
 * SILENCE IS NOT ABSENCE. A node that stops being heard may have moved out of range, had
 * its battery die, or be lying still in a place with no path back. The network cannot
 * distinguish these and this module does not pretend to: LOST means "no contact for
 * longer than the loss bound", which is a statement about the network and not about the
 * person. Anything that reports it as a statement about the person is overreaching.
 */

#ifndef CA_TBL_NODE_LIFECYCLE_H
#define CA_TBL_NODE_LIFECYCLE_H

#include <cstdint>
#include <map>

namespace ns3
{

//! Observed state of one tracked node.
enum CaTblNodeState : uint8_t
{
    CA_TBL_NODE_UNSEEN = 0,      //!< never heard from
    CA_TBL_NODE_TRACKED = 1,     //!< heard recently
    CA_TBL_NODE_DEGRADED = 2,    //!< heard, but not recently enough to act on
    CA_TBL_NODE_LOST = 3,        //!< no contact past the loss bound
    CA_TBL_NODE_REACQUIRED = 4   //!< heard again after being LOST -- distinct from UNSEEN
};

//! Age bounds, in seconds. Ours, not cited -- see the note on CaTblFreshnessBounds.
struct CaTblLifecycleBounds
{
    double degradedS = 60.0;
    double lostS = 300.0;
    //! How long REACQUIRED persists before settling to TRACKED. It is a transient
    //! ANNOUNCEMENT, not a resting state: a consumer that missed it must not be able to
    //! sit in it forever and keep re-reporting a rediscovery.
    double reacquiredHoldS = 30.0;
};

struct CaTblNodeRecord
{
    uint16_t nodeId = 0;
    CaTblNodeState state = CA_TBL_NODE_UNSEEN;
    double lastHeardS = -1.0;
    double firstHeardS = -1.0;
    double stateSinceS = 0.0;
    uint32_t lossEvents = 0;   //!< times this node has gone LOST
    uint32_t reacquisitions = 0;
    bool everLost = false;
};

/**
 * \brief Tracks the observed lifecycle of every node the network has heard of.
 *
 * Driven by two calls: NoteContact() when a node is heard, and Tick() to advance time.
 * BOTH ARE REQUIRED. A tracker driven only by contact can never notice an absence,
 * which is the failure mode this whole module exists to catch -- and it is the same
 * shape as rule 9, that a check for a defect whose signature is ABSENCE cannot be a
 * scan of present data.
 */
class CaTblNodeLifecycle
{
  public:
    CaTblNodeLifecycle() = default;
    explicit CaTblNodeLifecycle(const CaTblLifecycleBounds& b) : m_bounds(b) {}

    //! A node was heard at \p nowS. Returns the state after the update, so a caller can
    //! act on a REACQUIRED transition at the moment it happens rather than polling.
    CaTblNodeState NoteContact(uint16_t nodeId, double nowS);

    //! Advance time without new contact. This is what detects loss.
    void Tick(double nowS);

    CaTblNodeState StateOf(uint16_t nodeId) const;
    bool Get(uint16_t nodeId, CaTblNodeRecord& out) const;
    const std::map<uint16_t, CaTblNodeRecord>& All() const { return m_nodes; }

    //! Counts across every tracked node, for a summary line.
    uint32_t CountIn(CaTblNodeState s) const;
    uint32_t TotalLossEvents() const;
    uint32_t TotalReacquisitions() const;

    static const char* StateName(CaTblNodeState s);

  private:
    CaTblLifecycleBounds m_bounds;
    std::map<uint16_t, CaTblNodeRecord> m_nodes;
};

} // namespace ns3

#endif // CA_TBL_NODE_LIFECYCLE_H
