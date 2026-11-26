## COLLAPSE-6 with the fix: COLLAPSE-5's tests re-applied (H17-H22 rules)

storeys config  arm             L2     J_median  seeds
      5 g       ca_tbl_full     0.77      34.9      5
      5 g       poucet_fusion   0.85     469.0      5
      5 g       static_2p4      0.33       9.7      5
      5 g       static_868      0.81      15.1      5
      5 g       static_uwb      0.00     446.7      5
      5 muller  ca_tbl_full     0.66      33.6      5
      5 muller  static_2p4      0.21       9.9      5
      5 n10     ca_tbl_full     0.80      37.6      5
      5 rubble  ca_tbl_full     0.81      34.8      5
      5 st0     ca_tbl_full     0.74      40.5      5
      5 st2     ca_tbl_full     0.17      33.9      5
      8 g       ca_tbl_full     0.63      44.1      5
      8 g       poucet_fusion   0.64     488.4      5
      8 g       static_2p4      0.16      27.7      5
      8 g       static_868      0.62      13.8      5
      8 g       static_uwb      0.00     446.7      5
      8 muller  ca_tbl_full     0.55      43.1      5
      8 muller  static_2p4      0.10      29.3      5
      8 n10     ca_tbl_full     0.48      38.7      5
      8 rubble  ca_tbl_full     0.67      42.1      5
      8 st0     ca_tbl_full     0.43      44.0      5
      8 st2     ca_tbl_full     0.12      43.9      5
H17 s5: rubble 0.81 vs g 0.77 FAILS
H18 s5: L2 0.77 vs always-on 0.85; J 34.9 vs 469.0 FAILS
H19 s5: L2 0.77 vs best single static_868 0.81 HOLDS
H20 s5: stations 0/2/4 L2 0.74/0.17/0.77 FAILS
H21 s5: crew 10 L2 0.80 vs 20 0.77 HOLDS
H17 s8: rubble 0.67 vs g 0.63 FAILS
H18 s8: L2 0.63 vs always-on 0.64; J 44.1 vs 488.4 HOLDS
H19 s8: L2 0.63 vs best single static_868 0.62 HOLDS
H20 s8: stations 0/2/4 L2 0.43/0.12/0.63 FAILS
H21 s8: crew 10 L2 0.48 vs 20 0.63 FAILS
H22 s8 muller: ca_tbl_full 0.55 vs static_2p4 0.10 HOLDS

## H30: L2 with the fix minus L2 with the leak (paired seeds 821-825)

s5 g      ca_tbl_full    fix 0.77 leak 0.78 diff -0.01
s5 g      static_868     fix 0.81 leak 0.78 diff +0.03
s5 g      static_2p4     fix 0.33 leak 0.31 diff +0.02
s5 g      static_uwb     fix 0.00 leak 0.00 diff +0.00
s5 g      poucet_fusion  fix 0.85 leak 0.82 diff +0.03
s5 rubble ca_tbl_full    fix 0.81 leak 0.85 diff -0.04
s5 muller ca_tbl_full    fix 0.66 leak 0.62 diff +0.04
s5 muller static_2p4     fix 0.21 leak 0.24 diff -0.03
s5 st2    ca_tbl_full    fix 0.17 leak 0.15 diff +0.02
s5 n10    ca_tbl_full    fix 0.80 leak 0.82 diff -0.02
s8 g      ca_tbl_full    fix 0.63 leak 0.62 diff +0.01
s8 g      static_868     fix 0.62 leak 0.66 diff -0.04
s8 g      static_2p4     fix 0.16 leak 0.14 diff +0.02
s8 g      static_uwb     fix 0.00 leak 0.00 diff +0.00
s8 g      poucet_fusion  fix 0.64 leak 0.67 diff -0.03
s8 rubble ca_tbl_full    fix 0.67 leak 0.70 diff -0.03
s8 muller ca_tbl_full    fix 0.55 leak 0.51 diff +0.04
s8 muller static_2p4     fix 0.10 leak 0.09 diff +0.01
s8 st2    ca_tbl_full    fix 0.12 leak 0.09 diff +0.03
s8 n10    ca_tbl_full    fix 0.48 leak 0.50 diff -0.02

## H31: lone worker (N = 1)

H31 s5: lone worker L2 0.60 vs best single 0.80 FAILS (J 34.8, seeds 5)
H31 s8: lone worker L2 0.60 vs best single 0.80 FAILS (J 37.0, seeds 5)
