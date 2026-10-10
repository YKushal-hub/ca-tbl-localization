#!/bin/bash
cd "$(dirname "$0")/.." || exit 1   # repository root
./ns3 run "lora_dataset_generator --dataset=B --seed=7"
./ns3 run "lora_dataset_generator --dataset=C --seed=7"
./ns3 run "lora_dataset_generator --dataset=C2 --seed=7"
./ns3 run "lora_dataset_generator --dataset=D --seed=7 --packets_per_point=100"
./ns3 run "lora_dataset_generator --dataset=E --seed=7"
python3 analysis/compare_engine_to_fielddata.py
