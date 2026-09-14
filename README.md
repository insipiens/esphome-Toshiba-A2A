# ESPHome Toshiba A2A

ESPHome component and protocol research project for controlling and monitoring Toshiba residential air-to-air heat-pump indoor units over the internal UART interface used by Toshiba Wi-Fi/remote accessories.

> **Work in progress.** This repository is being developed from measurements on a small number of real Toshiba systems. It is **not** a claim of universal compatibility across Toshiba residential air-conditioning products. Model, firmware and regional differences matter, and features are being marked as confirmed, inferred or unknown as evidence is collected.

## Project origin, licence and attribution

This project is a derived work of **[pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi)**. That repository provided the original ESPHome Toshiba UART implementation on which this project started, including the core communication approach, climate component structure, Toshiba command/register handling and a substantial body of protocol knowledge.

The present project has diverged considerably and is undergoing a broad redesign, but it would not exist in its present form without that work. Credit for the original implementation belongs to **pedobry and the contributors to `esphome_toshiba_suzumi`**.

The original project's own references and acknowledgements are also retained as part of the technical lineage:

- **[toremick/shorai-esp32](https://github.com/toremick/shorai-esp32)** — referenced by the original project for the Toshiba connection-adapter approach.
- **[Vpowgh/TConnect](https://github.com/Vpowgh/TConnect)** — referenced by the original project as related Toshiba protocol/control work.

The original repository is licensed under the **GNU General Public License v3.0**. This derived project retains that licence. See [LICENSE](LICENSE) and [PROVENANCE.md](PROVENANCE.md).

This repository should therefore be read as a continuation and substantial rewrite of earlier open-source work, not as a clean-room or unrelated implementation.

## Why this repository exists

The original component successfully established practical ESPHome control of Toshiba units, but investigation of several different indoor-unit families exposed limitations in treating the protocol as one generic set of Home Assistant climate presets and sensors.

The redesign has three main goals:

1. represent Toshiba controls in Home Assistant in the same conceptual form as the physical Toshiba remote/panel rather than forcing unrelated functions into one preset selector;
2. identify the connected indoor unit and expose only controls that are credible for that model/family and operating mode;
3. separate confirmed protocol facts from model-specific behaviour and from assumptions that still require testing.

The intended architecture is:

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
- IDU/ODU terminology in place of inherited FCU/CDU naming;
- extended `0xE4` IDU and `0xE5` ODU/system telemetry handling;
- established interpretation of `E4 +2` as live IDU fan-speed feedback at approximately 10 rpm per count (RPM/10) on the tested units, with model-specific airflow mapping derived separately from fan/air-path geometry;
- passive raw UART capture and diagnostic tooling for observing Toshiba-originated traffic without injecting scan requests;
- controlled active register investigation where appropriate;
- Home Assistant entities for equipment identity and engineering telemetry;
- continued investigation of energy, current and performance-related fields against independent measurements;
- a control-matrix design intended to encode which controls Toshiba makes available in Heat, Cool, Dry, Fan and Auto modes, including forced dependencies.

The component directory is still named `toshiba_suzumi` at this stage to avoid an unnecessary breaking change while the redesign is underway. The repository scope is intentionally broader than Suzumi.

## Investigation method

Development is based on repeatable observation rather than assuming that similarly named Toshiba models or registers behave identically.

The process used so far includes:

- cross-referencing Toshiba operation, remote-control, service and engineering documentation;
- using the physical Toshiba remote/panel as the reference for the user-facing control taxonomy;
- capturing raw UART traffic while changing exactly one physical control at a time;
- comparing ordinary class-`0x10` requests/responses with unsolicited or pushed class-`0x11` publications;
- scanning registers only when useful and safe rather than assuming every meaningful value can be actively polled;
- moving the same ESP/UART adapter between indoor units to distinguish ESP hardware problems from IDU/model/firmware behaviour;
- comparing different capacity variants and firmware generations of the same broad indoor-unit family;
- checking telemetry against physical behaviour and independent measurements, including external electrical metering where available;
- retaining anomalous results instead of normalising them away — for example, some units have been observed to publish `NULL` in the IDU model field of an otherwise valid `0xE0` equipment-identification message;
- treating unknown fields as unknown until repeatable evidence supports a stronger interpretation.

## Hardware and platform actually tested

The rewrite has been developed against one real multi-split installation rather than a broad Toshiba laboratory fleet.

Directly available HVAC hardware includes:

- **RAS-5M34G3AVG-E1** multi-split outdoor unit;
- **RAS-B13J2FVG-E1** floor/console IDU;
- **RAS-B10J2FVG-family** floor/console IDUs, including units showing older/different firmware behaviour;
- **RAS-B10P2KVSG-E** high-wall IDU, identified directly through the `0xE0` equipment-identification message;
- a **RAS-B10G3KVSG-family** high-wall IDU used during UART/control investigation.

Current development hardware is **ESP32**, primarily **ESP32-C3 SuperMini-based adapters**. Other ESP platforms may still work through inherited ESPHome compatibility, but this project does not currently claim or document them as tested. In particular, the inherited ESP8266 example has been removed because ESP8266 has not been tested by this project.

Those HVAC units are enough to prove that significant behaviour differs by IDU/model/firmware, but they are **not** enough to assert that every J2FVG, P2KVSG, G3KVSG, Shorai, Seiya, Suzumi, Daiseikai or other Toshiba family behaves identically.

The original `esphome_toshiba_suzumi` project lists a substantially wider set of units believed compatible with the Toshiba RB-N105S-G/RB-N106S-G interface. That upstream compatibility list remains useful prior art, but this repository does not re-label those models as independently tested here.

## Findings that currently shape the design

- The `0xE0` class-`0x11` message can contain the IDU model and ODU model in two fixed equipment records and is used as the working basis for automatic model identification.
- A valid `0xE0` message may contain `NULL` for the IDU model on some units/firmware. The project reports model unavailable rather than inventing an identity.
- `0xE4` and `0xE5` engineering/status data can differ dramatically between indoor units connected to the same outdoor system.
- `E4 +2` is live indoor-fan speed feedback at approximately 10 rpm/count on the tested units. The higher values seen on high-wall units such as the P2KVSGB represent genuinely higher blower RPM, not a different UART scale; airflow conversion remains model-specific.
- On some units a field may return sentinel values when actively polled yet appear with meaningful live values in Toshiba-pushed class-`0x11` traffic. "Register did not answer a poll" is therefore not equivalent to "feature does not exist".
- Cross-testing the same ESP adapter on different IDUs showed that some missing engineering telemetry follows the indoor unit/controller rather than the ESP hardware.
- The historical `0xF7` values are a protocol encoding, not a good Home Assistant UI model. Toshiba presents functions such as ECO, Hi POWER, Fireplace, Outdoor Silent and Floor as distinct controls even when their protocol representation shares a register.

## Current control-model work

The former `supported_presets` mechanism is being replaced by divided controls:

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

Legacy `supported_presets` / `special_mode` handling remains in the component only as a migration compatibility path. It is not the preferred configuration model for this repository and is intentionally omitted from current examples.

The next stage is to complete the **model × HVAC-mode × available-control** matrix from Toshiba manuals and physical-unit testing. See [TOSHIBA_CONTROL_MATRIX.md](TOSHIBA_CONTROL_MATRIX.md).

## Installation during development

Use [example.yaml](example.yaml) as the current minimal example. It is deliberately limited to normal climate operation and directly useful entities.

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/insipiens/esphome-Toshiba-A2A
      ref: main
    components: [toshiba_suzumi]
    refresh: 1min
```

The example uses an ESP32-C3 build target because that reflects current project hardware. Its GPIO assignments are examples only and must match the user's actual adapter wiring.

Additional examples are separated by purpose rather than being mixed into the normal configuration:

- [examples/diagnostic_capture.yaml](examples/diagnostic_capture.yaml) — passive raw UART research capture; not needed for normal use;
- [examples/engineering_telemetry.yaml](examples/engineering_telemetry.yaml) — E4/E5 engineering/status sensors whose availability varies by IDU/model/firmware;
- [examples/output_estimation.yaml](examples/output_estimation.yaml) — experimental sensible-output estimator calibrated on the B13J2FVG reference unit, not a generic Toshiba output or COP model.

## Hardware interface and safety

The Toshiba accessory connector carries power and UART-level signals. The original project documents the RB-N105S-G/RB-N106S-G style connection and the use of an appropriate logic-level interface between the Toshiba unit and a 3.3 V ESP device.

**Do not assume pin order from this README alone. Check the connector, unit documentation and the original project before wiring. Disconnect mains power from the indoor unit before connecting or disconnecting an ESP interface. Incorrect wiring can damage the indoor-unit control board.**

The original wiring documentation and photographs remain available in the [upstream project](https://github.com/pedobry/esphome_toshiba_suzumi). They are referenced rather than being presented here as new work.

## Compatibility status

There are three distinct levels of evidence in this project:

- **Directly tested:** behaviour observed on the physical units listed above.
- **Documented/mapped:** behaviour supported by Toshiba manuals or strong protocol evidence but not necessarily exercised on every model.
- **Unknown:** behaviour not yet established, especially where firmware generations differ or a unit returns sentinel/`NULL` data.

Please do not interpret a family name in the capability code as a blanket compatibility guarantee.

## Contributing test evidence

Useful reports include exact IDU and ODU model numbers, firmware/version information where available, physical remote model, ESPHome version, raw UART captures showing the action that caused the traffic, before/after values for relevant registers, whether a value came from active polling or unsolicited Toshiba traffic, and physical behaviour observed at the unit.

Where possible, change one control at a time and repeat the test. A reproducible anomaly is more useful than a guessed decoding.

## Licence

GNU General Public License v3.0. See [LICENSE](LICENSE).

This repository contains modified source derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi). The changes in this repository are intentionally described as modifications and extensions of that work.
