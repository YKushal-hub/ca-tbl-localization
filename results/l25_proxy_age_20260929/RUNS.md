# L.2.5 runs

Registered in `report/PREREG_L25_PROXY_AGE.md`, commit `b16531f`, committed 2026-09-29 15:55:52 +0530.
Runner: `bash scripts/l25_proxy_age_run.sh batch <cell> 7 11 13`, 2 concurrent, cells in the
order searcher_t1, sparse_t1, searcher_t2, sparse_t2. Every invocation is in its `run_id.txt`,
with a SHA-256 of `arm_summary.csv`, the trace and `proxy_checks_*.csv` (the latter two untracked:
regenerate from the invocation; the digest vouches for them).

Earliest file in this directory: 2026-09-29 15:56:01; latest: 2026-09-29 15:57:18 (local time, +0530).

| cell | seed | runner rc | wall s | run_id.txt written | source |
|---|---|---|---|---|---|
| searcher_t1 | 7 | 0 | 10.760 | 15:56:11 | b16531f dirty 0 |
| searcher_t1 | 11 | 0 | 8.400 | 15:56:09 | b16531f dirty 0 |
| searcher_t1 | 13 | 0 | 10.648 | 15:56:20 | b16531f dirty 0 |
| sparse_t1 | 7 | 0 | 4.746 | 15:56:24 | b16531f dirty 0 |
| sparse_t1 | 11 | 0 | 3.661 | 15:56:23 | b16531f dirty 0 |
| sparse_t1 | 13 | 0 | 4.480 | 15:56:28 | b16531f dirty 0 |
| searcher_t2 | 7 | 0 | 19.940 | 15:56:48 | b16531f dirty 0 |
| searcher_t2 | 11 | 0 | 16.929 | 15:56:45 | b16531f dirty 0 |
| searcher_t2 | 13 | 0 | 17.363 | 15:57:02 | b16531f dirty 0 |
| sparse_t2 | 7 | 0 | 8.890 | 15:57:11 | b16531f dirty 0 |
| sparse_t2 | 11 | 0 | 8.868 | 15:57:11 | b16531f dirty 0 |
| sparse_t2 | 13 | 0 | 7.301 | 15:57:18 | b16531f dirty 0 |
