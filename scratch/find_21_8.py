import numpy as np

def compute_coverage(ple, env_name):
    # nlos scale distance
    scale_dist = 8000.0
    
    # 500 nodes
    # 10 gateways
    # area 50x50km
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
            
            # fspl
            fspl = 31.2
            pl = fspl + 10 * ple * np.log10(d)
            
            # nlos
            p_nlos = 1.0 - np.exp(-d / scale_dist)
            is_nlos = np.random.rand() < p_nlos
            if is_nlos:
                pl += np.random.normal(19, 9)
                
            # body shadowing
            pl += 6.0
            
            # temp blockage
            pl += 0.0 # mostly 0
            
            # fading
            pl += np.random.normal(0, 8) # shadowing + fading approx
            
            # budget
            budget = 14.0 - (-137.0)
            if pl <= budget:
                gw_reached += 1
                
        if gw_reached >= 1:
            wan_count += 1
        if gw_reached >= 3:
            tdoa_count += 1
            
    return wan_count/500, tdoa_count/500

for ple in np.linspace(2.5, 4.0, 150):
    wan, tdoa = compute_coverage(ple, 'Suburban')
    if abs(wan - 0.218) < 0.02:
        print(f'PLE: {ple:.3f} -> WAN: {wan:.3f}, TDOA: {tdoa:.3f}')
