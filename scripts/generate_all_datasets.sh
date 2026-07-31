#!/bin/bash
set -e

# Run from ns-3-dev directory
if [ ! -f "ns3" ]; then
    echo "Please run from ns-3-dev root directory"
    exit 1
fi

echo "Building ns-3..."
./ns3 build

OUTPUT_DIR="./results"
mkdir -p $OUTPUT_DIR

SEEDS=(7 11 23)

DATASETS=("A" "B" "C" "D" "E")

echo "Generating datasets..."
for seed in "${SEEDS[@]}"; do
    echo "Running for seed $seed"
    for ds in "${DATASETS[@]}"; do
        echo "  Dataset $ds"
        ./ns3 run "scratch/lora_dataset_generator --dataset=$ds --seed=$seed --output_dir=$OUTPUT_DIR --packets_per_point=100 --verbose=false"
    done
done

echo "Running analysis..."
python3 analysis/compare_engine_to_fielddata.py

echo "Done! Check overall_agreement_report.txt and $OUTPUT_DIR"
