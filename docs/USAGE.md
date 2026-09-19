# Usage and options

Normal installations should include the universal package rather than copying
the component definition. The package resolves the control family from the
configured exact IDU model and creates the controller, controls, diagnostics
and supported output estimates.

## Package substitutions

| Substitution | Purpose | Example |
|---|---|---|
| `device_name` | ESPHome node and network identity | `toshiba-a2a-living-room` |
| `friendly_name` | Home Assistant device name | `Toshiba Living Room` |
| `climate_entity_name` | Climate entity name; `""` permits composed naming | `Living Room` |
| `toshiba_model` | Exact indoor-unit model and startup profile | `RAS-B10J2FVG-E` |

The model is deliberately not guessed from a broad product description.
Capacity variants can share a control family while having different airflow
data, so retain the complete IDU model including suffixes.

## Main entities

The package provides:

- a climate entity for power, HVAC mode, setpoint, fan mode and ordinary swing;
- family-dependent controls including Power Select, ECO, Hi POWER, Outdoor
  Silent, Fireplace, 8 °C Heat, Floor, Sleep, Comfort Sleep, PURE, FIX positions and
  defrost actions where supported;
- separate `ON Timer` and `OFF Timer` selects; ON Timer accepts a new duration only while the unit is OFF, while OFF Timer accepts a new duration only while the unit is ON; `Off` remains available to cancel an armed timer;
- indoor/outdoor and heat-exchanger temperatures, IDU fan feedback, energy and
  other engineering telemetry when the IDU supplies it;
- IDU/ODU model and identity diagnostics;
- model-specific airflow and estimated sensible heating/cooling output; and
- a diagnostic `Toshiba focused monitor` switch.

Availability is determined by the family profile, current HVAC mode and actual
telemetry. An entity being present does not imply that every Toshiba firmware
implements it identically. See
[`TOSHIBA_CONTROL_MATRIX.md`](../TOSHIBA_CONTROL_MATRIX.md) for tested controls
and [`current_status.md`](../current_status.md) for current limitations.

## Output multiplier

`IDU Output Multiplier` is a restored configuration entity from `0.00` to
`1.00`, initially `1.00`. The value multiplies both estimated sensible heating
and cooling output without changing airflow. For example, `0.85` publishes 85%
of the unadjusted estimate.

Keep it at `1.00` unless comparing against simultaneous measurements provides
a reason to reduce it. This is an estimator calibration, not a compressor power
limit or a command sent to the Toshiba unit. Unsupported exact models report
the derived values as unavailable rather than receiving generic airflow data.

## Per-installation feature overrides

If a particular controller belongs to a supported family but a control has
been shown not to work, disable only that feature in the consuming YAML:

```yaml
climate:
  - id: !extend room_id
    disable_features:
      - fixed_position
```

Accepted values are `fixed_position`, `eco`, `hi_power`, `power_select`,
`outdoor_silent`, `fireplace`, `eight_degree_heat`, `floor`, `sleep`,
`comfort`, `pure` and `defrost`. The `defrost` value disables both defrost
actions. Overrides only remove controls; they do not add a capability that the
family profile does not support.

## Focused Monitor

Turning on `Toshiba focused monitor` records raw controller TX and IDU RX
traffic generated during normal operation, including direction, frame length
and bytes. It is intended for a short, deliberate diagnostic session and turns
off again after restart.

This monitor remains part of the active controller and is not a two-sided
passive capture of a genuine Toshiba adaptor. To observe both directions of an
existing adaptor-IDU exchange, build the separate
[passive sniffer](SNIFFER.md).
