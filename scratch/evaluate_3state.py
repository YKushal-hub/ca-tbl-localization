import sys, time
import numpy as np
import pandas as pd

def ekf_fuse(lora_err, s2_err, tsf):
    imu = np.maximum(0.00294 * (tsf ** 1.5) / 1.732, 0.5)
    rl  = np.maximum(lora_err, 1.0)
    ok2 = ~np.isnan(s2_err)
    rs2 = np.where(ok2, np.abs(s2_err) + 0.3, 1e6)
    vi = imu**2; vl = rl**2; vs = rs2**2
    vli = (vi * vl) / (vi + vl + 1e-9)
    v3  = (vli * vs) / (vli + vs + 1e-9)
    return np.sqrt(np.where(ok2, v3, vli))

def evaluate_metrics():
    print("Loading tdoa_processed_dataset_fixed.csv...")
    df = pd.read_csv("results/tdoa_processed_dataset_fixed.csv")
    
    rng_v = np.random.default_rng(12345)
    df["v_facility"] = rng_v.uniform(0, 1, size=len(df))
    
    rng_s2 = np.random.default_rng(67890)
    
    s2_data = {}
    for lam in [25, 50, 100]:
        dist = df[f"Dist{lam}"].values
        nlos = df[f"Nlos{lam}"].values
        
        pl = 40.05 + 10.0 * 2.75 * np.log10(np.maximum(dist, 1.0)) + np.where(nlos > 0, nlos * 1.3, 0.0)
        rssi = 8.0 - pl
        ok_24 = rssi > -90.0
        mg = rssi - -90.0
        std_24 = np.where(mg > 10, 0.5, np.maximum(0.3, 2.0 - mg * 0.15))
        noise_24 = rng_s2.normal(0, std_24)
        re_24 = np.where(ok_24, dist + noise_24, np.nan)
        err_24 = np.where(ok_24, noise_24, np.nan)
        
        isnlos = nlos > 0
        boundary = rng_s2.normal(np.where(isnlos, 30.0, 100.0), 10.0)
        ok_uwb = dist <= boundary
        nb = np.maximum(0.05, 0.139 * nlos - 2.139)
        los_n = rng_s2.normal(0, 0.100, len(dist))
        nlos_n = rng_s2.normal(0, nb * 0.6, len(dist))
        noise_uwb = np.where(isnlos, nlos_n, los_n)
        bias_uwb = np.where(isnlos, nb, 0.030)
        re_uwb = np.where(ok_uwb, dist + bias_uwb + noise_uwb, np.nan)
        err_uwb = np.where(ok_uwb, bias_uwb + noise_uwb, np.nan)
        
        s2_data[lam] = {
            "24_ok": ok_24, "24_re": re_24, "24_err": err_24,
            "uwb_ok": ok_uwb, "uwb_re": re_uwb, "uwb_err": err_uwb,
            "isnlos": isnlos.astype(int)
        }
        
    results = []
    # 5 Methods x 3 Topologies
    combos = [(m, t) for m in [1,2,3,4,5] for t in ["WAN","MESH","HYBRID"]]
    
    t0 = time.time()
    for P_fac in [0.05, 0.15, 0.30]:
        for lam in [25, 50, 100]:
            print(f"\n--- Grid: P_facility={P_fac}, lambda={lam} ---")
            at_facility = df["v_facility"].values < P_fac
            
            for method, topo in combos:
                mask = df["Topology"].values == topo
                idx = np.where(mask)[0]
                
                pos_err = df["PositionErrorM"].values[idx]
                gw_obs = df["GwObsCount"].values[idx]
                has_conn = gw_obs > 0
                has_tdoa = pos_err <= 5000.0 # Valid TDOA
                
                s2 = s2_data[lam]
                
                if method == 1: # LoRa Only
                    s2ok = np.zeros_like(s2["24_ok"][idx], dtype=bool)
                    s2err = np.full_like(s2["24_err"][idx], np.nan)
                elif method in (2, 4): # LoRa -> 2.4GHz
                    s2ok = s2["24_ok"][idx]
                    s2err = s2["24_err"][idx]
                elif method in (3, 5): # LoRa -> UWB
                    s2ok = s2["uwb_ok"][idx]
                    s2err = s2["uwb_err"][idx]
                
                # High-Fidelity Fused (Stage-2 triggers + connectivity to report)
                act = has_conn & s2ok & at_facility[idx]
                
                # TDOA Fallback (TDOA works + connectivity + no Stage-2)
                iact = has_conn & has_tdoa & ~act
                
                tsf = np.zeros(len(idx))
                if method in (4, 5): # With IMU
                    tsf = np.where(iact, rng_s2.exponential(15.0, len(idx)), 0.0)
                
                # EKF Fused Error
                fpe_full = ekf_fuse(pos_err, s2err, tsf)
                fpe_imu  = ekf_fuse(pos_err, np.full_like(s2err, np.nan), tsf)
                
                # Final physical error (np.nan means LOST)
                fpe = np.where(act, fpe_full, np.where(iact, fpe_imu, np.nan))
                
                # 3-State Metrics per fold
                folds = df["Fold"].values[idx]
                sr_list, ca_list = [], []
                
                for f in range(10):
                    fmask = (folds == f)
                    if fmask.sum() == 0: continue
                    f_fpe = fpe[fmask]
                    
                    succ = ~np.isnan(f_fpe)
                    sr = np.mean(succ)
                    ca = np.nanmean(f_fpe) if np.sum(succ) > 0 else np.nan
                    
                    sr_list.append(sr)
                    ca_list.append(ca)
                
                sr_m = np.mean(sr_list)*100; sr_ci = 1.96*np.std(sr_list)*100/np.sqrt(10)
                
                # Filter out nans for CondAcc stats if any fold completely failed
                valid_ca = [x for x in ca_list if not np.isnan(x)]
                if len(valid_ca) > 0:
                    ca_m = np.mean(valid_ca); ca_ci = 1.96*np.std(valid_ca)/np.sqrt(len(valid_ca))
                else:
                    ca_m = np.nan; ca_ci = np.nan
                
                print(f"M{method} {topo:6} | SR: {sr_m:5.1f}% ±{sr_ci:.1f} | CondAcc: {ca_m:6.1f}m ±{ca_ci:.1f}")
                
                results.append({
                    "P_facility": P_fac,
                    "Lambda": lam,
                    "Method": method,
                    "Topology": topo,
                    "SuccessRate": sr_m,
                    "SuccessRate_CI": sr_ci,
                    "CondAcc": ca_m,
                    "CondAcc_CI": ca_ci
                })
                
    tbl = pd.DataFrame(results)
    tbl.to_csv("results/5method_3topo_comparison.csv", index=False)
    print(f"\nDone in {time.time()-t0:.1f}s. Saved to results/5method_3topo_comparison.csv")

if __name__ == "__main__":
    evaluate_metrics()
