# Sourced by gate runners. The preset settings DESIGN_DECISIONS 19-26 changed on 2026-10-21, pinned back to what
# they were, so a gate written against the earlier preset tests what it was written to test. Inserted right after
# --specProfile=true: any flag the gate gives itself comes later on the command line and wins (last value wins).
# shellcheck disable=SC2034
PIN_PRE_DD19="--integrityCheck=true --imuFilter=cv --imuMagSigmaDeg=0 --ekf5FinderConsider=false --ekf5RelayGate=false --unknownStart=false --imuGyroBiasSigmaDps=0 --imuStrideSigmaPct=0 --finderPosSigma=0 --fixedVarThreshold=-1 --t2SensitivityRowKHz=0 --publishAfterStep=false --nlosRSource=operative --foliageScaleBounds=false --interferenceDistance=false --rssiRangeFromRxPower=false --pcsByteScale=1 --ekf5Window=0 --nFinders=4 --finderTasking=roundrobin --silenceStopsRelay=false --deadBatteryNoDraw=false --uwbMode=legacy --uwbSensitivityDbm=-99.3 --nGateways=3 --gwLayout=star --gwRidge=false --baseCampFrac=0 --searchLaunchOnLoss=0"
# DESIGN_DECISIONS 27 (2026-11-20) replaced DD 25-26's deployment (12 gateways on a ring, 12 finders in the field from
# t = 0) with three gateways on a line, a trailhead camp and two searchers dispatched on lost contact. A gate written
# against DD 26 pins it back with this line.
PIN_PRE_DD27="--nGateways=12 --gwLayout=legacy --gwRidge=false --baseCampFrac=0 --nFinders=12 --searchLaunchOnLoss=0"
# DD 39: the preset with ONE worker (--nTags=1 --measuredSubjects=1). The preset's relay, fix-routing, case-C and
# finder-tasking features need >= 3 tags or > 1 subject and refuse otherwise; this turns exactly those off.
PIN_ONE_WORKER="--nTags=1 --measuredSubjects=1 --identityQueryManifests=1 --relayedRanging=false --relayChain=false --subjectRelayChain=false --fixRouting=false --fixRoutingConfidence=false --caseCRelative=false --fixRoutingSubjects=false --finderTasking=tag0 --hopProvenance=false"
# DD 41 (owner decision 2026-11-22): --interferenceFrameKey is on by default. Every result recorded before that date
# ran with it off; a gate or check that reproduces one appends this pin.
PIN_PRE_DD41="--interferenceFrameKey=false"
# DD 65 (owner 2026-11-25): DD 54, 56, 57 and 59 entered the preset. A gate written against the earlier preset, or a
# campaign re-run, pins them off with this line.
PIN_PRE_DD65="--sparseRangeGeometry=false --peerSplitCi=false --sharedDuty868=false --ekf5WindowThetaWalk=false --sharedDuty868Finders=false --tdoaHeightCorrect=false --tdoaRubbleDelay=false"
# DD 67 (owner 2026-11-26): DD 60, 62 and 63 joined the preset; PIN_PRE_DD65 above pins them off too, so a gate pinned to the earlier preset is unchanged.
# A gate written against DD 65's preset (DD 54/56/57/59 on, 60/62/63 off) uses this one.
PIN_PRE_DD67="--sharedDuty868Finders=false --tdoaHeightCorrect=false --tdoaRubbleDelay=false"
