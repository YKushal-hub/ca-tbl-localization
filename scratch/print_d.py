import sys
sys.path.append('analysis')
import compare_engine_to_fielddata as c
res = c.analyze_dataset_D(7, './results')
print("Dataset D Medians (std, fine):", res)
if res and res[1] > 0:
    print("Improvement ratio:", res[0] / res[1])
