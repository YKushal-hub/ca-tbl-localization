import numpy as np

def compute_coverage(ple, tx_power):
    scale_dist = 8000.0
    np.random.seed(42)
    nodes_x = np.random.uniform(0, 50000, 500)
    nodes_y = np.random.uniform(0, 50000, 500)
    
    gw_x = np.random.uniform(0, 50000, 10)
    gw_y = np.random.uniform(0, 50000, 10)
    
    wan_count = 0
    tdoa_count = 0
    
    for i in range(500):
        gw_reached = 0
        for j in range(10):
            d = np.sqrt((nodes_x[i]-gw_x[j])**2 + (nodes_y[i]-gw_y[j])**2)
            if d < 1.0: d = 1.0
            
            fspl = 31.2
            pl = fspl + 10 * ple * np.log10(d)
            
            p_nlos = 1.0 - np.exp(-d / scale_dist)
            is_nlos = np.random.rand() < p_nlos
            if is_nlos:
                pl += np.random.normal(19, 9)
                
            pl += 6.0 # body
            
            pl += np.random.normal(0, 8) # fading
            
            budget = tx_power - (-137.0)
            if pl <= budget:
                gw_reached += 1
                
        if gw_reached >= 1:
            wan_count += 1
        if gw_reached >= 3:
            tdoa_count += 1
            
    return wan_count/500, tdoa_count/500

print('14 dBm, PLE 2.75:', compute_coverage(2.75, 14.0))
print('30 dBm, PLE 3.58:', compute_coverage(3.58, 30.0))
print('14 dBm, PLE 3.58:', compute_coverage(3.58, 14.0))

for tx in np.linspace(14, 30, 17):
    wan, tdoa = compute_coverage(3.58, tx)
    if abs(wan - 0.218) < 0.1:
        print(f'Tx: {tx} -> WAN: {wan:.3f}, TDOA: {tdoa:.3f}')
