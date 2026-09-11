# Toshiba control matrix redesign

## Status

**Work in progress.** This document describes the architecture and test programme being used to replace the historical one-register/one-UI-control assumptions in the component. It records current evidence; it is not a claim that all Toshiba residential IDUs implement every function listed here.

The project is derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi). The original project established the practical ESPHome/UART implementation on which this work began.

## 1. Design principle

The Toshiba remote control and indoor-unit panel are the reference for the user-facing control taxonomy.

The UART protocol is an implementation detail. Two controls sharing a register or value space does not, by itself, prove that they are one mutually-exclusive user function.

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
model capability + HVAC-mode rules
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
- `RAS-B10P2KVSG-E` high-wall IDU identified through `0xE0`;
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

The implementation starts with a shared Toshiba residential feature vocabulary and adds model/family-specific features. This is preferred to treating each family as an isolated control universe.

Current feature vocabulary includes:

```text
common HVAC
ECO
Hi POWER
Comfort Sleep (0x94)
Power Select
Outdoor Silent
Fireplace
8 °C heat
vertical airflow
horizontal airflow
Floor
air outlet select
HADA Care
Sleep (0xF7)
Comfort (0xF7)
```

`0x94` Comfort Sleep is deliberately kept separate from the `0xF7` Comfort value until their relationship is established experimentally.

Unknown models fall back conservatively. Exact model identification can still be published even when a feature profile is not known.

## 5. Replacing the old preset abstraction

The legacy component presented register `0xF7` as one list of climate presets. That was convenient for the protocol but does not match the physical Toshiba controls.

The replacement logical entities are:

| Toshiba function | Intended entity |
| --- | --- |
| Standard | no dedicated entity; cancellation/base state |
| Hi POWER | switch |
| ECO | switch |
| Fireplace 1 / 2 | select: Off / Fireplace 1 / Fireplace 2 |
| 8 °C heat | switch |
| Silent 1 / 2 | select: Off / Silent 1 / Silent 2 |
| Sleep | switch |
| Floor | switch |
| Comfort | switch |

The protocol encoder can continue to use `0xF7`, but the register is not the public logical-state model.

A received non-standard `0xF7` value is positive evidence for the function it identifies. It is **not automatically evidence that every other logical function is OFF**. Until Toshiba behaviour is established, unrelated entity states must not be fabricated merely because another `0xF7` value appeared. `STANDARD` is currently treated as the explicit base/cancellation value.

Legacy `supported_presets` handling is retained temporarily as a compatibility path while the divided controls are tested.

## 6. Operating-mode matrix — next stage

The next control layer is the relationship between HVAC mode and the controls Toshiba makes available on the physical remote.

The matrix must be established for:

```text
Auto
Cool
Heat
Dry
Fan
Off / standby where relevant
```

For each model/family and each HVAC mode, record:

- whether the remote offers the control;
- whether the UART accepts the command;
- whether the state can be read back;
- whether the function forces another setting;
- whether another control cancels it;
- whether firmware variants behave differently.

Examples requiring explicit confirmation include:

- Floor availability and its forced Fan Auto behaviour;
- 8 °C heat restrictions;
- ECO versus Hi POWER;
- Power Select interactions;
- Fireplace restrictions;
- Outdoor Silent availability;
- Comfort/Sleep behaviour;
- fixed and swinging louvre options by mode;
- console air-outlet selection by mode.

The Home Assistant presentation should eventually follow this matrix rather than simply exposing every syntactically valid UART command at all times.

## 7. UART evidence model

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

## 8. Engineering telemetry findings

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

## 9. Cross-unit validation method

A key test has been moving the same ESP/UART adapter between different IDUs.

When missing/sentinel engineering telemetry followed the IDU rather than the adapter, the hypothesis of an ESP PCB/UART-interface fault was rejected. Conversely, the same newer adapter immediately reported rich `0xE4`/`0xE5` data when moved to the B13 console.

This test is preferred to inferring firmware capability from entity names, room labels or adapter identity.

## 10. Required control-driven test programme

Testing should be control-driven rather than register-driven.

For each reference unit:

1. Establish a stable baseline state.
2. Record all readable relevant UART state.
3. Press exactly one Toshiba remote/panel control.
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
7. Power Select interaction with ECO/Hi POWER.
8. Comfort Sleep.
9. Fireplace and 8 °C heat.
10. Outdoor Silent.

### High-wall priorities

1. Vertical fixed position and swing.
2. Horizontal fixed position/swing where physically supported.
3. Combined swing where supported.
4. HADA Care where documented/supported.
5. ECO and Hi POWER interaction.
6. Fireplace.
7. Outdoor Silent.
8. Power Select interactions.
9. 8 °C heat restrictions.
10. Comfort/Sleep behaviour.

Older/different firmware units should be classified independently as READ_WRITE, READ_ONLY, WRITE_ONLY, UNSUPPORTED or UNKNOWN for each function where useful.

## 11. Configuration direction

Equipment identity should ultimately be discovered from `0xE0`; a hard-coded model should not be required for normal supported units. Explicit model configuration, if retained at all, should be an override/fallback for equipment that does not publish usable identity.

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
      name: "Outdoor Silent"
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

## 12. Migration strategy

1. **Equipment identity** — decode/publish `0xE0` without requiring diagnostic mode.
2. **Capability foundation** — map exact model to a conservative feature profile.
3. **Independent controls** — replace the false single-preset UI while keeping a compatibility shim.
4. **HVAC-mode matrix** — derive available controls and dependencies from Toshiba manuals and physical testing.
5. **Readback/conflict rules** — encode only interactions that are supported by repeatable evidence.
6. **Compatibility expansion** — accept captures and reports from additional Toshiba models and firmware generations.
7. **Cleanup** — remove obsolete preset abstractions only after the replacement model is stable.

## 13. Standard of evidence

The project should clearly distinguish:

- **directly tested** — reproduced on physical hardware available to the project;
- **documented/mapped** — supported by Toshiba documentation and/or strong protocol evidence but not necessarily tested on every model;
- **inferred** — plausible interpretation still requiring validation;
- **unknown** — insufficient evidence.

Do not upgrade an inference into a protocol fact merely because it gives a convenient Home Assistant entity name.
