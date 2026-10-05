# GEO-01 stage 1 — how this directory was made

```
GEO-01 stage 1, registered in report/PREREG_GEO01_STAGE1.md (cc058eb). Run from a clean tree at cc058eb, 2026-10-02T14:40:55Z.
build/scratch/ns3.47-ca_tbl_causal_sim-optimized --probe=true --seed=7 --boxHalf=5000 --outdir=results/geo01_stage1_2026-10-02/probe
python3 scripts/geo_optimise.py --probe results/geo01_stage1_2026-10-02/probe/probe_range_sweep.csv --out results/geo01_stage1_2026-10-02/opt --boxHalf 5000 --N 3,4,5,6,8 --lattice 200
python3 scripts/geo_screen.py --bin build/scratch/ns3.47-ca_tbl_causal_sim-optimized --probe results/geo01_stage1_2026-10-02/probe/probe_range_sweep.csv --out results/geo01_stage1_2026-10-02 --boxHalf 5000 --lattice 50 --N 3,4,5,6,8 --files results/geo01_stage1_2026-10-02/opt/geo_opt_realistic_N3.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_realistic_N4.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_realistic_N5.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_realistic_N6.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_realistic_N8.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_unrestricted_N3.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_unrestricted_N4.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_unrestricted_N5.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_unrestricted_N6.csv,results/geo01_stage1_2026-10-02/opt/geo_opt_unrestricted_N8.csv

Outputs (sha256), written after the last run closed:
6ce7afb0654b0495ab309a3da778da4d82bd90e4ecb591f06f6673c4a0e916eb  ./geo_screen.csv
7428c6901586b2e4073b456a11b60e2a66983c1e46a2d1ae9c6f0aa23c47b87c  ./geo_screen.md
4c5f381171f39623e94951250e2873163cfc33de5ac7899a154138cc5b51020c  ./opt/geo_opt_realistic_N3.csv
06bbdc9158509247d2603eb09e37a5a2d4981b5e9ecd43f0fb69bf205f846ddb  ./opt/geo_opt_realistic_N4.csv
cbd1fbdd7e19c8d711350612979cdf07ec23c0753ff21c4ad4ff17beff3a38a0  ./opt/geo_opt_realistic_N5.csv
aff1df777de2b3572e273d087b9abed5c945db1245bd2f4faaa49ab6ae839884  ./opt/geo_opt_realistic_N6.csv
2d67ab0cf84a8dd675b92050b322f346a445df93055afce0d303c492fa362ec9  ./opt/geo_opt_realistic_N8.csv
d699f2e08193d8c1d9b6e762100435674b01b61940a35bc95e8b8c36d5dec4c8  ./opt/geo_opt_unrestricted_N3.csv
04dcaeb54b48b9d21327c6d99851e28290617705331ee682856fe0aec58ef250  ./opt/geo_opt_unrestricted_N4.csv
2755d716dc9e821020d5e62ecf4eefaaacec89576e160359c59f2afa2f23df10  ./opt/geo_opt_unrestricted_N5.csv
703b4783dff8a01ead49491866007a4b708d31ae28a59f6b55b1c6d11ad53958  ./opt/geo_opt_unrestricted_N6.csv
677f297c514964defe5ce96305f69d8162072aa899da71dea7aa1a94f99fba22  ./opt/geo_opt_unrestricted_N8.csv
70b914d6c90e3ec9f7714c47db58c34a14d21809cd2c1ff95a97ff8d021bb38f  ./opt/optimise.log
402ae7ead88e31f5580db89aaae874646ab0f627226ea0690ac18d788024dee2  ./probe/probe.log
8c0c3b6d9f563754b755b7901303672bfb5d57c477a6b284e2492a9a63b69a80  ./probe/probe_range_sweep.csv
e4198afdb624604672a050736e3a6fbfb648c7db7e950d097d5593c69f58366a  ./probe/run_id.txt
3253545c6aa51542c3e5e8d5bc6bb1e13fddd2dc936f623d4370273a3d28d292  ./screen.log
```

Only the summaries, provenance and Markdown are tracked here (.gitignore: /results/**). geo_screen.csv, the optimiser CSVs and the logs regenerate from the commands above; the optimiser is seeded (2026), so its placements reproduce. They are, for stage 2's --gwLayout=file:

- `geo_opt_realistic_N3.csv`: 250.0,4500.0 0.0,4500.0 -250.0,4500.0 
- `geo_opt_realistic_N4.csv`: 4500.0,250.0 4500.0,0.0 4500.0,-250.0 4500.0,-500.0 
- `geo_opt_realistic_N5.csv`: -750.0,4500.0 -1000.0,4500.0 -500.0,4500.0 -4500.0,1250.0 -4500.0,1000.0 
- `geo_opt_realistic_N6.csv`: 4500.0,250.0 250.0,4500.0 -250.0,4500.0 4500.0,-250.0 0.0,4500.0 4500.0,0.0 
- `geo_opt_realistic_N8.csv`: 250.0,-4500.0 -4500.0,250.0 4500.0,250.0 4500.0,0.0 4500.0,500.0 -4500.0,500.0 0.0,-4500.0 -4500.0,0.0 
- `geo_opt_unrestricted_N3.csv`: 250.0,250.0 -250.0,-250.0 250.0,-250.0 
- `geo_opt_unrestricted_N4.csv`: 250.0,250.0 -250.0,250.0 250.0,-250.0 -250.0,-250.0 
- `geo_opt_unrestricted_N5.csv`: -750.0,250.0 -250.0,250.0 250.0,250.0 250.0,-250.0 250.0,-750.0 
- `geo_opt_unrestricted_N6.csv`: 1250.0,-250.0 -1250.0,750.0 -250.0,1250.0 -750.0,-750.0 -250.0,-1250.0 1250.0,250.0 
- `geo_opt_unrestricted_N8.csv`: 1750.0,1750.0 -1750.0,1750.0 -1750.0,1250.0 1750.0,-1750.0 -1750.0,-1750.0 -1250.0,-1750.0 1250.0,1750.0 1750.0,-1250.0 
