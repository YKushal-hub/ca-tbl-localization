import pandas as pd
import numpy as np

df = pd.read_csv("results/multimodal_M3_TMESH_D25.csv", nrows=200000)

hok_rows = df[df["Stage1HandoffTimeS"].notna()]
act_rows = df[df["Stage2Modality"] != "NONE"]

print("=== s2ok gate analysis: M3 MESH D25 (first 200k rows) ===")
print(f"Total rows: {len(df):,}")
print(f"hok rows (Stage1HandoffTimeS not NaN): {len(hok_rows):,}  ({len(hok_rows)/len(df)*100:.1f}%)")
print(f"act rows (Stage2Modality != NONE):     {len(act_rows):,}  ({len(act_rows)/len(df)*100:.1f}%)")
print()
if len(act_rows) > 0:
    nlos_frac = act_rows["Stage2IsNLOS"].mean()
    print(f"Stage-2 LOS fraction in fused rows: {(1-nlos_frac)*100:.1f}%  NLOS: {nlos_frac*100:.1f}%")
print()
print("s2ok definition (from code):")
print("  RTT (M1/M2): ok = rssi > RF24_SENS_DBM  (RSSI threshold only, no precision gate)")
print("  UWB (M3/M4): ok = dist <= 30m NLOS or <= 100m LOS  (distance ceiling only)")
print()
print("Both are COVERAGE gates, not PRECISION gates.")
print("'UWB precision -> more handoff success' narrative is incorrect.")
print("Correct: UWB ceiling vs RTT RSSI threshold yields different coverage at different distances.")
print()

# M2=M4 at D25 investigation
print("=== M2=M4 D25 median convergence ===")
print("From report: M2 MESH D25 median=0.50m, M4 MESH D25 median=0.50m")
print()
print("Mechanism: IMU rescue makes total 'tiny-fpe fraction' = hok-eligible fraction for both.")
print("  M2: fused_RTT=50.5%, IMU_only=hok%-fused%=~5.4%  -> total tiny=~55.9%")
print("  M4: fused_UWB=37.1%, IMU_only=hok%-fused%=~18.8% -> total tiny=~55.9%")
print("Both have same total tiny fraction (hok rate), P50 lands at same IMU-drift quantile.")
print()

# CRLB summary
print("=== CRLB summary ===")
import math
B = 125e3
beta_rms = B / math.sqrt(12)
snr_400 = (1.0/(2*math.pi*beta_rms*400e-9))**2
snr_100 = (1.0/(2*math.pi*beta_rms*100e-9))**2
print(f"sigma_t=400ns -> SNR required: {snr_400:.1f} linear = {10*math.log10(snr_400):.1f} dB")
print(f"sigma_t=100ns -> SNR required: {snr_100:.1f} linear = {10*math.log10(snr_100):.1f} dB")
print()
print("LoRa SF12/125kHz is designed for links at 0 to +10 dB above sensitivity.")
print("20 dB SNR is a generous but plausible mid-range link (not a marginal link).")
print("33 dB SNR would be exceptional -- very short range, strong LOS, 40+ dB link margin.")
print("Using SF12 specifically BECAUSE you're at low SNR. 33 dB negates the purpose.")
print()

# Geometry comparison
print("=== Geometry: why 156m != 42m ===")
print("This project : 10 gateways / 2500 km^2 -> avg spacing sqrt(2500/10) = 15.8 km")
print("Podevijn ref : urban testbed, typical spacing 0.5-2 km")
ratio = 15.8 / 1.0  # rough midpoint
print(f"Geometry ratio: {ratio:.0f}x sparser baseline -> GDOP ratio scales accordingly")
print(f"Observed error ratio: 156/42 = {156/42:.2f}x -- within expected range for this geometry difference")
print()
print("CONCLUSION: 156m is very likely the correct answer for this sparse deployment.")
print("42.43m/11.18m are external literature benchmarks for dense urban deployments,")
print("hardcoded in check_tdoa_converged.py without derivation from this project's geometry.")
print("Tuning sigma_t to reproduce them is the same shape of move as the 30dBm Tx power fix.")
