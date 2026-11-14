# References

The full parameter and prior-art lists are in [`Architecture.md` §13](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/Architecture.md)
and [`docs/CONTEXT_PACK.md`](https://github.com/YKushal-hub/ca-tbl-localization/blob/main/docs/CONTEXT_PACK.md), which also record
where a constant is the project's own rather than cited. The works below are those this site relies on.

## Prior art

- Q. Vey, F. Spies, B. Pestourie *et al.*, "POUCET: A Multi-Technology Indoor Positioning Solution for Firefighters and
  Soldiers," *Proc. IPIN*, 2021. The closest fielded system; the model for the `poucet_fusion` arm.
- A. Benouakta, T. M. Nguyen, F. Ferrero *et al.*, "Design of a Multi-Standard UWB-LoRa Antenna Structure and Transceiver
  Board," *Electronics*, 2023. UWB ranging floor; 124 m line-of-sight and 55 m non-line-of-sight.
- G. M. Bianco, R. Giuliano, G. Marrocco *et al.*, "LoRa System for Search and Rescue: Path-Loss Models and Procedures in
  Mountain Scenarios," *IEEE Internet of Things Journal*, 2021.
- B. Li, Y. Xu, Y. Liu, Z. Shi, "LoRaWAPS: A Wide-Area Positioning System Based on LoRa Mesh," *Applied Sciences*, 2023.
- F. R. Andersen, K. D. Ballal, M. N. Petersen *et al.*, "Ranging Capabilities of LoRa 2.4 GHz," *Proc. IEEE WF-IoT*, 2020.
- E. Gottschalk, S. Ayub, M. N. Petersen, "Low-Power Distance Ranging for 2D-Positioning Using LoRa 2.4 GHz," *Internet of
  Things*, 2026.
- Y. Zhou, X. Zhou, Z. Cheng *et al.*, "Impact of Environmental Factors on LoRa 2.4 GHz Time of Flight Ranging Outdoors,"
  2025. The tier-2 ranging floor.
- J. Petäjäjärvi, K. Mikhaylov *et al.*, "On the Coverage of LPWANs: Range Evaluation and Channel Attenuation Model for LoRa
  Technology," *Proc. ITST*, 2015.

## Methods

- Y. T. Chan and K. C. Ho, "A Simple and Efficient Estimator for Hyperbolic Location," *IEEE Trans. Signal Processing*, 1994.
- W. H. Foy, "Position-Location Solutions by Taylor-Series Estimation," *IEEE Trans. Aerospace and Electronic Systems*, 1976.
- R. E. Kalman, "A New Approach to Linear Filtering and Prediction Problems," *ASME J. Basic Engineering*, 1960.
- S. Joshi and S. Boyd, "Sensor Selection via Convex Optimization," *IEEE Trans. Signal Processing*, 2009.
- S. J. Julier and J. K. Uhlmann, "A Non-Divergent Estimation Algorithm in the Presence of Unknown Correlations,"
  *Proc. American Control Conference*, 1997. Covariance intersection.
- A. Demers *et al.*, "Epidemic Algorithms for Replicated Database Maintenance," *Proc. PODC*, 1987. The anti-entropy digest.

## Hardware, standards and regulation

- Semtech, SX1261/SX1262 data sheet; SX1280/SX1281 data sheet (Table 6-1); *SX1272/3/6/7/8 LoRa Modem Designer's Guide*
  (the time-on-air relation).
- Qorvo (Decawave), DW3000 data sheet (Table 9).
- IEEE 802.11e, Amendment 8: MAC Quality of Service Enhancements (the access-category pattern, adapted).
- ITU-R P.833, Attenuation in vegetation.
- Government of India, Ministry of Communications, Department of Telecommunications, G.S.R. 853(E), 10 December 2021,
  Table II (the duty-cycle rule enforced by the model).

A citation that has not been checked against its source is not listed here.
