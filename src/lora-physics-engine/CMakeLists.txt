set(source_files
    model/lora-physics-parameters.cc
    model/lora-distribution-sampler.cc
    model/lora-correlation-engine.cc
    model/lora-spatial-field-engine.cc
    model/submodules/lora-propagation-submodule.cc
    model/submodules/lora-shadowing-submodule.cc
    model/submodules/lora-fading-submodule.cc
    model/submodules/lora-nlos-submodule.cc
    model/submodules/lora-ranging-submodule.cc  # CA-TBL Phase S0b
    model/submodules/lora-atmospheric-submodule.cc
    model/submodules/lora-foliage-submodule.cc
    model/submodules/lora-interference-submodule.cc
    model/submodules/lora-hardware-submodule.cc
    model/submodules/lora-mobility-submodule.cc
    model/submodules/lora-terrain-submodule.cc
    model/submodules/lora-delay-spread-submodule.cc
    model/submodules/lora-antenna-submodule.cc
    model/submodules/lora-css-phy-submodule.cc
    model/submodules/lora-iq-impairment-submodule.cc
    model/submodules/lora-baseband-submodule.cc
    model/submodules/lora-sync-loop-submodule.cc
    model/submodules/lora-region-submodule.cc
    model/submodules/lora-building-entry-submodule.cc
    model/submodules/lora-wall-material-submodule.cc
    model/submodules/lora-floor-attenuation-submodule.cc
    model/submodules/lora-node-position-uncertainty-submodule.cc
    model/submodules/lora-temporal-blockage-submodule.cc
    model/submodules/lora-body-shadowing-submodule.cc
    model/submodules/lora-tunnel-submodule.cc
    model/submodules/lora-two-ray-ground-submodule.cc
    model/submodules/lora-soil-attenuation-submodule.cc
    model/lora-physics-engine.cc
    model/lora-2p4-physics-parameters.cc
    model/lora-2p4-physics-engine.cc
    model/lora-uwb-physics-parameters.cc
    model/lora-uwb-physics-engine.cc
    helper/lora-physics-engine-helper.cc
    helper/lora-calibration-loader.cc
    helper/lora-crosslayer-engine.cc
    helper/lora-sdr-validator.cc
    model/submodules/lora-iq-stream-submodule.cc
    model/submodules/lora-dsp-chain-submodule.cc
    model/submodules/lora-pll-submodule.cc
    model/submodules/lora-symbol-sync-submodule.cc
    model/lora-scalability-engine.cc
    helper/lora-validation-suite.cc
    model/submodules/lora-rf-nonlinearity-submodule.cc
    helper/lora-cooperative-phy-engine.cc
    model/lora-geographic-memory-engine.cc
    model/submodules/lora-microdsp-submodule.cc
    model/submodules/lora-radio-frontend-submodule.cc
    model/submodules/lora-spectrum-occupancy-submodule.cc
    helper/lora-distributed-phy-engine.cc
    model/lora-distributed-scalability-engine.cc
    helper/lora-global-validation-engine.cc
    model/lora-adaptive-environment-engine.cc
    model/ca-tbl-energy-model.cc   # CA-TBL Phase S2
    model/ca-tbl-policy.cc         # CA-TBL Phase S1
    model/ca-tbl-header.cc         # CA-TBL Phase S4
    model/ca-tbl-mac.cc            # CA-TBL Phase S4
        model/ca-tbl-relay-admission.cc # CA-TBL Phase S5 (C5)
    model/ca-tbl-session.cc        # CA-TBL Transport/Application layer
    model/ca-tbl-routing.cc        # CA-TBL Network layer 2/3
    model/ca-tbl-sparse-anchor.cc  # CA-TBL sect 7 sparse-anchor geometry
    model/ca-tbl-grant-coord.cc    # CA-TBL Network sect 6 grant coordination
    model/ca-tbl-node-lifecycle.cc # CA-TBL sect 24-27 node lifecycle
    model/ca-tbl-loocv.cc          # CA-TBL CAL-01 LOOCV weight fitting
    model/ca-tbl-tdoa.cc           # CA-TBL Physical sect 1.1 tier-1 TDoA solver
)

set(header_files
    model/lora-physics-parameters.h
    model/lora-distribution-sampler.h
    model/lora-correlation-engine.h
    model/lora-spatial-field-engine.h
    model/submodules/lora-propagation-submodule.h
    model/submodules/lora-shadowing-submodule.h
    model/submodules/lora-fading-submodule.h
    model/submodules/lora-nlos-submodule.h
    model/submodules/lora-ranging-submodule.h   # CA-TBL Phase S0b
    model/submodules/lora-atmospheric-submodule.h
    model/submodules/lora-foliage-submodule.h
    model/submodules/lora-interference-submodule.h
    model/submodules/lora-hardware-submodule.h
    model/submodules/lora-mobility-submodule.h
    model/submodules/lora-terrain-submodule.h
    model/submodules/lora-delay-spread-submodule.h
    model/submodules/lora-antenna-submodule.h
    model/submodules/lora-css-phy-submodule.h
    model/submodules/lora-iq-impairment-submodule.h
    model/submodules/lora-baseband-submodule.h
    model/submodules/lora-sync-loop-submodule.h
    model/submodules/lora-region-submodule.h
    model/submodules/lora-building-entry-submodule.h
    model/submodules/lora-wall-material-submodule.h
    model/submodules/lora-floor-attenuation-submodule.h
    model/submodules/lora-node-position-uncertainty-submodule.h
    model/submodules/lora-temporal-blockage-submodule.h
    model/submodules/lora-body-shadowing-submodule.h
    model/submodules/lora-tunnel-submodule.h
    model/submodules/lora-two-ray-ground-submodule.h
    model/submodules/lora-soil-attenuation-submodule.h
    model/lora-physics-engine.h
    model/lora-2p4-physics-parameters.h
    model/lora-2p4-physics-engine.h
    model/lora-uwb-physics-parameters.h
    model/lora-uwb-physics-engine.h
    helper/lora-physics-engine-helper.h
    helper/lora-calibration-loader.h
    helper/lora-crosslayer-engine.h
    helper/lora-sdr-validator.h
    model/submodules/lora-iq-stream-submodule.h
    model/submodules/lora-dsp-chain-submodule.h
    model/submodules/lora-pll-submodule.h
    model/submodules/lora-symbol-sync-submodule.h
    model/lora-scalability-engine.h
    helper/lora-validation-suite.h
    model/submodules/lora-rf-nonlinearity-submodule.h
    helper/lora-cooperative-phy-engine.h
    model/lora-geographic-memory-engine.h
    model/submodules/lora-microdsp-submodule.h
    model/submodules/lora-radio-frontend-submodule.h
    model/submodules/lora-spectrum-occupancy-submodule.h
    helper/lora-distributed-phy-engine.h
    model/lora-distributed-scalability-engine.h
    helper/lora-global-validation-engine.h
    model/lora-adaptive-environment-engine.h
    model/ca-tbl-energy-model.h    # CA-TBL Phase S2
    model/ca-tbl-policy.h          # CA-TBL Phase S1
    model/ca-tbl-header.h          # CA-TBL Phase S4
    model/ca-tbl-mac.h             # CA-TBL Phase S4
        model/ca-tbl-relay-admission.h  # CA-TBL Phase S5 (C5)
    model/ca-tbl-session.h         # CA-TBL Transport/Application layer
    model/ca-tbl-routing.h         # CA-TBL Network layer 2/3
    model/ca-tbl-sparse-anchor.h   # CA-TBL sect 7 sparse-anchor geometry
    model/ca-tbl-grant-coord.h     # CA-TBL Network sect 6 grant coordination
    model/ca-tbl-node-lifecycle.h  # CA-TBL sect 24-27 node lifecycle
    model/ca-tbl-loocv.h           # CA-TBL CAL-01 LOOCV weight fitting
    model/ca-tbl-tdoa.h            # CA-TBL Physical sect 1.1 tier-1 TDoA solver
)

build_lib(
  LIBNAME lora-physics-engine
  SOURCE_FILES ${source_files}
  HEADER_FILES ${header_files}
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libmobility}
    ${libpropagation}
  TEST_SOURCES
    test/lora-physics-engine-test-suite.cc
)
