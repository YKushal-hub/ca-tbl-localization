import csv

max_rain = 0.0
env_8_count = 0
tunnel_1_count = 0

with open('results/ml_training_dataset.csv', 'r') as f:
    reader = csv.reader(f)
    next(reader) # skip header
    for row in reader:
        try:
            rain = float(row[41])
            if rain > max_rain:
                max_rain = rain
            
            env = int(row[0])
            if env == 8:
                env_8_count += 1
                
            istun = int(row[16])
            if istun == 1:
                tunnel_1_count += 1
        except Exception as e:
            pass

print(f"Max Rain: {max_rain}")
print(f"Env 8 count: {env_8_count}")
print(f"IsTunnel 1 count: {tunnel_1_count}")
