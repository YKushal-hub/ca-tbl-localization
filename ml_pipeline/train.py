import pandas as pd
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
import numpy as np
import time
import argparse

class LoRaDataset(Dataset):
    def __init__(self, csv_file, chunksize=100000):
        print(f'Loading data from {csv_file} using pandas chunking...')
        
        # We will load chunks and concatenate them to satisfy the chunking requirement
        chunks = []
        for chunk in pd.read_csv(csv_file, usecols=['Environment', 'DistanceM', 'TxPowerDbm', 'TrueRssiDbm'], chunksize=chunksize):
            chunks.append(chunk)
        df = pd.concat(chunks, ignore_index=True)
        
        self.env = torch.tensor(df['Environment'].values, dtype=torch.long)
        
        dist = np.log10(np.clip(df['DistanceM'].values, 1.0, None))
        dist_mean, dist_std = dist.mean(), dist.std()
        self.dist = torch.tensor((dist - dist_mean) / dist_std, dtype=torch.float32).unsqueeze(1)
        
        tx = df['TxPowerDbm'].values
        tx_mean, tx_std = tx.mean(), tx.std()
        if tx_std < 1e-6: tx_std = 1.0
        self.tx = torch.tensor((tx - tx_mean) / tx_std, dtype=torch.float32).unsqueeze(1)
        
        self.y = torch.tensor(df['TrueRssiDbm'].values, dtype=torch.float32).unsqueeze(1)
        
        self.n_samples = len(df)
        print(f'Loaded {self.n_samples} samples.')

    def __len__(self):
        return self.n_samples

    def __getitem__(self, idx):
        return self.env[idx], self.dist[idx], self.tx[idx], self.y[idx]

class LoRaNet(nn.Module):
    def __init__(self, num_envs=9):
        super(LoRaNet, self).__init__()
        self.env_embedding = nn.Embedding(num_envs, 4)
        
        self.fc = nn.Sequential(
            nn.Linear(4 + 1 + 1, 64),
            nn.ReLU(),
            nn.Linear(64, 64),
            nn.ReLU(),
            nn.Linear(64, 1)
        )

    def forward(self, env, dist, tx):
        env_emb = self.env_embedding(env)
        x = torch.cat([env_emb, dist, tx], dim=1)
        return self.fc(x)

def train(args):
    dataset = LoRaDataset(args.data)
    
    train_size = int(0.8 * len(dataset))
    val_size = len(dataset) - train_size
    train_dataset, val_dataset = torch.utils.data.random_split(dataset, [train_size, val_size])
    
    train_loader = DataLoader(train_dataset, batch_size=args.batch_size, shuffle=True, num_workers=2)
    val_loader = DataLoader(val_dataset, batch_size=args.batch_size, shuffle=False, num_workers=2)
    
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    model = LoRaNet().to(device)
    
    criterion = nn.MSELoss()
    optimizer = torch.optim.Adam(model.parameters(), lr=args.lr)
    
    print(f'Training on {device}...')
    for epoch in range(args.epochs):
        model.train()
        train_loss = 0.0
        start_t = time.time()
        
        for env, dist, tx, y in train_loader:
            env, dist, tx, y = env.to(device), dist.to(device), tx.to(device), y.to(device)
            
            optimizer.zero_grad()
            out = model(env, dist, tx)
            loss = criterion(out, y)
            loss.backward()
            optimizer.step()
            
            train_loss += loss.item() * env.size(0)
            
        train_loss /= len(train_dataset)
        
        model.eval()
        val_loss = 0.0
        with torch.no_grad():
            for env, dist, tx, y in val_loader:
                env, dist, tx, y = env.to(device), dist.to(device), tx.to(device), y.to(device)
                out = model(env, dist, tx)
                val_loss += criterion(out, y).item() * env.size(0)
        val_loss /= len(val_dataset)
        
        epoch_time = time.time() - start_t
        print(f'Epoch {epoch+1}/{args.epochs} [{epoch_time:.1f}s] - Train Loss (MSE): {train_loss:.2f} - Val Loss (MSE): {val_loss:.2f} - Val RMSE: {np.sqrt(val_loss):.2f} dB')

    torch.save(model.state_dict(), 'model.pt')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', type=str, default='../results/ml_training_dataset.csv')
    parser.add_argument('--batch_size', type=int, default=1024)
    parser.add_argument('--epochs', type=int, default=10)
    parser.add_argument('--lr', type=float, default=0.001)
    args = parser.parse_args()
    
    train(args)
