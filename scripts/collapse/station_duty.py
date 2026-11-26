#!/usr/bin/env python3
"""B5 (DD 50): a station's 868 MHz duty cycle when it answers N buried workers with ONE batched broadcast per cycle
(the protocol the device-side bill corresponds to, DD 35 audit note), against the access-point cap.

Time on air: Semtech AN1200.13 (LoRa modem designer's guide; cited by the earlier work as semtech2013an120013),
  T_sym = 2^SF / BW;  T_preamble = (n_preamble + 4.25) T_sym;
  n_payload = 8 + max(ceil((8 PL - 4 SF + 28 + 16 CRC - 20 IH) / (4 (SF - 2 DE))) (CR + 4), 0)
with SF 7, BW 125 kHz, CR 4/5 (CR = 1), explicit header (IH = 0), CRC on, no low-data-rate optimisation (DE = 0),
8-symbol preamble -- the main simulator's tier-1 PHY.
Cap: 10% of a trailing hour for an access point (WPC G.S.R. 853(E) Table II, as modelled in ca-tbl-mac.cc).
Payload: a 13-byte frame header (CaTblHeader) plus E bytes per worker; E has no source and is swept 4-8.
"""
import math

SF, BW, CR, PRE = 7, 125e3, 1, 8
HDR = 13
CYCLE = 2.5
CAP = 0.10


def toa(pl):
    ts = (2 ** SF) / BW
    npl = 8 + max(math.ceil((8 * pl - 4 * SF + 28 + 16 - 0) / (4 * (SF - 0))) * (CR + 4), 0)
    return (PRE + 4.25) * ts + npl * ts


def main():
    print("workers  bytes/worker  airtime_ms  duty   cap 10%")
    worst_ok = {}
    for n in (1, 7, 20, 30):
        for e in (4, 6, 8):
            pl = HDR + e * n
            frames = math.ceil(pl / 255)  # a LoRa frame carries at most 255 bytes
            t = sum(toa(min(255, pl - 255 * k)) for k in range(frames))
            duty = t / CYCLE
            print(f"{n:7d} {e:13d} {1e3 * t:11.1f} {100 * duty:5.1f}%  {'OK' if duty <= CAP else 'EXCEEDS'}")
            worst_ok.setdefault(e, []).append((n, duty <= CAP))
    for e, v in worst_ok.items():
        mx = max([n for n, ok in v if ok], default=0)
        print(f"  with {e} bytes per worker, one batched reply per cycle stays under 10% up to N = {mx} (of 1/7/20/30)")


if __name__ == "__main__":
    main()
