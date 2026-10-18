# ACC-B2 — how this directory was made

Registered in `report/PREREG_ACC_B2_PRECISE.md` (`66338f26`) before any cell ran. Engine: the build of `6c0c3194` (`--subjectPreciseOnly`, G111). Cloud machine (g++ 13.3, -march=native, 4 cores), worktree at `66338f26` with a byte copy of the build, detached.

```
2026-10-09T00:39:17Z  (cd ~/campaigns/acc_b2 && bash scripts/acc_b2_run.sh ~/campaigns/acc_b2_out)
2026-10-09T01:05:17Z  (all cells done, runner exit 0)
```

All 80 cells exit 0; `run_id.txt` source lines:
```
     80 source 66338f26 dirty 0 profile optimized
```

Analysis: `python3 scripts/acc_b2_analyse.py <outdir>` at `66338f26`, unchanged ("Problems: 0"); `ACC_B2.md` is its output. Per cell: `arm_summary.csv`, `run_id.txt`; `raw_manifest.sha256` digests every raw file.

Frozen build (sha256, relative to the worktree):
```
2da3a607e60e6bace6bb9744d3e6a52d720f4563e6dd45e572729e34a801efd4  build/scratch/ns3.47-ca_tbl_causal_sim-optimized
9a1f705be395aff6f1aa72fe717a4db62451ba3142a29a9661003989caac4a67  build/lib/libns3.47-antenna-optimized.so
df091d6bab400c6d2c0a62dde4baba77e4adcc12158fb5310f42c29c54131116  build/lib/libns3.47-aodv-optimized.so
9e074cccee50674f012c090c9125b3181101da7370884b566c41a81d516a7ca9  build/lib/libns3.47-applications-optimized.so
0b1f0a3dabf26dce05fd3b043881dfce86b1654f1527040bc20cd7c078238f13  build/lib/libns3.47-bridge-optimized.so
d7dd301bc2557b38a31341542967486f623f17b8f7c9f5899394fe6f79edde79  build/lib/libns3.47-buildings-optimized.so
1ad509830eeefe69a96b20fa5c1f0dd6fe6ee7849316a70031a37f80f382a6d5  build/lib/libns3.47-config-store-optimized.so
b5736aa07ab37a81c210f679a62286d86779518d033f8e5a64a17d52de705be0  build/lib/libns3.47-core-optimized.so
995747e8c86546a3d780de0a31aad2f865c1470346fceabffce189058d1a2e9d  build/lib/libns3.47-csma-layout-optimized.so
002b5916d730fd5fd2749d01a21fa82bd8fdd3bf6bb9cbdc3c5323e3fbfc3e1c  build/lib/libns3.47-csma-optimized.so
45904f5d25dae1c5b9f35b36283fdf686d67da99b53ef31f1176cf0604a87e28  build/lib/libns3.47-dsdv-optimized.so
026a0f4bbb098132ba10070afae9a555828fbc73c0256c199583f5f38d00d2e1  build/lib/libns3.47-dsr-optimized.so
2a7e8f2b0666c182f055723563692e19e223120857a6bbce3021cf22c8447185  build/lib/libns3.47-energy-optimized.so
d1d9658525800f0b8d78e869aa199c6eb7c704257eee6cf428c7b45ede3ca5d3  build/lib/libns3.47-fd-net-device-optimized.so
4fd9b1c6e8e8d733dd6bead349e077171e4bed2af58fff70cf174423743a5e5d  build/lib/libns3.47-flow-monitor-optimized.so
6134731c59444622551500daa712101159faf7f4447a6b85ad83cb50d2d39181  build/lib/libns3.47-internet-apps-optimized.so
20cb9975ee2db1c5872b293ee202cc6cb5d670545c8122342b96f698e99a1983  build/lib/libns3.47-internet-optimized.so
3ab7bb2d288c0043a0e90b433eefcdd72e2095febf988342250b30fe035365e6  build/lib/libns3.47-lora-physics-engine-optimized.so
c5c8643f3be9a56c4f916eff3952214111e8bcf859e5a176f53c04eec3b20636  build/lib/libns3.47-lorawan-optimized.so
11e91009298e8d0437e4cb6b5f83b801cbffe705e76d0f5341b9353bf5e98285  build/lib/libns3.47-lr-wpan-optimized.so
8ac6a5d26935ae9bd937236e0b3e36f9815ca14034731163c8e53b850d6cac80  build/lib/libns3.47-lte-optimized.so
34b148070f225808dd27ff862239af11a18e4be1b7ab402db3e11ea06ec1bdfd  build/lib/libns3.47-mesh-optimized.so
37f44159290e610e2ee591937c1b616971130dc7e8db7c1c0e84d50a59a65378  build/lib/libns3.47-mobility-optimized.so
ec499fd969aa818b2ec7bedf230d013fa0831a6b1f68e920dec6a2a3f30efb8c  build/lib/libns3.47-netanim-optimized.so
f9f6e5839417536a4ccaaf6abb85e62a5ceb92ec81d26187bbbec221a14866f3  build/lib/libns3.47-network-optimized.so
e2abf28126b340a4e7f53b55e3b82f173189343a0abb17fdf7580246c2e6e885  build/lib/libns3.47-nix-vector-routing-optimized.so
86788e9754d6e49b5bff9250339521abeec27280125276a9f6efa7d44b5f2463  build/lib/libns3.47-olsr-optimized.so
f416336cff1d1a57a0d255a73d0a8b762cd789d96a053171af5136c205729ff8  build/lib/libns3.47-point-to-point-layout-optimized.so
5a06432eab5c89c9f233e40a9dbd51451876280545440d93778e20fe9918924a  build/lib/libns3.47-point-to-point-optimized.so
4fafbca33bda6ac4e1d6b37f359f6b9a991fef537f3dcb3274894c474c1a8f6a  build/lib/libns3.47-propagation-optimized.so
a74fae16de06f9640c4727f840847604507ebe668e7a1d7a1706afa91aa59bb4  build/lib/libns3.47-sixlowpan-optimized.so
13d2beea569d9fc6ee205646cfe26d8a8e1122831fb52e7e037247503c8c3f31  build/lib/libns3.47-spectrum-optimized.so
aae88dd37f060f4e462ccd5dfcb6dc80fd8a0eab1ee3645d79b7833422842bf2  build/lib/libns3.47-stats-optimized.so
3935cfef2f596a950e49221ec717d128f6fc26ddc72bd696d6d989aefd113327  build/lib/libns3.47-tap-bridge-optimized.so
9ab738992509b4abd216534b16d9a0932f69f9dbb10b0236bd4d2ed91b1a8085  build/lib/libns3.47-topology-read-optimized.so
fce7d7df5b6a90a27f3345fa1bb7d9af5774d2dcb2361391b1fbf238dae56f8e  build/lib/libns3.47-traffic-control-optimized.so
735e2edd887d9e4e747c2240f68177f14d5c9d0e2b24b774e1a62ae8e57a8196  build/lib/libns3.47-uan-optimized.so
228b20f53ace735aa1a3c426412fa286f42d791fccc37aee7f6a4bcad6504b0e  build/lib/libns3.47-virtual-net-device-optimized.so
fee07789be65d8ab8c58b5c0ca66c4e9ea400293a4ba7f33be053f1e6b2690e8  build/lib/libns3.47-wifi-optimized.so
04b4978496ef3ac749ffa50d14ec829f827fe30f157b88242e9be76049d40577  build/lib/libns3.47-zigbee-optimized.so
```
