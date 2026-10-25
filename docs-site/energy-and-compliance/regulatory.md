# Regulatory considerations

!!! warning "No compliance claim"
    The simulator **enforces** the duty-cycle rule it models and reports the measured duty cycle. That is not a
    statement that any deployment complies. Compliance depends on the applicable rules, the device configuration and the
    operating conditions, and has not been assessed against hardware.

## The rule the model enforces

The governing rule is the Government of India's G.S.R. 853(E), 10 December 2021, Table II, for the 865–867 MHz band.
Its text names the detection of avalanche victims as an intended use of this device category.

| device class | power limit | duty cycle |
|---|---|---|
| network access points (gateways) | 500 mW e.r.p. | 10 % |
| other devices (field nodes, finders) | 500 mW e.r.p. | **2.5 %** |

Adaptive power control is a condition of the allowance and is implemented. Enforcement is the standard sub-GHz
off-time rule: after occupying the channel for $T$, stay silent until $T/\text{limit}$ has elapsed. `Architecture.md` §7.5 records a measured tier-1
duty cycle of 0.0226 against the 0.025 cap in an earlier campaign.

## A constraint that shapes the architecture

| quantity | value |
|---|---|
| tier-1 ranging frame (SF7, 125 kHz, CR 4/5, 20 B) | 56.58 ms |
| off-time armed by one frame at 2.5 % | 2.263 s |
| cycle period | 2.5 s |
| slack at cycle start | +0.237 s |
| escalation handshake frame | 41.22 ms |

The tier-1 budget affords about **one transmission per cycle**. A handoff needs two (ranging plus handshake), so a
handoff costs a ranging slot at any cycle period. This is why the cycle is 2.5 s and not 1.0 s, at which a node would
run at a 5.6 % duty cycle, illegal under this rule; results taken at 1.0 s are not comparable.

Regional rules differ. The 868 MHz band's rules in the EU, and the 2.4 GHz and UWB rules anywhere, are **not** modelled
here. Anyone deploying should consult the applicable authority's current text.
