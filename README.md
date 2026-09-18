# ESPHome Toshiba A2A

ESPHome support for controlling and monitoring Toshiba residential air-to-air heat-pump indoor units over the internal UART interface used by Toshiba Wi-Fi accessories.

This project is under active development and is based on testing real Toshiba units. Model and firmware differences matter, so compatibility should not be assumed beyond the units and families documented here.

## Currently tested

- `RAS-B13J2FVG-E1` floor/console IDU
- `RAS-B10J2FVG` family floor/console IDUs
- `RAS-B10P2KVSGB-E` high-wall IDU
- `RAS-5M34G3AVG-E1` multi-split outdoor unit
- ESP32-C3 / ESPHome

## Quick start

Build or obtain a suitable level-shifted interface, then copy the universal
installation template and set the exact indoor-unit model. Detailed electrical,
flashing and verification steps are in [the installation guide](docs/INSTALLATION.md).

Use the single universal package for all supported Toshiba IDU families. The
consumer supplies only a coherent set of installation substitutions:

```yaml
substitutions:
  device_name: "toshiba-a2a-01"
  friendly_name: "toshiba-A2A-MR"
  climate_entity_name: "Music Room"
  toshiba_model: "RAS-B13J2FVG-E"

packages:
  toshiba_a2a:
    url: https://github.com/insipiens/esphome-Toshiba-A2A
    ref: main
    files:
      - packages/toshiba-a2a.yaml
    refresh: 0s
```

The component resolves the protocol family and default capabilities internally
from `toshiba_model`. `device_name` is the ESPHome/network identity,
`friendly_name` is the Home Assistant device name, and `climate_entity_name` is the
climate entity name. `climate_entity_name` may be left blank (`""`) if you prefer
Home Assistant's composed naming. There are no family-specific installation
templates.

If testing shows that an older/reduced IDU controller exposes a feature that
does not actually work, disable only that feature in the consuming YAML. For
example:

```yaml
climate:
  - id: !extend room_id
    disable_features:
      - fixed_position
```

See [examples/toshiba-a2a-template.yaml](examples/toshiba-a2a-template.yaml)
and [Usage and options](docs/USAGE.md).

## What it provides

The component exposes the normal climate controls plus Toshiba-specific functions such as Power Select, ECO, Hi POWER, Silent operation, Fireplace, 8 °C heat, Floor mode, PURE, louvre/FIX controls where supported, engineering temperatures, fan feedback, energy data and model identification. The universal package also exposes model-specific IDU airflow and estimated sensible heating/cooling output for models with an airflow calibration; unsupported models report those derived values as unavailable. A restored `IDU Output Multiplier` number (`0.00`–`1.00`) permits later calibration without recompiling.

Not every function is available on every model or in every HVAC mode.

## Documentation

Start here:

- [Installation](docs/INSTALLATION.md) — flash, connect and verify a controller
- [Usage and options](docs/USAGE.md) — substitutions, entities, feature overrides and output calibration
- [Controller hardware](docs/HARDWARE.md) — tested BOM, populated-board photograph and corrected Gerbers
- [Passive UART sniffer](docs/SNIFFER.md) — receive-only capture of a genuine adaptor
- [Adding support for a model](docs/ADDING_A_MODEL.md) — evidence and implementation workflow
- [Examples](examples/README.md) — which configuration to use and why

For the current implementation, tested behaviour, known limitations and active
investigations, see [current_status.md](current_status.md).

Detailed protocol material is kept separately:

- [TOSHIBA_CONTROL_MATRIX.md](TOSHIBA_CONTROL_MATRIX.md) — controls by family and HVAC mode
- [TOSHIBA_REGISTER_MAP.md](TOSHIBA_REGISTER_MAP.md) — UART register/protocol findings
- [J2_MANUAL_CONTROL_NOTES.md](J2_MANUAL_CONTROL_NOTES.md) — J2 manual-derived notes
- [CHANGELOG.txt](CHANGELOG.txt) — development history

## Hardware caution

The Toshiba accessory connector carries power and UART signals. Do not assume
connector pin order or logic levels from a photograph. Verify the unit and
interface before wiring, and disconnect mains power before connecting or
disconnecting the ESP interface. See [Controller hardware](docs/HARDWARE.md).

## Origin and licence

This project is derived from [pedobry/esphome_toshiba_suzumi](https://github.com/pedobry/esphome_toshiba_suzumi) and retains its GPLv3 licence. The original project also references related work by [toremick/shorai-esp32](https://github.com/toremick/shorai-esp32) and [Vpowgh/TConnect](https://github.com/Vpowgh/TConnect).

See [PROVENANCE.md](PROVENANCE.md) and [LICENSE](LICENSE) for full attribution and licence information.
