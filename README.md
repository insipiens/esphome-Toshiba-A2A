# ESPHome Toshiba A2A

ESPHome support for controlling and monitoring Toshiba residential air-to-air heat-pump indoor units over the internal UART interface used by Toshiba Wi-Fi accessories.

This project is under active development and is based on testing real Toshiba units. Model and firmware differences matter, so compatibility should not be assumed beyond the units and families documented here.

## Currently tested

- `RAS-B13J2FVG-E1` floor/console IDU
- `RAS-B10J2FVG` family floor/console IDUs
- `RAS-B10P2KVSGB-E` high-wall IDU
- `RAS-5M34G3AVG-E1` multi-split outdoor unit
- ESP32-C3 / ESPHome

## Installation

Use the single universal package for all supported Toshiba IDU families. The
consumer supplies only a coherent set of installation substitutions:

```yaml
substitutions:
  device_name: "toshiba-a2a"
  friendly_name: "Toshiba Air Conditioner"
  toshiba_model: "RAS-B10J2FVG-E"

packages:
  toshiba_a2a:
    url: https://github.com/insipiens/esphome-Toshiba-A2A
    ref: main
    files:
      - packages/toshiba-a2a.yaml
    refresh: 0s
```

The component resolves the protocol family and default capabilities internally
from `toshiba_model`. There are no family-specific installation templates.

If testing shows that an older/reduced IDU controller exposes a feature that
does not actually work, disable only that feature in the consuming YAML. For
example:

```yaml
climate:
  - id: room_id
    disable_features:
      - fixed_position
```

See [examples/toshiba-a2a-template.yaml](examples/toshiba-a2a-template.yaml).

## What it provides

The component exposes the normal climate controls plus Toshiba-specific functions such as Power Select, ECO, Hi POWER, Silent operation, Fireplace, 8 °C heat, Floor mode, PURE, louvre/FIX controls where supported, engineering temperatures, fan feedback, energy data and model identification.

Not every function is available on every model or in every HVAC mode.

## Documentation

For the current implementation, tested behaviour, known limitations and active investigations, see [current_status.md](current_status.md).

Detailed protocol material is kept separately:

- [TOSHIBA_CONTROL_MATRIX.md](TOSHIBA_CONTROL_MATRIX.md) — controls by family and HVAC mode
- [TOSHIBA_REGISTER_MAP.md](TOSHIBA_REGISTER_MAP.md) — UART register/protocol findings
- [J2_MANUAL_CONTROL_NOTES.md](J2_MANUAL_CONTROL_NOTES.md) — J2 manual-derived notes
- [CHANGELOG.txt](CHANGELOG.txt) — development history

## Hardware caution

The Toshiba accessory connector carries power and UART signals. Do not assume connector pin order or logic levels from this README. Verify the unit and interface before wiring, and disconnect mains power before connecting or disconnecting the ESP interface.

## Origin and licence

This project is derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi) and retains its GPLv3 licence. The original project also references related work by [toremick/shorai-esp32](https://github.com/toremick/shorai-esp32) and [Vpowgh/TConnect](https://github.com/Vpowgh/TConnect).

See [PROVENANCE.md](PROVENANCE.md) and [LICENSE](LICENSE) for full attribution and licence information.
