# J2 manual control semantics and multi-split caveats

## Scope

These notes capture control semantics taken from the Toshiba J2 user manual together with direct observation of the J2 infrared remote-control display. They are intended to guide the Home Assistant control model and future UART validation. They do **not** promote untested protocol interactions to confirmed facts.

The directly relevant installed family is the `RAS-B10J2FVG` / `RAS-B13J2FVG` console family used on a shared multi-split outdoor unit.

## Evidence levels used here

- **Manual-documented**: stated by Toshiba documentation.
- **Remote-observed**: directly reproduced on the physical J2 remote with the IR transmitter covered so no command was sent to the IDU.
- **Inference**: physical/control interpretation that still needs UART or multi-head validation.

## Power Select

**Manual-documented:** Power Select limits the maximum current. Toshiba warns that inadequate cooling or heating capacity may occur while the limit is active.

This makes Power Select principally an outdoor-unit / system-capacity constraint rather than an ordinary local indoor-fan adjustment.

**Important limitation:** on a multi-split, this strongly implies that the physical operating envelope of the shared outdoor unit can affect all active heads. It does **not** by itself prove whether every IDU stores or reports one common Power Select state.

The protocol evidence remains separate: Power Select is carried on register `0x87` rather than the F7 special-function selector.

## Silent Operation

**Manual-documented:** Silent Operation is explicitly described as keeping the outdoor unit operating silently. Toshiba provides Silent 1 and Silent 2. Silent 2 prioritises outdoor-unit sound level and reduces the maximum sound level by 4 dB, with a warning that heating or cooling capacity may be inadequate.

This is therefore also an outdoor-unit-affecting control.

As with Power Select, the shared outdoor unit means a physical effect can propagate to all active IDUs. That is not yet proof that Silent state itself is globally stored or mirrored across every IDU.

## Power Select versus Silent

The J2 remote presents the Power Select choices and Silent 1 / Silent 2 through a cycling user interface. That is evidence that Toshiba groups them as alternative user choices on the remote.

However, cycling through a display is **not sufficient evidence** that their internal states cannot coexist. At least three implementations remain possible until UART testing is performed:

1. choosing one clears the other;
2. both states remain stored but one takes precedence;
3. both remain active and the outdoor unit obeys the tighter resulting constraint.

A decisive UART test is to set Power Select, then Silent, and inspect both `0x87` and F7/F8 state; then repeat in the reverse order.

## Hi POWER

**Manual-documented:** Hi POWER automatically controls room temperature and airflow for faster automatic cooling or heating operation. It is not available in Dry or Fan-only mode.

This description differs materially from Power Select and Silent. Hi POWER appears to alter the local demand/control strategy, including indoor airflow, so it should not currently be classified as a pure outdoor-unit cap.

On a multi-split, increasing demand from one IDU can still alter shared compressor operation and therefore indirectly affect the other heads. That system interaction must be distinguished from the logical scope of the Hi POWER command itself.

## Fireplace operation

**Manual-documented:** Fireplace operation keeps the indoor-unit fan blowing continuously during thermo-off so heat from another source can be circulated around the room. The manual describes default, Fireplace 1 and Fireplace 2 settings.

This is principally an indoor-fan/circulation policy rather than a heat-generation or outdoor-unit capacity mode.

**Manual-documented topology restriction:** Fireplace operation does not work with an IMS multi-system combination. Therefore Fireplace capability must not be inferred solely from the J2 indoor-unit family. System topology is also part of applicability.

For the installed multi-split system this function is not a candidate for control optimisation and should be treated as unavailable where the Toshiba IMS restriction applies.

## 8 °C heating operation

**Manual-documented:** 8 °C heating is a low-energy heating mode intended to keep room temperature within approximately 5–13 °C.

This is a setback / frost-protection operating regime, not a fine-grained normal-heating optimisation control.

## Fireplace / 8 °C selector behaviour on the J2 remote

**Remote-observed:** with the IR transmitter covered, stepping the relevant control on the J2 remote cycles through:

```text
Normal -> Fireplace 1 -> Fireplace 2 -> 8 °C heat -> Normal ...
```

Each selection removes the previous mode indication from the remote display.

This is strong evidence that, at the J2 remote/UI level, these are mutually exclusive special heating states rather than independent switches. It does not, by itself, prove how every underlying UART field is cleared or retained.

For J2 user-interface modelling, the preferred logical representation is therefore a single special-heating-mode selector:

```text
Normal / Fireplace 1 / Fireplace 2 / 8 °C heat
```

rather than independent Fireplace and 8 °C booleans. Fireplace must additionally be suppressed when the system topology is an unsupported IMS multi-system combination.

## Implication for optimisation experiments

For normal occupied-room optimisation on this multi-split, fan speed remains the cleanest clearly local actuator:

- manual fan level changes local indoor airflow;
- Power Select alters the permitted outdoor-unit current/capacity envelope;
- Silent Operation explicitly alters outdoor-unit operation;
- Hi POWER changes local demand/airflow strategy but can change the shared compressor operating point indirectly;
- Fireplace is a thermo-off circulation mode and is not applicable to the IMS multi-system arrangement;
- 8 °C heat is a setback/frost-protection regime.

The B13J2 can therefore be used as the instrumented reference unit to characterise how fan-speed changes alter useful heat delivery and system electrical input. Results can then inform fan-control policy on the less-instrumented B10J2 units, while recognising that total electrical input is a shared-system quantity rather than a per-IDU electrical measurement.

## Outstanding tests

1. Verify Power Select / Silent coexistence or cancellation by observing `0x87` and F7/F8 in both selection orders.
2. Test Hi POWER on one J2 head while another head is held steady and log the second head's load/allocation telemetry and fan state.
3. Capture the J2 UART state corresponding to the remote's `Normal / Fireplace 1 / Fireplace 2 / 8 °C` cycle to confirm the protocol representation.
4. Do not infer Fireplace availability from IDU family alone; retain the IMS multi-system restriction in the capability model.