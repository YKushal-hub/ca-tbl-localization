import numpy as np
from scipy import stats
from scipy.stats import linregress
from typing import List

class Request:
    def __init__(self, node_id: int, pcs_history: List[float], battery_remaining: float, battery_cap: float, 
                 time_since_fix: float, priority: str):
        self.node_id = node_id
        self.pcs_history = pcs_history
        self.battery_remaining = battery_remaining
        self.battery_cap = battery_cap
        self.dt = time_since_fix
        self.priority = priority # 'ambient', 'normal', 'URGENT'

STALENESS_BOUND = 10.0

def compute_T_deg(pcs_history: List[float], k: float) -> float:
    if len(pcs_history) < 3:
        return 0.0
    pts = pcs_history[-5:]
    res = linregress(range(len(pts)), pts)
    pcs_slope = res.slope
    degradation_magnitude = max(0, -pcs_slope)
    return 1.0 - np.exp(-k * degradation_magnitude)

def compute_T_batt(battery_rem: float, battery_cap: float) -> float:
    return 1.0 - (battery_rem / battery_cap)

def compute_T_stale(dt: float) -> float:
    return min(1.0, dt / STALENESS_BOUND)

def compute_T_query(priority: str) -> float:
    if priority == 'URGENT':
        return 1.0
    elif priority == 'normal':
        return 0.5
    return 0.0

def score_request(req: Request, lam: tuple, k: float) -> float:
    t_deg = compute_T_deg(req.pcs_history, k)
    t_batt = compute_T_batt(req.battery_remaining, req.battery_cap)
    t_stale = compute_T_stale(req.dt)
    t_query = compute_T_query(req.priority)
    return lam[0]*t_deg + lam[1]*t_batt + lam[2]*t_stale + lam[3]*t_query

def calibrate_k(samples: int = 1000):
    slopes = []
    for _ in range(samples):
        start = np.random.uniform(2.0, 3.0)
        end = np.random.uniform(1.0, start)
        hist = np.linspace(start, end, 5) + np.random.normal(0, 0.05, 5)
        res = linregress(range(5), hist)
        slopes.append(max(0, -res.slope))
    p90 = np.percentile(slopes, 90)
    k = -np.log(0.05) / p90
    print(f"[Calibration] P90 degradation magnitude = {p90:.3f} per step")
    print(f"[Calibration] Calibrated k = {k:.3f}")
    return k

class Scheduler:
    def __init__(self, method: str, lam: tuple = (1, 1, 1, 1), k: float = 15.0):
        self.method = method
        self.lam = lam
        self.k = k

    def pick_next(self, queue: List[Request]) -> Request:
        if not queue:
            return None
        
        override_candidates = [r for r in queue if r.dt >= 3 * STALENESS_BOUND]
        if override_candidates:
            override_candidates.sort(key=lambda r: r.dt, reverse=True)
            winner = override_candidates[0]
            # winner.dt = 0.0 is done in the loop
            return winner
            
        if self.method == 'FIFO':
            winner = max(queue, key=lambda r: r.dt)
            return winner
            
        elif self.method == 'Strict Priority':
            urgents = [r for r in queue if r.priority == 'URGENT']
            if urgents:
                winner = max(urgents, key=lambda r: r.dt)
                return winner
            normals = [r for r in queue if r.priority == 'normal']
            if normals:
                winner = max(normals, key=lambda r: r.dt)
                return winner
            winner = max(queue, key=lambda r: r.dt)
            return winner
            
        elif self.method == 'Dynamic':
            winner = max(queue, key=lambda r: score_request(r, self.lam, self.k))
            return winner

def test_hand_built(k: float):
    print("\n--- Hand-Built Scenario ---")
    queue = [
        Request(1, [2.5, 2.2, 1.9, 1.6, 1.3], 90, 100, 2.0, 'ambient'), # Fast degrading
        Request(2, [2.5, 2.5, 2.5, 2.5, 2.5], 5, 100, 2.0, 'ambient'),  # Low battery
        Request(3, [2.5, 2.5, 2.5, 2.5, 2.5], 90, 100, 2.0, 'URGENT'),  # Query triggered
        Request(4, [2.5, 2.5, 2.5, 2.5, 2.5], 90, 100, 2.0, 'ambient'), # Ordinary 1
        Request(5, [2.5, 2.5, 2.5, 2.5, 2.5], 90, 100, 5.0, 'ambient'), # Ordinary 2 (staler)
    ]
    
    lam = (1.0, 1.0, 0.5, 2.0)
    for r in queue:
        s = score_request(r, lam, k)
        print(f"Node {r.node_id}: Score = {s:.3f} | T_deg={compute_T_deg(r.pcs_history, k):.2f}, T_batt={compute_T_batt(r.battery_remaining, r.battery_cap):.2f}, T_stale={compute_T_stale(r.dt):.2f}, T_query={compute_T_query(r.priority):.2f}")

    sch = Scheduler('Dynamic', lam, k)
    winner = sch.pick_next(queue)
    print(f"Winner: Node {winner.node_id} (Expected 3 or 1 depending on weights)")

def test_starvation_isolation(k: float):
    print("\n--- Dedicated Starvation Isolation Test ---")
    lam = (1.0, 1.0, 1.0, 2.0)
    sch = Scheduler('Dynamic', lam, k)
    
    node2_dt = 0.0
    granted_at = -1
    for step in range(40):
        q = [
            Request(1, [2.5, 2.5, 2.5, 2.5, 2.5], 90, 100, 2.0, 'URGENT'),
            Request(2, [2.5, 2.5, 2.5, 2.5, 2.5], 90, 100, node2_dt, 'ambient')
        ]
        winner = sch.pick_next(q)
        if winner.node_id == 2:
            granted_at = step
            print(f"Node 2 (Ambient) force-granted at step {step} with dt={node2_dt:.1f}!")
            break
        else:
            node2_dt += 1.0
            q[1].dt = node2_dt
            
    assert granted_at != -1, "Node 2 starved indefinitely!"
    assert node2_dt >= 3 * STALENESS_BOUND, "Granted too early (not via override)!"
    print("Starvation isolation test passed: Override successfully preempted permanent starvation.")

def run_simulation(seed: int, method: str, lam: tuple, k: float, total_steps: int = 1000):
    np.random.seed(seed)
    nodes = []
    for i in range(1, 21):
        if i <= 4:
            prio = 'URGENT'
        elif i <= 10:
            prio = 'normal'
        else:
            prio = 'ambient'
        nodes.append({'id': i, 'prio': prio, 'dt': 0.0, 'wait_time': 0.0, 'batt': 100.0, 'true_pcs': 2.5, 'pcs_history': [2.5, 2.5, 2.5, 2.5, 2.5], 'is_degrading': False})
        
    sch = Scheduler(method, lam, k)
    
    urgent_ttf = []
    normal_ttf = []
    ambient_ttf = []
    starvations = 0 
    
    for step in range(total_steps):
        queue = []
        for n in nodes:
            # Stateful PCS logic
            if not n['is_degrading'] and np.random.rand() < 0.02: 
                # 2% chance per step to start a permanent degradation crisis
                n['is_degrading'] = True
                
            if n['is_degrading']:
                n['true_pcs'] = max(1.0, n['true_pcs'] - np.random.uniform(0.02, 0.10))
            else:
                n['true_pcs'] = np.clip(n['true_pcs'] + np.random.normal(0, 0.02), 2.0, 3.0)
                
            measured_pcs = n['true_pcs'] + np.random.normal(0, 0.05)
            n['pcs_history'].pop(0)
            n['pcs_history'].append(measured_pcs)
                
            # Stateful Battery logic
            n['batt'] = max(0.0, n['batt'] - np.random.uniform(0.01, 0.05))
            
            queue.append(Request(n['id'], list(n['pcs_history']), n['batt'], 100.0, n['dt'], n['prio']))
            
        winner = sch.pick_next(queue)
        
        if winner.dt >= 3 * STALENESS_BOUND:
            starvations += 1
            
        winner.dt = 0.0 # Reset after checking override
            
        for n in nodes:
            if n['id'] == winner.node_id:
                if n['prio'] == 'URGENT':
                    urgent_ttf.append(n['dt'])
                elif n['prio'] == 'normal':
                    normal_ttf.append(n['dt'])
                elif n['prio'] == 'ambient':
                    ambient_ttf.append(n['dt'])
                n['dt'] = 0.0
            else:
                n['dt'] += 1.0
                
    return {
        'urgent_ttf': np.mean(urgent_ttf) if urgent_ttf else 0,
        'normal_ttf': np.mean(normal_ttf) if normal_ttf else 0,
        'ambient_ttf': np.mean(ambient_ttf) if ambient_ttf else 0,
        'starvations': starvations
    }

def print_paired_stats(name: str, a: list, b: list):
    diff = np.array(a) - np.array(b)
    mean_diff = np.mean(diff)
    se = np.std(diff, ddof=1) / np.sqrt(len(diff))
    if se == 0:
        ci_lower, ci_upper, p_val = mean_diff, mean_diff, 1.0
    else:
        ci_lower, ci_upper = stats.t.interval(0.95, len(diff)-1, loc=mean_diff, scale=se)
        p_val = stats.ttest_rel(a, b).pvalue
        
    wins = np.sum(diff < 0)
    losses = np.sum(diff > 0)
    ties = np.sum(diff == 0)
    print(f"Paired Diff ({name}): {mean_diff:+.3f} (95% CI: [{ci_lower:+.3f}, {ci_upper:+.3f}]) | p={p_val:.4e}")
    print(f"  Concordance (A better < 0): A won {wins}, B won {losses}, Ties {ties}")

def run_30_seed_sweep(k: float):
    print("\n--- 30-Seed Lambda Sweep ---")
    
    # We will sweep lambda_4 (Query priority weight) while holding others fixed at 1.0
    # lambda = (deg, batt, stale, query)
    lam_sweep = [
        (1.0, 1.0, 1.0, 0.5),
        (1.0, 1.0, 1.0, 1.0),
        (1.0, 1.0, 1.0, 2.0),
        (1.0, 1.0, 1.0, 5.0)
    ]
    
    # We will also run FIFO and Strict Priority as baselines
    print(f"{'Method':<20} | {'L4':<4} | {'Urgent TTF':<10} | {'Ambient TTF':<12} | {'Overrides (Mean)':<15}")
    print("-" * 75)
    
    fifo_urg, fifo_amb, fifo_starv = [], [], []
    strict_urg, strict_amb, strict_starv = [], [], []
    
    for seed in range(1, 31):
        res_fifo = run_simulation(seed, 'FIFO', (0,0,0,0), k)
        fifo_urg.append(res_fifo['urgent_ttf'])
        fifo_amb.append(res_fifo['ambient_ttf'])
        fifo_starv.append(res_fifo['starvations'])
        
        res_strict = run_simulation(seed, 'Strict Priority', (0,0,0,0), k)
        strict_urg.append(res_strict['urgent_ttf'])
        strict_amb.append(res_strict['ambient_ttf'])
        strict_starv.append(res_strict['starvations'])
        
    print(f"{'FIFO':<20} | {'-':<4} | {np.mean(fifo_urg):<10.3f} | {np.mean(fifo_amb):<12.3f} | {np.mean(fifo_starv):<15.3f}")
    print(f"{'Strict Priority':<20} | {'-':<4} | {np.mean(strict_urg):<10.3f} | {np.mean(strict_amb):<12.3f} | {np.mean(strict_starv):<15.3f}")
    
    for lam in lam_sweep:
        dyn_urg, dyn_amb, dyn_starv = [], [], []
        for seed in range(1, 31):
            res_dyn = run_simulation(seed, 'Dynamic', lam, k)
            dyn_urg.append(res_dyn['urgent_ttf'])
            dyn_amb.append(res_dyn['ambient_ttf'])
            dyn_starv.append(res_dyn['starvations'])
        
        print(f"{'Dynamic Scoring':<20} | {lam[3]:<4.1f} | {np.mean(dyn_urg):<10.3f} | {np.mean(dyn_amb):<12.3f} | {np.mean(dyn_starv):<15.3f}")
        
        if lam[3] == 2.0:
            dyn_baseline_urg = dyn_urg
            dyn_baseline_starv = dyn_starv
            
    print("\n--- Statistical Comparison (Dynamic L4=2.0 vs Baselines) ---")
    print("\n[Dynamic vs FIFO]")
    print_paired_stats("URGENT Time-to-Fix", dyn_baseline_urg, fifo_urg)
    print_paired_stats("Override Grants (Starvation Events)", dyn_baseline_starv, fifo_starv)
    
    print("\n[Dynamic vs Strict Priority]")
    print_paired_stats("URGENT Time-to-Fix", dyn_baseline_urg, strict_urg)
    print_paired_stats("Override Grants (Starvation Events)", dyn_baseline_starv, strict_starv)

if __name__ == '__main__':
    np.random.seed(42)
    k = calibrate_k()
    test_hand_built(k)
    test_starvation_isolation(k)
    run_30_seed_sweep(k)
