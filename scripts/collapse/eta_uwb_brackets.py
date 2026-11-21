# Derives the effective rubble solid fraction and UWB rubble-loss brackets (rows ETA-*, UWB65-*, UWB8-*, SLOW-BR).
import math
def itu(f): # ITU-R P.2040-4 concrete, dB/m; A = 1636*sigma/sqrt(eps')
    ep=5.24; sig=0.0462*f**0.7822; return 1636*sig/math.sqrt(ep)
nist2=103.82; nist65=242.34   # NIST6055-LOW/HIGH
A2={'low':20.0,'central':31.0,'high':80.0}   # OES layered no-ferro / with ferro / measured, 2 GHz
solid2={'low':itu(2.0),'central':itu(2.0),'high':nist2}
solid65={'low':itu(6.5),'central':itu(6.5),'high':nist65}
solid8={'low':itu(8.0),'central':itu(8.0),'high':None}
epsr={'low':5.24,'central':(5.24+9)/2,'high':9.0}
for b in A2:
    eta=A2[b]/solid2[b]
    print(b, f"solid2={solid2[b]:.2f} eta={eta:.4f} UWB6.5={eta*solid65[b]:.2f}",
      f"UWB8={eta*itu(8.0):.2f}(ITU)", f"sqrt(eps)-1={math.sqrt(epsr[b])-1:.4f} bias/m={eta*(math.sqrt(epsr[b])-1):.4f}")
print("itu 2,6.5,8:",itu(2),itu(6.5),itu(8))
