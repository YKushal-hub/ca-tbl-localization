# V6-ONDEMAND results — the on-demand rule is now calibrated, and there is almost nothing for it to save (registered, exploratory)

Registration: `report/PREREG_V6_ONDEMAND.md` at `95b79351`, committed before any cell. 120 runs, all `rc 0`,
`source 95b79351 dirty 0`, one compile stamp, frozen binary with a SHA-256 manifest. Tracked per cell:
`arm_summary.csv`, `run_id.txt` (`results/v6_2026-10-16/`).

## 1. Verdicts
| # | prediction | outcome |
|---|---|---|
| V6-1 | A: calibrated predictor within 0.05 of observed success (attempted cycles after 300 s) | **held**: 0.434 against 0.404 (n 10 692) |
| V6-2 | CAL energy saving under 5 % in both configs | **held**: A +0.42 % [−0.54, +1.60]; B −0.37 % [−2.20, +0.75] |
| V6-3 | CAL CEP95 within 25 % in both configs | **held**: A +0.9 %; B −1.6 % |
| V6-4 | B: raw rule sleeps on under 2 % of cycles | **held**: 0.000 |

## 2. Reading
- **B4-3's "on-demand never sleeps" was correct behaviour in the endgame**, not a defect. 97 % of 2.4 GHz attempts
  there succeed (dev reliability), so waking the radio is right.
- **At 5 km the defect was the predictor**, which over-stated success. Online Platt recalibration fixes it (gap 0.03).
  The calibrated rule then skips the hopeless decided cycles, but those are a small part of tag 0's energy.
- **The energy lever is elsewhere.** 2.4 GHz is 22–44 % of `ca_tbl_full`'s tag-0 energy. In the endgame, UWB
  (13 J of 25.7 J in dev) and the dual-base 868 MHz slot dominate. A rule that saves energy has to act on those
  (master plan v3 V9), not on the 2.4 GHz radio.
- **Decision:** `--onDemandCalibrate` is kept, default off; if on-demand is used at all, it should be the calibrated
  form. `--traceOnDemand` is the instrument for any future rule.

## 3. Analysis output (`scripts/v6_analyse.py`)
# V6-ONDEMAND results (exploratory)

| config | variant | tag-0 J | CEP95 (m) | 2.4 attempted share | sleeps (share of decided) |
|---|---|---|---|---|---|
| A | ALWAYS | 16.01 | 841.7 | 1.000 | 0.0000 |
| A | RAW | 15.99 | 842.1 | 0.999 | 0.0181 |
| A | CAL | 15.89 | 847.1 | 0.959 | 0.3426 |
| B | ALWAYS | 24.00 | 14.9 | 1.000 | 0.0000 |
| B | RAW | 24.00 | 14.9 | 1.000 | 0.0000 |
| B | CAL | 24.09 | 14.0 | 0.994 | 0.0083 |

A: CAL energy saving +0.42 % [-0.54, +1.60]; CEP95 change +0.9 % [-1.1, +3.2]

B: CAL energy saving -0.37 % [-2.20, +0.75]; CEP95 change -1.6 % [-9.9, +5.2]

V6-1 data: A CAL attempted cycles after 300 s: n 10692, mean pCal 0.434, observed 0.404

## Predictions

- **V6-1**: held (gap 0.030)
- **V6-2**: held
- **V6-3**: held
- **V6-4**: held (0.0000)

