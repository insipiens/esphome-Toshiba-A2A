# ESPHome Toshiba A2A

ESPHome component and protocol research project for controlling and monitoring Toshiba residential air-to-air heat-pump indoor units over the internal UART interface used by Toshiba Wi-Fi/remote accessories.

> **Work in progress.** This repository is being developed from measurements on a small number of real Toshiba systems. It is **not** a claim of universal compatibility across Toshiba residential air-conditioning products. Model, firmware and regional differences matter, and features are being marked as confirmed, inferred or unknown as evidence is collected.

## Project origin, licence and attribution

This project is a derived work of **[pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi)**. That repository provided the original ESPHome Toshiba UART implementation on which this project started, including the core communication approach, climate component structure, Toshiba command/register handling and a substantial body of protocol knowledge.

The present project has diverged considerably and is undergoing a broad redesign, but it would not exist in its present form without that work. Credit for the original implementation belongs to **pedobry and the contributors to `esphome_toshiba_suzumi`**.

The original project's own references and acknowledgements are also retained as part of the technical lineage:

- **[toremick/shorai-esp32](https://github.com/toremick/shorai-esp32)** — referenced by the original project for the Toshiba connection-adapter approach.
- **[Vpowgh/TConnect](https://github.com/Vpowgh/TConnect)** — referenced by the original project as related Toshiba protocol/control work.

The original repository is licensed under the **GNU General Public License v3.0**. This derived project retains that licence. See [LICENSE](LICENSE).

This repository should therefore be read as a continuation and substantial rewrite of earlier open-source work, not as a clean-room or unrelated implementation.

## Why this repository exists

The original component successfully established practical ESPHome control of Toshiba units, but investigation of several different indoor-unit families exposed limitations in treating the protocol as one generic set of Home Assistant climate presets and sensors.

The redesign has three main goals:

1. represent Toshiba controls in Home Assistant in the same conceptual form as the physical Toshiba remote/panel rather than forcing unrelated functions into one preset selector;
2. identify the connected indoor unit and expose only controls that are credible for that model/family and operating mode;
3. separate confirmed protocol facts from model-specific behaviour and from assumptions that still require testing.

The long-term architecture is intended to be:

```text
UART packet
   |
   v
protocol decoder
   |
   v
logical Toshiba state
   |
   v
model capability + operating-mode rules
   |
   v
ESPHome entities
   |
   v
Home Assistant
```

A UART register is therefore treated as a transport/protocol detail, not automatically as the correct Home Assistant UI abstraction.

## What has changed from the original component

The current rewrite includes, or is in the process of introducing:

- decoding of pushed class-`0x11` Toshiba messages as well as ordinary request/response traffic;
- decoding of the `0xE0` equipment-identification message and publication of IDU and ODU model strings;
- model/family capability profiles instead of assuming every Toshiba unit supports the same functions;
- a shared residential capability vocabulary with model-specific additions rather than one monolithic profile per family;
- separation of the old `0xF7` "special mode" preset list into independent Home Assistant controls such as ECO, Hi POWER, Fireplace, Outdoor Silent, 8 °C heat, Sleep, Floor and Comfort;
- fixed vertical-air-direction control in addition to ESPHome's standard swing modes;
- improved terminology: IDU/ODU is preferred over the older FCU/CDU naming;
- extended `0xE4` IDU and `0xE5` ODU/system telemetry handling;
- correction of interpretations that measurements showed were too strong — for example, the IDU fan field is retained as a raw/live fan feedback or nominal air-velocity quantity and is **not** claimed to be literal RPM;
- passive raw UART capture and diagnostic tooling for observing Toshiba-originated traffic without injecting scan requests;
- active register-scanning tools for controlled protocol investigation where appropriate;
- Home Assistant entities for equipment identity and engineering telemetry;
- continued investigation of energy, current and performance-related fields against independent electrical measurements;
- a control-matrix design intended to encode which controls Toshiba makes available in Heat, Cool, Dry, Fan and Auto modes, including forced dependencies such as functions that make the unit select Fan Auto.

The component directory is still named `toshiba_suzumi` at this stage to avoid an unnecessary breaking change while the redesign is underway. The repository scope is intentionally broader than Suzumi.

## Investigation method

Development is based on repeatable observation rather than assuming that similarly named Toshiba models or registers behave identically.

The process used so far includes:

- cross-referencing Toshiba operation, remote-control, service and engineering documentation;
- using the physical Toshiba remote/panel as the reference for the user-facing control taxonomy;
- capturing raw UART traffic while changing exactly one physical control at a time;
- comparing ordinary class-`0x10` requests/responses with unsolicited or pushed class-`0x11` publications;
- scanning registers only when doing so is useful and safe, rather than assuming every meaningful value can be actively polled;
- moving the same ESP/UART adapter between indoor units to distinguish ESP hardware problems from IDU/model/firmware behaviour;
- comparing different capacity variants and firmware generations of the same broad indoor-unit family;
- checking telemetry against physical behaviour and independent measurements, including external electrical metering where available;
- retaining anomalous results instead of normalising them away — for example, some units have been observed to publish `NULL` in the IDU model field of an otherwise valid `0xE0` equipment-identification message;
- treating unknown fields as unknown until repeatable evidence supports a stronger interpretation.

This matters because Toshiba units can expose the same normal climate controls while differing substantially in the engineering telemetry or pushed status data they make available.

## Hardware actually available to this project

The rewrite has been developed against one real multi-split installation rather than a broad laboratory collection of Toshiba products.

The outdoor unit available for direct testing is:

- **RAS-5M34G3AVG-E1** multi-split ODU.

Indoor units directly observed during development include:

- **RAS-B13J2FVG-E1** floor/console unit;
- **RAS-B10J2FVG-family** floor/console units, including units showing older/different firmware behaviour;
- **RAS-B10P2KVSG-E** high-wall unit, identified directly through the `0xE0` equipment-identification message;
- a **RAS-B10G3KVSG-family** high-wall unit used during UART/control investigation.

Those units are enough to prove that significant behaviour differs by IDU/model/firmware, but they are **not** enough to assert that every J2FVG, P2KVSG, G3KVSG, Shorai, Seiya, Suzumi, Daiseikai or other Toshiba family behaves identically.

The original `esphome_toshiba_suzumi` project lists a substantially wider set of units believed compatible with the Toshiba RB-N105S-G/RB-N106S-G interface. That upstream compatibility list remains useful prior art, but this repository does not re-label those models as independently tested here.

## Findings that currently shape the design

Some results have been particularly important to the rewrite:

- The `0xE0` class-`0x11` message can contain the IDU model and ODU model in two fixed equipment records. This is now used as the working basis for automatic model identification.
- A valid `0xE0` message may contain `NULL` for the IDU model on some units/firmware. The project treats this as "model unavailable" rather than substituting the ODU model or inventing an identity.
- `0xE4` and `0xE5` engineering/status data can differ dramatically between indoor units connected to the same outdoor system.
- On some units a field may return sentinel values when actively polled yet appear with meaningful live values in Toshiba-pushed class-`0x11` traffic. Therefore "register did not answer a poll" is not equivalent to "feature does not exist".
- Cross-testing the same ESP adapter on different IDUs showed that some missing engineering telemetry follows the indoor unit/controller rather than the ESP hardware.
- The historical `0xF7` values are a protocol encoding, not a good Home Assistant UI model. Toshiba presents functions such as ECO, Hi POWER, Fireplace, Outdoor Silent and Floor as distinct controls, even when their protocol representation shares a register.

## Current control-model work

The former `supported_presets` mechanism is being replaced by divided controls. Current development exposes the existing `0xF7` set conceptually as:

| Toshiba function | Home Assistant representation |
| --- | --- |
| Standard | absence/cancellation of a special function; no dedicated control |
| Hi POWER | switch |
| ECO | switch |
| Fireplace 1 / 2 | select: Off / Fireplace 1 / Fireplace 2 |
| 8 °C heat | switch |
| Silent 1 / 2 | select: Off / Silent 1 / Silent 2 |
| Sleep | switch |
| Floor | switch |
| Comfort | switch |

This is still being validated. A shared register does not prove that all of these logical functions are mutually exclusive, and receiving one value must not be used to fabricate false OFF states for unrelated controls without evidence from the unit.

The next stage is to complete the **model × HVAC-mode × available-control** matrix from Toshiba manuals and physical-unit testing. For example, Floor and 8 °C heat are not meaningful in every HVAC mode, and some functions force secondary state changes such as Fan Auto.

See [TOSHIBA_CONTROL_MATRIX.md](TOSHIBA_CONTROL_MATRIX.md) for the design and test programme.

## Installation during development

Until the internal component is renamed, ESPHome configuration should continue to load `toshiba_suzumi` from this repository:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/insipiens/esphome-Toshiba-A2A
      ref: main
    components: [toshiba_suzumi]
    refresh: 1min

uart:
  id: uart_bus
  tx_pin: 4       # example only - use the pins appropriate to your hardware
  rx_pin: 5       # example only - use the pins appropriate to your hardware
  parity: EVEN
  baud_rate: 9600

climate:
  - platform: toshiba_suzumi
    name: "Toshiba A2A"
    id: toshiba_a2a
    uart_id: uart_bus

    idu_model:
      name: "IDU Model"
    odu_model:
      name: "ODU Model"

    power_select:
      name: "Power Select"
    vertical_air_direction:
      name: "Vertical Air Direction"

    eco:
      name: "ECO"
    hi_power:
      name: "Hi POWER"
    fireplace:
      name: "Fireplace"
    eight_degree_heat:
      name: "8 Degree Heat"
    outdoor_silent:
      name: "Outdoor Silent"
    sleep:
      name: "Sleep"
    floor:
      name: "Floor"
    comfort:
      name: "Comfort"
```

Not every entity in that example is appropriate for every IDU. Automatic capability- and operating-mode-driven presentation is part of the current redesign. During development, configure and test conservatively.

## Hardware interface and safety

The Toshiba accessory connector carries power and UART-level signals. The original project documents the RB-N105S-G/RB-N106S-G style connection and the use of an appropriate logic-level interface between the Toshiba unit and a 3.3 V ESP device.

**Do not assume pin order from this README alone. Check the connector, unit documentation and the original project before wiring. Disconnect mains power from the indoor unit before connecting or disconnecting an ESP interface. Incorrect wiring can damage the indoor-unit control board.**

The original wiring documentation and photographs remain valuable and are retained in this repository with attribution to the source project.

## Compatibility status

There are three distinct levels of evidence in this project:

- **Directly tested:** behaviour observed on the physical units listed above.
- **Documented/mapped:** behaviour supported by Toshiba manuals or strong protocol evidence but not necessarily exercised on every model.
- **Unknown:** behaviour not yet established, especially where firmware generations differ or a unit returns sentinel/`NULL` data.

Please do not interpret a family name in the capability code as a blanket compatibility guarantee. Reports and raw captures from other Toshiba models are useful precisely because the current test population is small.

## Contributing test evidence

Useful reports include:

- exact IDU and ODU model numbers;
- firmware/version information if the unit exposes it;
- the physical remote model;
- ESPHome version;
- raw UART captures showing the action that caused the traffic;
- before/after values for the relevant registers;
- whether a value was obtained by active polling or unsolicited Toshiba traffic;
- physical behaviour observed at the unit.

Where possible, change one control at a time and repeat the test. A reproducible anomaly is more useful than a guessed decoding.

## Licence

GNU General Public License v3.0. See [LICENSE](LICENSE).

This repository contains modified source derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi). The changes in this repository are intentionally described as modifications and extensions of that work.
