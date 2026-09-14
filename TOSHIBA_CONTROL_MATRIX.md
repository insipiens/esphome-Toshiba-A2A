# Toshiba control matrix redesign

## Status

**Work in progress.** This document describes the architecture and test programme being used to replace the historical one-register/one-UI-control assumptions in the component. It records current evidence; it is not a claim that all Toshiba residential IDUs implement every function listed here.

The project is derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi). The original project established the practical ESPHome/UART implementation on which this work began.

## 1. Design principle

The Toshiba remote control and Toshiba app are the reference for the user-facing control taxonomy.

The UART protocol is an implementation detail. Two controls sharing a register or value space does not, by itself, prove that they are one mutually-exclusive user function. Conversely, the app may group controls together even when they live in different protocol registers.

The intended layering is:

```text
Toshiba UART packet
        |
        v
protocol decoder
        |
        v
logical Toshiba state
        |
        v
family capability + HVAC-mode rules + compatibility rules
        |
        v
ESPHome entities
        |
        v
Home Assistant
```

Do not reduce this to:

```text
UART register -> Home Assistant selector
```

## 2. Equipment identification

Pushed class-`0x11` register `0xE0` messages are used as the working equipment-identification source.

Captured 114-byte frames contain two 50-byte equipment records:

```text
byte 12       0xE0
bytes 13..62  IDU record
bytes 63..112 ODU record
byte 113      checksum
```

Each record currently decodes as:

```text
+0..20   model, 21 bytes
+21..33  identifier, 13 bytes — meaning unresolved
+34..42  identifier, 9 bytes  — meaning unresolved
+43..49  identifier, 7 bytes  — meaning unresolved
```

The first model string is treated as the IDU model and the second as the ODU model. Some valid frames from older/different IDU firmware have contained literal `NULL` in the IDU model field. That is represented as model unavailable; the ODU model is never promoted into the IDU field.

`0xE0` has been observed as pushed/asynchronous traffic. A short ordinary active read can time out, so the component must not assume that equipment identity can be obtained by polling `0xE0` during initialisation.

## 3. Direct test scope

The redesign has been developed against one real multi-split system, not a broad Toshiba laboratory fleet.

Directly available hardware includes:

- ODU: `RAS-5M34G3AVG-E1`;
- `RAS-B13J2FVG-E1` floor/console IDU;
- `RAS-B10J2FVG`-family floor/console IDUs with different/older firmware behaviour;
- `RAS-B10P2KVSGB-E` high-wall IDU identified through `0xE0`;
- `RAS-B10G3KVSG`-family high-wall IDU used during UART/control investigation.

Family mappings in code are therefore working hypotheses supported by manuals and direct evidence from this limited sample. They are not blanket compatibility guarantees for every Toshiba unit with a similar family code.

## 4. Capability model

Capabilities are separated from the exact model string.

Current model families recognised by the experimental mapper are:

```text
UNKNOWN
J2FVG
G3KVSG
P2KVSG
```

The implementation uses a lightweight family capability matrix. Exact-model quantitative data such as airflow is kept separately in the output component.

Current feature vocabulary includes:

```text
common HVAC
ECO
Hi POWER
Comfort Sleep (0x94)
Power Select
Outdoor Silent / Silent Operation
Fireplace
8 °C heat
vertical airflow
horizontal airflow
Floor
air outlet select
HADA Care
Sleep (0xF7)
Comfort (0xF7)
PURE
Start Defrost
```

`0x94` Comfort Sleep is deliberately kept separate from the `0xF7` Comfort value until their relationship is established experimentally.

Unknown models fall back conservatively. Exact model identification can still be published even when a feature profile is not known.

## 5. Toshiba app grouping versus protocol grouping

The Toshiba app currently exposes a top `Fan Speed` section and a lower `Function` section.

### Fan Speed

The fan control is mutually exclusive at the UI level:

```text
Manual fan level
Auto
Quiet
```

The underlying command register remains `0xA0`:

```text
31 = Quiet
32..36 = manual levels 1..5
41 = Auto
```

The aggregate `0xF8 +2` byte mirrors the current fan command value. Captures while clicking the app controls showed `31`, `33` and `41`; `33` is one manual level, not a generic "manual" enum.

### Function

`Function` is an app/UI grouping, not one protocol field. Controls in the section are currently known to span multiple registers:

```text
Power Select       -> 0x87
PURE               -> 0xC7
Hi POWER / ECO /
Silent / 8 °C etc. -> F7 selector, also mirrored in F8 +3
Start Defrost      -> 0xCB write action
```

Therefore the Home Assistant model must not expose `Function` as one synthetic selector.

The established F7/F8 special-function values relevant to the directly tested P2 unit are:

```text
00 = Standard
01 = Hi POWER
02 = Silent 1
03 = ECO
0A = Silent 2
```

`Silent Operation` has three app states: Standard, Silent 1 and Silent 2. It affects outdoor-unit behaviour.

`PURE` is independent of the other Function controls in the observed app behaviour.

## 6. Validated P2KVSG operating-mode matrix

The following matrix is directly observed on the genuine Toshiba app connected to the `RAS-B10P2KVSGB-E`. It must not be copied to other families without validation.

| Function control | Auto | Cool | Heat | Dry | Fan |
| --- | ---: | ---: | ---: | ---: | ---: |
| Power Select | yes | yes | yes | yes | yes |
| ECO | yes | yes | yes | no | no |
| Hi POWER | yes | yes | yes | no | no |
| Silent Operation | yes | yes | yes | no | no |
| PURE | yes | yes | yes | yes | yes |
| 8 °C heat | no | no | yes | no | no |
| Start Defrost | yes | no | yes | no | no |

A further heat-only app function with a heater/radiator-style icon is visible but has not yet been named or mapped. It is deliberately omitted from the code feature matrix until identified.

### Fan availability by HVAC mode

On the same P2 unit:

| Fan choice | Auto | Cool | Heat | Dry | Fan |
| --- | ---: | ---: | ---: | ---: | ---: |
| Manual levels | yes | yes | yes | no | yes |
| Auto | yes | yes | yes | yes | yes |
| Quiet | yes | yes | yes | no | yes |

Dry mode therefore forces the app-side fan choice to Auto.

### Power Select compatibility rule

In Auto, Cool and Heat, changing Power Select to 100%, 75% or 50% cancels the other performance-related Function selections:

```text
ECO        -> cancelled
Hi POWER   -> cancelled
Silent 1/2 -> Standard
PURE       -> unchanged
```

In Dry and Fan, the app exposes only Power Select and PURE from this group, so there is no equivalent conflict to resolve.

This interaction is represented in code as a small family+mode cancellation mask. PURE is intentionally excluded because it is independent in observed behaviour.

## 7. Start Defrost

The Toshiba app exposes Start Defrost as a momentary Function action in Heat and Auto on the directly tested P2 unit.

Direct sniffer capture:

```text
WiFi -> IDU   register CB, value 02
IDU  -> WiFi  generic CB ACK
```

Working mapping:

```text
CB 02 = Start Defrost
```

This is an action/button, not a persistent switch state.

The `CD` and `CE` traffic seen immediately afterwards is ordinary energy-history polling and is unrelated to the defrost command.

## 8. Replacing the old preset abstraction

The legacy component presented register `0xF7` as one list of climate presets. That was convenient for the protocol but does not match the Toshiba app/control model.

The replacement logical entities remain divided controls, for example:

| Toshiba function | Intended entity |
| --- | --- |
| Standard | cancellation/base state where relevant |
| Hi POWER | switch |
| ECO | switch |
| Fireplace 1 / 2 | select: Off / Fireplace 1 / Fireplace 2 |
| 8 °C heat | switch |
| Silent Operation | select: Standard / Silent 1 / Silent 2 |
| Sleep | switch |
| Floor | switch |
| Comfort | switch |
| Power Select | select: 100% / 75% / 50% |
| PURE | switch |
| Start Defrost | button/action |

The protocol encoder can continue to use `0xF7` where appropriate, but the register is not the public logical-state model.

A received non-standard `0xF7` value is positive evidence for the function it identifies. It is **not automatically evidence that every other logical function is OFF** because independent functions can live in other registers. `STANDARD` is the explicit base state for the F7 special-function selector only.

Legacy `supported_presets` handling is retained temporarily as a compatibility path while the divided controls are tested.

## 9. UART evidence model

The project distinguishes ordinary request/response traffic from Toshiba-originated pushed traffic.

Observed classes include:

- class `0x10`: ordinary commands/requests sent by the ESP;
- class `0x90`: common response envelope to those requests;
- class `0x11`: unsolicited/pushed Toshiba status and equipment publications.

This distinction is important. A register can return sentinel values or fail to answer an active request while meaningful live values for the same logical register appear in pushed class-`0x11` traffic.

Therefore:

```text
active poll failed != feature absent
```

and:

```text
active poll returned sentinel != pushed telemetry is necessarily invalid
```

## 10. Engineering telemetry findings

Current working interpretation of extended status fields includes:

### `0xE4` IDU status

```text
+0 IDU heat-exchanger temperature
+1 junction/secondary temperature
+2 raw live fan/air-velocity feedback quantity — NOT literal RPM
+3..7 unresolved / commonly zero in observed frames
```

Physical vane-anemometer testing on the B13 console showed close correspondence between the raw `+2` value and approximately 0.1 m/s per count at the measured outlet position. That does not establish universal physical velocity calibration; outlet position, louvre state and profile matter.

### `0xE5` ODU/system status

Current working interpretation:

```text
+0 discharge temperature
+1 suction temperature
+2 ODU heat-exchanger temperature
+3 IDU-associated load/allocation-like value
+4/+5 unresolved, often zero
+6 current-like quantity
+7 unresolved
```

The exact physical scope of some fields remains unresolved. Labels should stay conservative until measurements justify stronger claims.

## 11. Cross-unit validation method

A key test has been moving the same ESP/UART adapter between different IDUs.

When missing/sentinel engineering telemetry followed the IDU rather than the adapter, the hypothesis of an ESP PCB/UART-interface fault was rejected. Conversely, the same newer adapter immediately reported rich `0xE4`/`0xE5` data when moved to the B13 console.

This test is preferred to inferring firmware capability from entity names, room labels or adapter identity.

## 12. Required control-driven test programme

Testing should be control-driven rather than register-driven.

For each reference unit:

1. Establish a stable baseline state.
2. Record all readable relevant UART state.
3. Press exactly one Toshiba app/remote/panel control.
4. Capture all UART traffic and changed registers.
5. Record physical behaviour and secondary state changes.
6. Try a known or suspected conflicting control.
7. Return to baseline.
8. Repeat to distinguish deterministic behaviour from transient traffic.

### J2FVG floor/console priorities

1. Air outlet selection in Heat.
2. Air outlet selection in Cool.
3. Air outlet behaviour in Dry.
4. Floor ON/OFF with a manual fan selected first.
5. Manual fan/swing request while Floor is active.
6. ECO and Hi POWER interaction.
7. Power Select interaction with ECO/Hi POWER/Silent.
8. Comfort Sleep.
9. Fireplace and 8 °C heat.
10. Outdoor Silent.

### High-wall priorities

1. Repeat the P2 mode/function matrix on G3KVSG before sharing rules across families.
2. Vertical fixed position and swing.
3. Horizontal fixed position/swing where physically supported.
4. Combined swing where supported.
5. HADA Care where documented/supported.
6. ECO and Hi POWER interaction.
7. Fireplace.
8. Outdoor Silent.
9. Power Select interactions.
10. Comfort/Sleep behaviour.

Older/different firmware units should be classified independently as READ_WRITE, READ_ONLY, WRITE_ONLY, UNSUPPORTED or UNKNOWN for each function where useful.

## 13. Configuration direction

Equipment identity should ultimately be discovered from `0xE0`; a hard-coded model should not be required for normal supported units. Explicit model configuration, if retained at all, should be an override/fallback for equipment that does not publish usable identity.

The runtime control path is intended to remain lightweight:

```text
E0 model string
   |
   +--> family --> family capability matrix --> mode availability/dependency rules
   |
   +--> exact model --> airflow/performance endpoints
```

No register-probing scan is required to infer the control feature set.

Independent feature entities belong under the Toshiba climate component in YAML, for example:

```yaml
climate:
  - platform: toshiba_suzumi
    name: "Toshiba A2A"
    uart_id: uart_bus

    idu_model:
      name: "IDU Model"
    odu_model:
      name: "ODU Model"

    eco:
      name: "ECO"
    hi_power:
      name: "Hi POWER"
    fireplace:
      name: "Fireplace"
    outdoor_silent:
      name: "Silent Operation"
    eight_degree_heat:
      name: "8 Degree Heat"
    sleep:
      name: "Sleep"
    floor:
      name: "Floor"
    comfort:
      name: "Comfort"
```

During the current development stage this example deliberately shows the complete divided set for testing. It does **not** mean every IDU supports every item.

## 14. Migration strategy

1. **Equipment identity** — decode/publish `0xE0` without requiring diagnostic mode.
2. **Capability foundation** — derive family from the detected model and use a compact family feature matrix.
3. **Quantitative model data** — keep exact-model airflow/performance endpoints separate from feature capability.
4. **Independent controls** — replace the false single-preset UI while keeping a compatibility shim.
5. **HVAC-mode matrix** — derive available controls and dependencies from Toshiba manuals and physical testing.
6. **Readback/conflict rules** — encode only interactions that are supported by repeatable evidence.
7. **Compatibility expansion** — accept captures and reports from additional Toshiba models and firmware generations.
8. **Cleanup** — remove obsolete preset abstractions only after the replacement model is stable.

## 15. Standard of evidence

The project should clearly distinguish:

- **directly tested** — reproduced on physical hardware available to the project;
- **documented/mapped** — supported by Toshiba documentation and/or strong protocol evidence but not necessarily tested on every model;
- **inferred** — plausible interpretation still requiring validation;
- **unknown** — insufficient evidence.

Do not upgrade an inference into a protocol fact merely because it gives a convenient Home Assistant entity name.
