# Failure modes

Each row is a failure the repository has measured, designed against or recorded. None is hypothetical, and the
fault model is **environmental, not adversarial**.

| failure | mechanism | handling |
|---|---|---|
| no usable anchors at 5 km | a subject often holds one precise anchor or none, so a single circle's two sides cannot be told apart | finders tasked in **pairs**, a quarter turn apart, so two precise anchors cross; the deployment requirement is set by anchor supply, not the filter |
| wrong root of a circle intersection | a Gaussian started at one root stays there | particle-filter acquisition keeps every root alive until the cloud is one mode |
| wrong TDoA position | three gateways give an exact solve with no redundancy | fused only if its normalised innovation is within the 99 % $\chi^2$ bound |
| one range turns into heading, bias and scale error | at 5 km one precise anchor lets a single update corrupt the inertial states | 60-cycle window smoother; magnetometer reference |
| overconfident relay | a relay's own error is not an anchor's | $\sigma_\mathrm{eff}^2=\sigma^2+u^{\mathsf T}P_Bu$, NIS inflation, a $\chi^2_1(0.999)$ gate, depth limit two |
| finder GNSS error read as range noise | it is a slowly varying offset common to a finder's ranges | consider state |
| escalation into a dead zone | the target tier cannot answer | a failed exchange is charged its transmission **plus the full receive timeout**, so it is expensive, not merely useless; evidence gate suppresses |
| handshake blocked by the duty cap | 56.58 ms arms 2.263 s of off-time against a 2.5 s cycle | handshake frames are on the air and counted against the cap; **demotion is never deferred** |
| escalation on an untrustworthy estimate | the score is a function of the covariance, which may be wrong | switching-point test on residuals against other nodes' reports (independent of the covariance) |
| silenced or battery-dead node | a node that stops transmitting | a silenced radio relays nothing and a depleted node draws nothing (`--silenceStopsRelay`, `--deadBatteryNoDraw`; only act in fault runs) |
| stable biased RSSI (cold battery, iced antenna, canopy multipath) | a variance-based check reads it as clean | the fix test targeted this, and **did not detect it well** (recall 0.003–0.074); it was removed |

See also [limitations](../research/limitations.md) for failures the project has *not* closed.
