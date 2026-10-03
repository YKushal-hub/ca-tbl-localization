# GEO-01 stage 2 — generated table (scripts/geo_stage2_analyse.py)

Criterion (ca_tbl_full): mean availability >= 0.95 and median tFirst100m <= 600 s (never reached = 1800 s).

| candidate | arm | seeds | availability (mean) | tFirst100m median (s) | CEP50 / CEP95 mean (m) | first UWB contact median (s) | err <= 25 m | subject J |
|---|---|---|---|---|---|---|---|---|
| N3_cluster | ca_tbl_full | 10 | 0.224 | 8 | 18.3 / 44.4 | 1800 | 0.675 | 14.08 |
| N3_cluster | static_868 | 10 | 0.263 | 9 | 157.3 / 306.8 | 1800 | 0.218 | 8.25 |
| N3_star | ca_tbl_full | 10 | 0.245 | 8 | 17.7 / 43.0 | 1800 | 0.685 | 13.81 |
| N3_star | static_868 | 10 | 0.280 | 8 | 151.7 / 256.7 | 1800 | 0.339 | 8.18 |
| N4_arc90 | ca_tbl_full | 10 | 0.273 | 8 | 16.7 / 41.0 | 1800 | 0.713 | 13.73 |
| N4_arc90 | static_868 | 10 | 0.312 | 8 | 27.6 / 48.2 | 1800 | 0.472 | 8.17 |
| N4_star | ca_tbl_full | 10 | 0.250 | 8 | 57.4 / 93.4 | 1800 | 0.618 | 14.11 |
| N4_star | static_868 | 10 | 0.283 | 9 | 35.7 / 60.7 | 1800 | 0.269 | 8.20 |
| N5_arc140 | ca_tbl_full | 10 | 0.292 | 6 | 19.0 / 49.7 | 1800 | 0.708 | 14.05 |
| N5_arc140 | static_868 | 10 | 0.343 | 5 | 22.3 / 42.2 | 1800 | 0.625 | 8.18 |
| N5_arc90 | ca_tbl_full | 10 | 0.291 | 6 | 16.0 / 37.0 | 1800 | 0.766 | 14.53 |
| N5_arc90 | static_868 | 10 | 0.342 | 5 | 27.5 / 52.4 | 1800 | 0.535 | 8.16 |
| N6_arc180 | ca_tbl_full | 10 | 0.306 | 5 | 16.6 / 39.7 | 1800 | 0.722 | 14.51 |
| N6_arc180 | static_868 | 10 | 0.351 | 5 | 16.3 / 33.0 | 1800 | 0.774 | 8.17 |
| N6_arc240 | ca_tbl_full | 10 | 0.306 | 8 | 14.8 / 36.0 | 1800 | 0.764 | 14.56 |
| N6_arc240 | static_868 | 10 | 0.356 | 5 | 15.2 / 29.9 | 1800 | 0.819 | 8.14 |
| N8_arc240 | ca_tbl_full | 10 | 0.322 | 8 | 13.6 / 31.3 | 1800 | 0.798 | 14.98 |
| N8_arc240 | static_868 | 10 | 0.377 | 5 | 14.4 / 27.6 | 1800 | 0.824 | 8.16 |
| N8_ring | ca_tbl_full | 10 | 0.329 | 8 | 14.5 / 29.8 | 1800 | 0.808 | 14.55 |
| N8_ring | static_868 | 10 | 0.384 | 8 | 12.9 / 22.1 | 1800 | 0.873 | 8.06 |

**No candidate meets the criterion** (availability >= 0.95 and median tFirst100m <= 600 s). Registered fallback: the highest mean availability, N8_ring (0.329, CEP95 29.8 m); runner-up N8_arc240.

Abort checks: 0 problems (none)
