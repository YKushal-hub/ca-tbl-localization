# LORA-CMP-2 — how this directory was made

Registered in report/PREREG_LORA_CMP2.md (`73b5bd3`) before any cell ran. The engine is the
build of `3d637f4` (`--arms=lora_fusion`, G88). The cells ran from a git worktree at
`73b5bd3` (~/campaigns/lora_cmp2) holding a byte copy of that build, pinned by LD_LIBRARY_PATH
(scripts/lora_cmp2_run.sh).

```
2026-10-05T05:08:44+00:00  (cd ~/campaigns/lora_cmp2 && bash scripts/lora_cmp2_run.sh /home/user/ca-tbl-localization/results/lora_cmp2_2026-10-05)
2026-10-05T05:46:54+00:00  (all 143 cells done)
```

The run was detached and uninterrupted; no cell was re-run. All 143 cells exit 0, and every
cell's `run_id.txt` reads:
```
    143 source 73b5bd31 dirty 0 profile optimized
```

**The registered reproduction check fired on the first analysis.** X_main's and X_end's
`poucet_fusion` rows differed from CAMP-FINAL's in `rmse_mlat` and `cep95_mlat` only.
The cause and the amendment are in PREREG_LORA_CMP2.md §7 (`3e40aff1`), committed before this
write-up. The amended analysis is `python3 scripts/lora_cmp2_analyse.py
results/lora_cmp2_2026-10-05 results/lora_cmp_2026-10-05 results/camp_final_2026-10-04`.
It reports "Problems: 0".

The frozen build (sha256, relative to the worktree):
```
de4896abd73a4beb21c21cfc0e1abafc1ce9390a1b4b9c6a99707bc6a096bcaf  build/scratch/ns3.47-ca_tbl_causal_sim-optimized
7393c9ce7b170c613809e2e6794c0937f2898ec5f4231420b0e09d3354f79365  build/lib/libns3.47-antenna-optimized.so
148188cce0e24eb8a994206cdbe9b44ea9d4b9efcfdb267ee56628be16f57c34  build/lib/libns3.47-aodv-optimized.so
28e7abd31652422bad5d95346e2d71f276815540274ea7107e5fd63d62fc064a  build/lib/libns3.47-applications-optimized.so
8cdd5d7e7b27a25aab0537afe4ffbc3bc0a9733158575ec5e5acae8196d85952  build/lib/libns3.47-bridge-optimized.so
347db5581b29f5462d93e71f5ad35e58998a8137355ff30450eb2318e8fe194c  build/lib/libns3.47-buildings-optimized.so
8aa3c3164d944c624c4982278f77b5799d1d0ee75ab0bd51472f34aef2fbf88d  build/lib/libns3.47-config-store-optimized.so
bfe9ba0f9705098e3611d42304ff1e11ead36440c4e8dbd97327a19210a0caca  build/lib/libns3.47-core-optimized.so
981b7762084d5322ce08871f113cd992a97387b2af271f1f904d1d68cb362832  build/lib/libns3.47-csma-layout-optimized.so
3d6f76427c99883866beeb759114bcdd527847af905c6b9a5056ac30d61dbe85  build/lib/libns3.47-csma-optimized.so
6f460735f7a2b4d892bed05f10ff46100fd140c4b237966688ce97609282428a  build/lib/libns3.47-dsdv-optimized.so
1959b142dac7098eca569e7ff7f8978a3876f882689614db67c373e2296f155b  build/lib/libns3.47-dsr-optimized.so
baf2f41bd6c9012f9b4d982f090529216282acb28632ae6161d9ea9106f9b119  build/lib/libns3.47-energy-optimized.so
ba89f941065639ee09374b35c8ad0eb9fc93a741db597a36c13a51f97d015edd  build/lib/libns3.47-fd-net-device-optimized.so
b8e3da01e943c9f0019aaafd6cc8dc87127ccd0c0a70a00699756d727e5aa3f3  build/lib/libns3.47-flow-monitor-optimized.so
0857b4af631e64f5196295c5d03ac2c48b21ff0c72691e62406afba9f251653a  build/lib/libns3.47-internet-apps-optimized.so
1f55e6759fb7c00d18b48ea1fb06862436edaeb28ddf9bb963812e32e7e07fed  build/lib/libns3.47-internet-optimized.so
8d7774d0548574272267d5f93ddc3a61fe6b1a9899420eb264cbed811ffb17b4  build/lib/libns3.47-lora-physics-engine-optimized.so
6cda96599d2d305a7d1750386a1d8a73ad7bbd7a71e7ffc538904257883a6f95  build/lib/libns3.47-lorawan-optimized.so
bd96cf3f47fd06e9b438655121c3624cc5e3e382e51fd1080a246b775e465b83  build/lib/libns3.47-lr-wpan-optimized.so
cc1f3a0fd7fa8e091c23d896a2b94d7046fd9c98f83481d9e36379c6e4a7476c  build/lib/libns3.47-lte-optimized.so
50d1ce281b5d46f49b2d3060b457addc0a0668aa88214a0b027880343e61c2de  build/lib/libns3.47-mesh-optimized.so
01c4be6e074d91f9a0d727b7270864b7b2489fedf9b46dea975c5a62d5fbe936  build/lib/libns3.47-mobility-optimized.so
ee4354fa46b1399bf8fe47cf963da7b97e938b351d5dfa2a2c86be1de8645a17  build/lib/libns3.47-netanim-optimized.so
b42558a5e2904a65a1aa9d33e0de302c9741d9fb9ad14e3dbad04d21331abfe6  build/lib/libns3.47-network-optimized.so
155ce195137a874a906c689b390439b13266787c19b8ca231b2b18a16c4a1504  build/lib/libns3.47-nix-vector-routing-optimized.so
44c204df372a6356deb806b386dae036cb0656229069e1500b2a33469c4f26cb  build/lib/libns3.47-olsr-optimized.so
fcf390a1dbc082fdacfa263b679c019d834aa3e3f8a8e172add92b1290ce625b  build/lib/libns3.47-point-to-point-layout-optimized.so
cb4f458a1606985259bb825edd669155dd28d69a8b4451def4d47fe7a04bbe0e  build/lib/libns3.47-point-to-point-optimized.so
585f192e8d939904a17c5d442b16896a70ba7fed117dc726e71d35d3c538ee71  build/lib/libns3.47-propagation-optimized.so
7ec8db489566b3b89fb14de34293b35a9688cf46d563e7336194751e49e98c21  build/lib/libns3.47-sixlowpan-optimized.so
37a26ed0fde84684c63b315b0c54744027cf81f9ff3be817fe2bb14b61817f07  build/lib/libns3.47-spectrum-optimized.so
2e21af1514ac07fa764b75d585c5854ea96f101a05dc06acc17afb619ec1b949  build/lib/libns3.47-stats-optimized.so
1392e58bb9a62129f8db1a9cc4fdf9e22e914077c962188fee52f7bd84944fb7  build/lib/libns3.47-tap-bridge-optimized.so
34a97fd8f1a9e033af94b3a3f8b6b32393ca8a4a1bb112fd22a5858191ab095f  build/lib/libns3.47-topology-read-optimized.so
da9feb80579aac55c02b8d605371f51833f9dd6b0c190782ee35ad68a095b824  build/lib/libns3.47-traffic-control-optimized.so
75a1b04a89de53474e3e0696b39d2c051454329d34a8465919d408cfd45fa4dc  build/lib/libns3.47-uan-optimized.so
b4e80007ab3a41efae4b62e84e1a4775e83d4aa87ae89e1b843dedacd3bfe67b  build/lib/libns3.47-virtual-net-device-optimized.so
d7b27caf7220384c1707ed4a68849135163f09733d324ed4c17f33977f69f5d3  build/lib/libns3.47-wifi-optimized.so
16a1a2752d3a4a2728820a4d86cd1b9770eb46db88def6884f1b6e21aa0e17c1  build/lib/libns3.47-zigbee-optimized.so
```

Tracked: this file, LORA_CMP2.md, lora_cmp2_table.csv, raw_manifest.sha256, and every cell's
`arm_summary.csv` and `run_id.txt`.

Outputs (sha256):
```
e9dbc85f6848b41f3b0a1f7527c1e49fe195d159b4a787c06bd3a51b0d5ac71b  LORA_CMP2.md
de0acf7c0485b22e32e0b195902579e145ab7b2e112be630bc3752886a328634  lora_cmp2_table.csv
48df52b2d3d909276f05b03f0ba99537b0cc036be8402c70a90188de957714a4  raw_manifest.sha256
```
