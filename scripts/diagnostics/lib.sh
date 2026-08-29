#!/usr/bin/env bash
# Shared helpers. No set -e anywhere in this tree: a module that fails must be
# distinguishable from one that ran and found nothing, and neither may stop the batch.
DIAG_ROOT="${DIAG_ROOT:?}"
BIN=build/scratch/ns3.47-ca_tbl_causal_sim-optimized
finding() { printf '%s\n' "$*" >> "$DIAG_ROOT/.findings"; }
