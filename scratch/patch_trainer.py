#!/usr/bin/env python3
"""patch_trainer.py -- Add shared deterministic train/test split to train_stage2_parallel.py"""
import sys

PATH = "/home/kushal/ns/ns-3-dev/scratch/train_stage2_parallel.py"
with open(PATH) as f:
    src = f.read()

# ── Patch 1: replace per-variant split with shared index ─────────────────
OLD_SPLIT = """        X_tr, X_te, y_tr, y_te = train_test_split(
            X, y, test_size=0.15, random_state=42
        )
        mode_te = mode_col.loc[y_te.index]"""

NEW_SPLIT = """        # Shared deterministic split: written once, reused across all 15 variants.
        # Without this, each variant draws its own shuffled 15% test set from a
        # differently-sampled dataframe, making LORA_ONLY_MAE incomparable
        # across Method/Topology (observed: 32m gap between M1 and M2 variants).
        split_train_path = Path(DST) / "split_index_train.npy"
        split_test_path  = Path(DST) / "split_index_test.npy"
        n_total = len(X)
        if split_train_path.exists() and split_test_path.exists():
            # Load existing split (same indices for all variants)
            tr_idx = np.load(split_train_path, allow_pickle=True)
            te_idx = np.load(split_test_path,  allow_pickle=True)
            # Indices may be larger than current df if sample sizes differ slightly
            # — intersect with actual integer positions
            tr_idx = tr_idx[tr_idx < n_total]
            te_idx = te_idx[te_idx < n_total]
        else:
            # First variant: create and save the canonical split
            all_idx = np.arange(n_total)
            rng_split = np.random.default_rng(2026_07_11)  # fixed seed
            rng_split.shuffle(all_idx)
            cut = int(n_total * 0.85)
            tr_idx, te_idx = all_idx[:cut], all_idx[cut:]
            np.save(split_train_path, tr_idx)
            np.save(split_test_path,  te_idx)
            print("[{}] Saved shared split index: {:,} train / {:,} test".format(
                label, len(tr_idx), len(te_idx)), flush=True)

        X_arr = X.values
        y_arr = y.values
        X_tr = pd.DataFrame(X_arr[tr_idx], columns=X.columns)
        X_te = pd.DataFrame(X_arr[te_idx], columns=X.columns)
        y_tr = pd.Series(y_arr[tr_idx], name=y.name)
        y_te = pd.Series(y_arr[te_idx], name=y.name)
        mode_te = pd.Series(mode_col.values[te_idx], name="FinalLocalizationMode")"""

if OLD_SPLIT not in src:
    print("ERROR: split pattern not found — check trainer version"); sys.exit(1)

src = src.replace(OLD_SPLIT, NEW_SPLIT, 1)

# ── Patch 2: ensure numpy imported (it already is via lgb, but be explicit) ─
if "import numpy as np" not in src:
    src = src.replace("import pandas as pd", "import numpy as np\nimport pandas as pd", 1)

with open(PATH, "w") as f:
    f.write(src)

# Verify
assert "split_index_train.npy" in src, "Patch FAILED"
assert "2026_07_11" in src, "Fixed seed FAILED"
print("train_stage2_parallel.py patched: shared split index added.")
print(f"  'train_test_split' calls remaining: {src.count('train_test_split(')}")
print(f"  (should be 0 or only in import line)")
