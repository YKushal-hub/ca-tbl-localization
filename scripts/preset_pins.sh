# Sourced by gate runners. The preset settings DESIGN_DECISIONS 19-26 changed on 2026-10-21, pinned back to what
# they were, so a gate written against the earlier preset tests what it was written to test. Inserted right after
# --specProfile=true: any flag the gate gives itself comes later on the command line and wins (last value wins).
# shellcheck disable=SC2034
PIN_PRE_DD19="--integrityCheck=true --imuFilter=cv --imuMagSigmaDeg=0 --ekf5FinderConsider=false --ekf5RelayGate=false --unknownStart=false --imuGyroBiasSigmaDps=0 --imuStrideSigmaPct=0 --finderPosSigma=0 --fixedVarThreshold=-1 --t2SensitivityRowKHz=0 --publishAfterStep=false --nlosRSource=operative --foliageScaleBounds=false --interferenceDistance=false --rssiRangeFromRxPower=false --pcsByteScale=1 --ekf5Window=0 --nFinders=4 --finderTasking=roundrobin --silenceStopsRelay=false --deadBatteryNoDraw=false --uwbMode=legacy --uwbSensitivityDbm=-99.3 --nGateways=3 --gwLayout=star"
