#!/usr/bin/env python3
"""Phase 0 (plan): can each worker's own nRF52840 run its estimation, on its battery? (DD 43)

Inputs, each measured or sourced:
  step_us_host   host time per CaTblPolicy::Step(), --phase0Timing, one core (taskset -c 0), the run's load
  coremark_host  CoreMark on the same core under the same load (EEMBC CoreMark 1f483d5, gcc-11 -O2)
  coremark_nrf   212, nRF52840 CoreMark from flash, cache on (row NRF-CM)
  i_cpu_a        3.3 mA running CoreMark at 64 MHz from flash, DC/DC (row NRF-ICPU0)
  v              3.0 V (the same row's supply)
  cycle_s        2.5 s (the scenario's cycle)
  radio_j_per_h  the worker's radio/MAC energy per hour from the same run (subject_summary totalJ)
Unsourced: the double-precision penalty k_dp (the M4F FPU is single precision only, row NRF-FPU). Instead of
guessing it, the script reports the largest k_dp for which the plan's decision rule still passes:
  on-worker if compute <= 10% of radio energy and <= 20% of the cycle time.
"""
import argparse


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--step_us_host", type=float, required=True)
    ap.add_argument("--coremark_host", type=float, required=True)
    ap.add_argument("--coremark_nrf", type=float, default=212.0)
    ap.add_argument("--i_cpu_a", type=float, default=3.3e-3)
    ap.add_argument("--v", type=float, default=3.0)
    ap.add_argument("--cycle_s", type=float, default=2.5)
    ap.add_argument("--radio_j_per_h", type=float, required=True)
    a = ap.parse_args()
    t_nrf = a.step_us_host * 1e-6 * a.coremark_host / a.coremark_nrf   # s per step, single precision
    e_step = t_nrf * a.i_cpu_a * a.v                                     # J per step
    steps_h = 3600.0 / a.cycle_s
    e_h = e_step * steps_h
    frac_time = t_nrf / a.cycle_s
    frac_energy = e_h / a.radio_j_per_h
    k_time = 0.20 / frac_time
    k_energy = 0.10 / frac_energy
    print(f"nRF52840 time per step (k_dp = 1): {t_nrf * 1e3:.1f} ms = {100 * frac_time:.1f}% of the {a.cycle_s} s cycle")
    print(f"compute energy: {e_step * 1e3:.3f} mJ per step, {e_h:.2f} J/h = {100 * frac_energy:.1f}% of the radio's "
          f"{a.radio_j_per_h:.1f} J/h")
    print(f"decision rule passes for a double-precision penalty k_dp <= {min(k_time, k_energy):.1f} "
          f"(time limit {k_time:.1f}, energy limit {k_energy:.1f})")


if __name__ == "__main__":
    main()
