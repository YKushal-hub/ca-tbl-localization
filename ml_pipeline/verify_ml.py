import torch
import pandas as pd
import numpy as np
import argparse
from train import LoRaNet, LoRaDataset
from torch.utils.data import DataLoader

def verify(args):
    dataset = LoRaDataset(args.data)
    loader = DataLoader(dataset, batch_size=args.batch_size, shuffle=False)
    
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    model = LoRaNet().to(device)
    model.load_state_dict(torch.load(args.model))
    model.eval()
    
    criterion = torch.nn.MSELoss()
    total_loss = 0.0
    
    print('Running verification...')
    with torch.no_grad():
        for env, dist, tx, y in loader:
            env, dist, tx, y = env.to(device), dist.to(device), tx.to(device), y.to(device)
            out = model(env, dist, tx)
            total_loss += criterion(out, y).item() * env.size(0)
            
    total_loss /= len(dataset)
    rmse = np.sqrt(total_loss)
    print(f'Verification RMSE: {rmse:.3f} dB')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=str, default='../results/ml_training_dataset.csv')
    parser.add_argument('--model', type=str, default='model.pt')
    parser.add_argument('--batch_size', type=int, default=1024)
    args = parser.parse_args()
    verify(args)
