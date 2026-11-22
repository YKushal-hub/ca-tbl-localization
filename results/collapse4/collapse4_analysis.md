# COLLAPSE-4 analysis (verbatim output of scripts/collapse4_analyse.py)

```
cell                        seeds  L2    hours_med  hours_p5
s5_central_n1_base             5  0.80         25        24
s5_central_n1_frame            5  0.20        290       202
s5_central_n1_gate             5  0.40        276       266
s5_central_n20_base            5  0.69        173        39
s5_central_n20_frame           5  0.70        250       170
s5_central_n20_gate            5  0.70        247       170
s5_low_n1_base                 5  0.60         26        24
s5_low_n1_frame                5  0.20        278        93
s5_low_n1_gate                 5  0.60        278       119
s5_low_n20_base                5  0.80        242        36
s5_low_n20_frame               5  0.71        272       177
s5_low_n20_gate                5  0.84        265       193
s8_central_n1_base             5  0.60         59        26
s8_central_n1_frame            5  0.60        264       181
s8_central_n1_gate             5  0.60        275       182
s8_central_n20_base            5  0.66        179        43
s8_central_n20_frame           5  0.69        214       171
s8_central_n20_gate            5  0.69        230       172
s8_low_n1_base                 5  0.80         25        23
s8_low_n1_frame                5  0.60        277       273
s8_low_n1_gate                 5  0.40        291       262
s8_low_n20_base                5  0.76        190        46
s8_low_n20_frame               5  0.76        265       169
s8_low_n20_gate                5  0.78        266       169

HYPOTHESES
H13 s5 low n20: gate p5 193 h HOLDS
H13 s5 low n1: gate median 278 h HOLDS
H14 s5 low: gate L2 - base L2 +0.04 HOLDS
H16 s5 low: frame L2 - gate L2 -0.13 FAILS
H13 s5 central n20: gate p5 170 h HOLDS
H13 s5 central n1: gate median 276 h HOLDS
H14 s5 central: gate L2 - base L2 +0.01 HOLDS
H16 s5 central: frame L2 - gate L2 +0.00 HOLDS
H13 s8 low n20: gate p5 169 h HOLDS
H13 s8 low n1: gate median 291 h HOLDS
H14 s8 low: gate L2 - base L2 +0.02 HOLDS
H16 s8 low: frame L2 - gate L2 -0.02 HOLDS
H13 s8 central n20: gate p5 172 h HOLDS
H13 s8 central n1: gate median 275 h HOLDS
H14 s8 central: gate L2 - base L2 +0.03 HOLDS
H16 s8 central: frame L2 - gate L2 +0.00 HOLDS
H15 s5: resolved 135/600 (0.23), median |error| 0.74 m HOLDS
H15 s8: resolved 102/600 (0.17), median |error| 0.74 m HOLDS
```

Binary manifest:
```
9285aea1e044f12fe67757bfe39fbcb368c2366ad5f008e1ee9c92c73f992541  ns3.47-ca_tbl_causal_sim-optimized
98962fb0d173ad2d173e0c3d440bec5b1ec2c5c87e3ad914747f427a6ed7a315  lib/libns3.47-antenna-optimized.so
f489e55589d12bd323cf580b4a5f18e649fd1f6fa373d1cca1136923cba9bf1b  lib/libns3.47-aodv-optimized.so
a14b63ceddd211d1e977a369e564bf508c86c7b0b241a452161419a5c3a90244  lib/libns3.47-applications-optimized.so
6ece7d3e3dd92dfa27f2c9f496fdeccfeae199e5ef4815e83c32e6a7aa00b357  lib/libns3.47-bridge-optimized.so
b7f4bff9cfbd7c2396930ce068d2d7287cf6857d73d9d751d8d1e3e26bee437d  lib/libns3.47-buildings-optimized.so
007305a16593262eece86caa6eb007180e2986060b1cb0d05170942795ab38df  lib/libns3.47-config-store-optimized.so
489efdab3998b40f2a730beab7cd41237160cc8182f473182db2554bc813d02c  lib/libns3.47-core-optimized.so
459417cb3b9cd6c3fdb548418cdc4d924f77b1078a8d79916568116ed07c89dc  lib/libns3.47-csma-layout-optimized.so
be1ae905a60b77d3c14f98ede938dabbc0f55432f28c5ce40730bca905dccb4f  lib/libns3.47-csma-optimized.so
24d15e20843dd67205d042173687a5b1898f131a5c09e64d1903482dd3445997  lib/libns3.47-dsdv-optimized.so
2ad4a20b70b9f1529e74442a030aac3837adb03c8da94316e29ca618643984b1  lib/libns3.47-dsr-optimized.so
fb112e6b4c91a45fd6a1bd642f0a4e2777f3a043f54e3d3eda92d732fa113d7d  lib/libns3.47-energy-optimized.so
c0d4996048f5f85183f68855169a70c253e888b32b11c0025ce558abef1b8734  lib/libns3.47-fd-net-device-optimized.so
bc44bb4f0253280153eeae1009a0bb3e2ead3fbe655d629839be541aa2b054b4  lib/libns3.47-flow-monitor-optimized.so
d488bf18365691a043e3541a4bff32c94cd2d0f8e2e7dafa069fef87da82f0e0  lib/libns3.47-internet-apps-optimized.so
24726554be30ea45b45a23bbfa787daa1521af8253724bcf7083bd0cdddae950  lib/libns3.47-internet-optimized.so
d2b2e1279fc6bfa01695d94a7e9e1d462ef937016f44c92b47aeb7b95b5639cb  lib/libns3.47-lora-physics-engine-optimized.so
da7dd94ecc3e7c8c8cc935c79f7c01c699ada8eb608762462c795433dc676285  lib/libns3.47-lorawan-optimized.so
53db9fc850f08468a51a5cd8a21329ef906377aeb22ad705d3e8ea5d3d10abb6  lib/libns3.47-lr-wpan-optimized.so
39f44431d7dc526f59b53e5841158594b54aefddcaae393abb58a7036a2f9bd0  lib/libns3.47-lte-optimized.so
6ddefbfa29b550513f558796f91dbe9678ae91f98c7ae79acf66c636735b87ce  lib/libns3.47-mesh-optimized.so
3fd6f7d405c413fdff897265ea078c0df7dc7b730fd8b5a821c9a8a9f5224815  lib/libns3.47-mobility-optimized.so
f2fe5bfc4d5c004f39d3210bf11bbf49f659b693b23c1d7bdbe50e26d82d1da6  lib/libns3.47-netanim-optimized.so
9a4eeb2066ec2ad2b1a2b8c5c78a0e8d61d813d6a598326970abdea228b7a2c0  lib/libns3.47-network-optimized.so
2b8900b3058b6579c7e61b2a7dc38d320c6f5c9be3f59c8ff8aa15fa5fe1521d  lib/libns3.47-nix-vector-routing-optimized.so
ef3eb5adf5194210f4c23c410d11219518bf5f71a2f9cec5b8ece68503a7cc48  lib/libns3.47-olsr-optimized.so
33d7f665ca2e3b5251a5b540b7abdc749240e6f1e8f63ad11e4e50c80d17e1ab  lib/libns3.47-point-to-point-layout-optimized.so
288613f1d8736b7b1c466b55c0852e3b2c060b1aa78b6c3b3695c875f8d5141f  lib/libns3.47-point-to-point-optimized.so
d9d37ba09c5e8650b280699a9acd0472afb8c42a1811856266478587082b17a0  lib/libns3.47-propagation-optimized.so
a8aa8ff8c218c042db66b760f050f93158e2de23ae0b1e0153b42c7133a92bd2  lib/libns3.47-sixlowpan-optimized.so
b867617dacb50a62dfc35efb977b5f0a3d15169868f06478baa8c898d8941172  lib/libns3.47-spectrum-optimized.so
e5d72880b50b3b074c8186db67a6b76c6c270d5fae84efadae33af3dc77d1594  lib/libns3.47-stats-optimized.so
a6a78060161314e22d06eefdf0121c987705a5d84b0bc4fa448c9fc4b914e024  lib/libns3.47-tap-bridge-optimized.so
15f9cc362acaf41bc991ed3b45ec063c6afd0e521bedd070e805744864bfaadf  lib/libns3.47-topology-read-optimized.so
cc9646205150bd5dfc67df41b8da73f23e93016ffaf74bb8ca3ded2bb1d4185b  lib/libns3.47-traffic-control-optimized.so
98233c8bda6b66f2ea2ef18d63cbfb182b9e8dbbc3db9a30d823b9436e163193  lib/libns3.47-uan-optimized.so
659599e511fd391e1996b871fdab279326acf6c7f93297d57b49fd39e18108f1  lib/libns3.47-virtual-net-device-optimized.so
e4464b8f0d3547c09937b1fe34490ba172922110a5457bd3edda1e81b51a30de  lib/libns3.47-wifi-optimized.so
1ff41dbcba5cf0fadbdd2294cc805dd98852441c00b391df18723da9927ea44f  lib/libns3.47-zigbee-optimized.so
```
