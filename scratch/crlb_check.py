import math

# CRLB for timing estimation: sigma_t = 1 / (2*pi * beta_rms * sqrt(SNR_linear))
# For LoRa chirp, uniform spectrum over bandwidth B: beta_rms = B / sqrt(12)
B = 125e3  # 125 kHz, established in this project

beta_rms = B / math.sqrt(12)
print(f"LoRa BW = {B/1e3:.0f} kHz")
print(f"beta_rms = B/sqrt(12) = {beta_rms:.1f} Hz")
print()

print("sigma_t = 1 / (2*pi * beta_rms * sqrt(SNR)):")
print(f"{'SNR_dB':>8}  {'SNR_lin':>10}  {'sigma_t_ns':>12}")
print("-"*40)
for snr_db in [10, 13, 15, 17, 18, 19, 20, 22, 25, 30, 32, 33]:
    snr_lin = 10**(snr_db/10.0)
    sigma_t_ns = 1.0 / (2*math.pi * beta_rms * math.sqrt(snr_lin)) * 1e9
    if abs(sigma_t_ns - 400) < 60:
        marker = "  <--- 400ns (engine default)"
    elif abs(sigma_t_ns - 100) < 25:
        marker = "  <--- 100ns (tuned value)"
    else:
        marker = ""
    print(f"{snr_db:>8}  {snr_lin:>10.1f}  {sigma_t_ns:>12.1f} ns{marker}")

print()
print("What SNR is required for sigma_t = 400ns?")
snr_for_400 = (1.0/(2*math.pi * beta_rms * 400e-9))**2
print(f"  SNR_lin = {snr_for_400:.1f}  ({10*math.log10(snr_for_400):.1f} dB)")

print()
print("What SNR is required for sigma_t = 100ns?")
snr_for_100 = (1.0/(2*math.pi * beta_rms * 100e-9))**2
print(f"  SNR_lin = {snr_for_100:.1f}  ({10*math.log10(snr_for_100):.1f} dB)")

print()
# LoRa SNR context
print("LoRa SF12/125kHz SNR context:")
print("  Sensitivity: -137 dBm  |  Noise floor: -174+10*log10(125000) = -123 dBm")
print("  => Demodulation SNR at sensitivity: -137-(-123) = -14 dB")
print("  => Typical working SNR for a good (not marginal) link: +5 to +20 dB relative to noise floor")
print()
for working_snr_db in [5, 10, 15, 20, 25]:
    snr_lin = 10**(working_snr_db/10.0)
    sigma_t = 1.0/(2*math.pi * beta_rms * math.sqrt(snr_lin)) * 1e9
    print(f"  Link SNR = {working_snr_db:+d} dB  ->  sigma_t = {sigma_t:.0f} ns")

print()
print("GDOP analysis: 156m/42m ratio:")
ratio = 156.0/42.0
print(f"  Observed ratio (sparse/dense): {ratio:.2f}x")
print(f"  This project: 10 gateways over 2500 km2 -> avg spacing ~{math.sqrt(2500/10):.1f} km")
print(f"  Podevijn (urban): typical gateway spacing 0.5-2 km")
print(f"  GDOP scales with baseline/area geometry -> 3-5x ratio is expected for this difference")
