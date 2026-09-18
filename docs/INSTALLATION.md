# Installation

This guide installs the universal Toshiba A2A package on the tested ESP32-C3
interface. Read [the hardware guide](HARDWARE.md) before building or connecting
the adaptor.

## Before connecting the IDU

1. Assemble and inspect the interface, including a continuity check between
   supply and ground.
2. Keep the indoor unit isolated from mains while connecting or disconnecting
   the five-pin cable.
3. Flash the ESP32-C3 from USB and confirm that it boots before connecting it to
   the indoor unit.

The package is configured for the tested board wiring: GPIO21 is UART TX,
GPIO20 is UART RX, and the bus is 9600 baud, 8 data bits, even parity and one
stop bit. If using different hardware, verify both its logic levels and pinout
independently.

## Create the ESPHome configuration

Copy [`examples/toshiba-a2a-template.yaml`](../examples/toshiba-a2a-template.yaml)
into your ESPHome configuration directory. Set these four substitutions:

```yaml
substitutions:
  device_name: "toshiba-a2a-living-room"
  friendly_name: "Toshiba Living Room"
  climate_entity_name: "Living Room"
  toshiba_model: "RAS-B10J2FVG-E"
```

`toshiba_model` must contain the exact model printed on the indoor unit, not
the outdoor-unit model or a shortened product-family name. It supplies the
startup family/profile when the IDU does not provide usable E0 identity data.
The Toshiba-reported model remains visible separately as a diagnostic entity.

Create the secrets referenced by the example:

```yaml
wifi_ssid: "your-ssid"
wifi_password: "your-password"
key: "your-32-byte-base64-api-key"
ota_password: "your-ota-password"
```

Install with the ESPHome dashboard or command line:

```console
esphome run toshiba-a2a-living-room.yaml
```

The first installation normally uses USB. Subsequent builds can use OTA once
the node is online.

## Connect and verify

1. Isolate the IDU from mains.
2. Fit the keyed Toshiba cable to the adaptor, following the PCB silkscreen.
3. Secure the adaptor so its underside and USB connector cannot contact the
   indoor unit chassis or other conductors.
4. Restore power and watch the ESPHome log.
5. Add the discovered node to Home Assistant and verify that the climate entity
   reports mode, setpoint and room temperature.
6. Compare the configured model with the `IDU Model` entity if the latter is
   available.
7. Test ordinary power, mode, temperature and fan commands before testing any
   family-specific functions.

Do not repeatedly operate unsupported buttons to determine compatibility.
Disable a control that is exposed for the family but does not work on the
particular IDU, as described in [Usage and options](USAGE.md).

## Troubleshooting

- No boot or no entities: disconnect the IDU and confirm that the ESP32 still
  boots from USB; then recheck supply, ground and module orientation.
- No climate traffic: verify GPIO21/GPIO20, even parity, connector orientation
  and the level shifter connections.
- Reboots or unreliable traffic: inspect the power path and solder joints; do
  not assume a visually similar ESP32-C3 SuperMini has an identical layout.
- Wrong or absent family-specific controls: verify the exact `toshiba_model`
  spelling and consult [the control matrix](../TOSHIBA_CONTROL_MATRIX.md).
- Need protocol evidence: use the separate receive-only setup in
  [the sniffer guide](SNIFFER.md), not the normal controller configuration.
