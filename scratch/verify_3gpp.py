import subprocess
import re
import numpy as np
import os

def get_mean_rssi_at_distance(distance_m):
    cmd = [
        './ns3', 'run', 'scratch/lora_physics_engine',
        '--',
        '--env=3',  # 3 = ENV_URBAN_MODERATE
        '--weather=0',
        '--distance=' + str(distance_m),
        '--txPower=14.0'
    ]
    # ns-3 outputs to stdout and stderr, we capture both
    result = subprocess.run(cmd, capture_output=True, text=True)
    out = result.stdout + result.stderr
    rssi_matches = re.findall(r'RSSI:\s*([-\d.]+)\s*dBm', out)
    
    # Wait, the runner might output RX power instead of RSSI, let's check
    if not rssi_matches:
        rssi_matches = re.findall(r'RxPowerDbm\s*[:=]\s*([-\d.]+)', out)
        
    if not rssi_matches:
        # Also check for SNR to see if there is any output
        return None
    rssis = [float(r) for r in rssi_matches]
    return np.mean(rssis)

print("=== 3GPP UMa RSSI Validation ===")
print("Distance    Engine RSSI (dBm)    Rademacher Target")
for d, target in [(100, -102.6), (200, -107.4), (500, -113.7), (1000, -118.4), (2000, -123.2)]:
    engine_rssi = get_mean_rssi_at_distance(d)
    if engine_rssi is not None:
        gap = engine_rssi - target
        print(f"{d:4d}m      {engine_rssi:17.1f}   {target:17.1f}   (Gap: {gap:+.1f} dB)")
    else:
        print(f"{d:4d}m      ERROR (No output)")
