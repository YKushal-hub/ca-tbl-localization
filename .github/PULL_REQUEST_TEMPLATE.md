## Change

<!-- What changes, and why. One change per pull request where possible. -->

## Evidence

<!-- The measured number, the gate, the null control. Link the pre-registration for a campaign. -->

## Checklist

- [ ] New mechanism: default-off flag, byte-identical null control, gate with a negative fixture
- [ ] Campaign: pre-registration committed before the first cell; run records `dirty 0`
- [ ] No tracked `results/` directory regenerated in place; no `run_id.txt` edited; no `trace_*.csv` tracked
- [ ] `bash scripts/catbl_gates.sh` run; only the expected reds (G11, G93) remain; `git status` unchanged by it
- [ ] Documents that quote a number name its source and date
