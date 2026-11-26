# COLLAPSE-11

| cell | arm | L2 off | L2 on | on - off |
|---|---|---|---|---|
| s5 g | ca_tbl_full | 0.910 | 0.925 | 0.015 [-0.010, 0.045] |
| s5 g | static_868 | 0.955 | 0.955 | 0.000 [0.000, 0.000] |
| s5 g | static_2p4 | 0.350 | 0.355 | 0.005 [-0.020, 0.030] |
| s5 g | static_uwb | 0.015 | 0.015 | 0.000 [0.000, 0.000] |
| s5 g | poucet_fusion | 0.920 | 0.965 | 0.045 [0.005, 0.080] |
| s5 muller | ca_tbl_full | 0.740 | 0.790 | 0.050 [0.025, 0.080] |
| s5 muller | static_2p4 | 0.240 | 0.285 | 0.045 [0.020, 0.070] |
| s5 st2 | ca_tbl_full | 0.150 | 0.305 | 0.155 [0.100, 0.205] |
| s5 n10 | ca_tbl_full | 0.870 | 0.890 | 0.020 [-0.040, 0.080] |
| s5 n1 | ca_tbl_full | 0.700 | 0.500 | -0.200 [-0.500, 0.000] |
| s5 n1 | static_868 | 0.900 | 0.900 | 0.000 [0.000, 0.000] |
| s5 n1 | static_2p4 | 0.300 | 0.300 | 0.000 [0.000, 0.000] |
| s8 g | ca_tbl_full | 0.730 | 0.725 | -0.005 [-0.045, 0.035] |
| s8 g | static_868 | 0.760 | 0.760 | 0.000 [0.000, 0.000] |
| s8 g | static_2p4 | 0.215 | 0.215 | 0.000 [-0.035, 0.035] |
| s8 g | static_uwb | 0.015 | 0.015 | 0.000 [0.000, 0.000] |
| s8 g | poucet_fusion | 0.725 | 0.740 | 0.015 [-0.030, 0.050] |
| s8 muller | ca_tbl_full | 0.635 | 0.645 | 0.010 [-0.025, 0.045] |
| s8 muller | static_2p4 | 0.140 | 0.155 | 0.015 [0.000, 0.030] |
| s8 st2 | ca_tbl_full | 0.140 | 0.175 | 0.035 [0.005, 0.070] |
| s8 n10 | ca_tbl_full | 0.750 | 0.790 | 0.040 [-0.010, 0.100] |
| s8 n1 | ca_tbl_full | 0.900 | 0.900 | 0.000 [0.000, 0.000] |
| s8 n1 | static_868 | 0.800 | 0.800 | 0.000 [0.000, 0.000] |
| s8 n1 | static_2p4 | 0.300 | 0.300 | 0.000 [0.000, 0.000] |

**H44: HOLDS** (every crew cell: lower end of ours on - off >= -0.05)

| cell | device peaks > 2.5 % off | on | largest off | on |
|---|---|---|---|---|
| s5 g | 0/220 | 0/220 | 0.02488 | 0.02488 |
| s5 muller | 0/220 | 0/220 | 0.02488 | 0.02487 |
| s5 st2 | 0/220 | 0/220 | 0.02488 | 0.02488 |
| s5 n10 | 0/120 | 0/120 | 0.02488 | 0.02488 |
| s5 n1 | 0/30 | 0/30 | 0.01884 | 0.01941 |
| s8 g | 0/220 | 0/220 | 0.02488 | 0.02488 |
| s8 muller | 0/220 | 0/220 | 0.02488 | 0.02488 |
| s8 st2 | 0/220 | 0/220 | 0.02487 | 0.02488 |
| s8 n10 | 0/120 | 0/120 | 0.02488 | 0.02487 |
| s8 n1 | 0/30 | 0/30 | 0.01803 | 0.01803 |

**H45: HOLDS** (off and on: every worker's and searcher's device's audited trailing-hour 868 MHz airtime <= 2.5 %)

s5 g on: ours - static_868 L2 -0.030 [-0.075, 0.015]
s8 g on: ours - static_868 L2 -0.035 [-0.075, 0.010]

**H46: FAILS** (four stations, 20 workers: lower end of ours - best single radio >= -0.05)

| cell | coverage off | on | on - off |
|---|---|---|---|
| s5 g | 0.796 | 0.859 | 0.063 [0.044, 0.080] |
| s5 muller | 0.703 | 0.753 | 0.050 [0.029, 0.070] |
| s5 st2 | 0.518 | 0.655 | 0.138 [0.086, 0.186] |
| s5 n10 | 0.809 | 0.855 | 0.046 [-0.007, 0.094] |
| s5 n1 | 0.671 | 0.804 | 0.133 [-0.003, 0.326] |
| s8 g | 0.825 | 0.853 | 0.028 [0.002, 0.052] |
| s8 muller | 0.748 | 0.764 | 0.016 [-0.006, 0.038] |
| s8 st2 | 0.599 | 0.622 | 0.023 [-0.011, 0.066] |
| s8 n10 | 0.834 | 0.878 | 0.044 [0.017, 0.067] |
| s8 n1 | 0.922 | 0.949 | 0.026 [-0.007, 0.082] |

**H47: HOLDS** (coverage on - off: lower end >= -0.02 in every crew cell, > 0 at 5 storeys with four and with two stations)

**H48: HOLDS** (on, four stations, 20 workers: mean coverage >= 0.85 at both storey counts)


Problems: 2
- /home/nethi/catbl_work/campaigns/collapse11/s8_n1_on/seed1702: the DD49 line does not match 'on'
- /home/nethi/catbl_work/campaigns/collapse11/s8_n1_on/seed1703: the DD49 line does not match 'on'
