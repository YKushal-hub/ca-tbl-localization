#!/bin/bash
cd "$(dirname "$0")" || exit 1
python3 train.py --data ../results/ml_training_dataset_test_500k.csv --epochs 20 --batch_size 1024
