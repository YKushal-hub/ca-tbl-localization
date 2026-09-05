C4: the blocker is the PHY provider, which models exactly one field node

Instrumented the delivery path instead of guessing between the three candidates. The
answer eliminates all three and finds something further down.

MEASURED, 600 s, nTags=2, arbitration on:

  peer ESC_REQUEST  sent=162  dropDuty=0  dropCad=0  lostColl=0
  anchors           recv=661  dropDedup=0
  arbiter           granted 661, denied 0, pre-empted 0, all target 1

So the peer transmits 162 requests and loses none to the duty governor, to CAD, or to
collision -- and no anchor receives one of them. Not a single peer frame appears
anywhere on the receive side.

NOT THE THREE CANDIDATES. Duty budget, CAD survival and collision are all zero. A
fourth hypothesis, that the anchors dedup cache confuses two nodes because
NextPacketId() is a per-MAC counter starting at 0 and DeliverRx keys dedup on packet
id ALONE, is plausible and is a real latent defect -- but droppedDedup is 0, so it is
not what is happening here. Recorded because it will bite the moment two nodes do
transmit into the same cache.

THE ACTUAL CAUSE. EnginePhyProvider is constructed with a single fieldNodeId and
carries a single SetNodePosition(). It models ONE field node. A frame originating from
node 2 has no position the provider can place, so the link is never computed and no
anchor can receive it. The provider sits BELOW the MAC, so no amount of MAC-level work
reaches it.

WHAT THIS MEANS FOR THE PLAN, and it is worth more than the C4 result itself:

  - C3 is unaffected and its result stands. Queries flow anchor -> target and what was
    measured is the MEASURED tag answering and correctly ignoring. That path the
    provider does model.
  - C4 cannot be demonstrated on this substrate at all. It needs a peer whose
    transmissions propagate.
  - AND THE FULL PER-TAG REFACTOR WOULD NOT BE SUFFICIENT EITHER. Giving each tag its
    own policy, energy model and filter does not make the provider multi-node. C2-full
    is necessary and not sufficient; the provider has to carry a position per node
    before any second target can be heard.

That last point changes the shape of the remaining Chapter 12 work and was not visible
from the work packages, which scope C2 as a scenario-level refactor.

Removed the temporary diagnostic. Null control still reproduces baseline_v7
byte-for-byte.
