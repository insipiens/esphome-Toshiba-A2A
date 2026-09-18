# Adding support for a model

Model support has three distinct layers: family-level control encoding,
exact-model airflow data, and per-installation exceptions. Establish which
layer is missing before changing code.

## Collect reproducible evidence

Record at least:

- exact indoor- and outdoor-unit model strings, including suffixes;
- single- or multi-split installation and the attached indoor units;
- remote-controller model and available buttons/functions;
- HVAC mode, setpoint, requested fan and louvre state for each capture;
- whether a frame came from the ESP controller, a genuine Toshiba adaptor or
  an unsolicited IDU publication;
- repeated before/after captures for every proposed command mapping; and
- whether engineering values are derived from published datasheets or direct
  measurement, including the measurement method where applicable.

Preserve raw timestamped UART bytes. A decoded interpretation is useful, but it
cannot replace the underlying capture. Follow [the sniffer guide](SNIFFER.md)
when a genuine adaptor is available.

## Case 1: another exact model in a known family

The current dispatcher in
[`toshiba_device_profile.cpp`](../components/toshiba_a2a/toshiba_device_profile.cpp)
recognises `J2FVG` and `P2KVSG` model-family tokens. If the new IDU truly uses
one of those established control encodings:

1. Configure its exact model as `toshiba_model` and compile the standard
   template.
2. Verify power, modes, fan values, swing/FIX behaviour and each advertised
   function against the genuine remote or adaptor.
3. Compare readback as well as command acknowledgement; an ACK alone does not
   prove physical behaviour.
4. Use `disable_features` for a controller-specific omission. Do not create a
   new family merely because one firmware lacks a feature.
5. Add the tested model and limitations to the compatibility documentation.

## Case 2: add an exact-model output estimate

Control-family support does not automatically establish airflow. Add separate
cooling and heating entries to
[`toshiba_airflow_data.h`](../components/toshiba_output/toshiba_airflow_data.h)
only when the exact capacity/model has defensible data for:

- the live E4+2 fan-feedback range; and
- the corresponding minimum and maximum air volume in m³/h.

Document whether values are derived from published datasheets, repeatable
direct measurement or a provisional inference. Test stopped-fan, low/high fan
and heating/cooling behaviour. Do not copy airflow endpoints from a different
capacity simply because the units share a protocol family.

## Case 3: a genuinely new control family

A family is new when verified traffic demonstrates different capabilities or
wire encodings, not merely a different model name. A complete change normally
requires:

1. a family enum and display name in `toshiba_device_profile.h/.cpp`;
2. a declarative family header modelled on `toshiba_family_j2fvg.h` or
   `toshiba_family_p2kvsg.h`;
3. a precise model matcher in `profile_for_model()`;
4. proven capability and per-HVAC-mode masks;
5. any family-specific A3 louvre/FIX encoding and decoding;
6. validated F8 command/readback behaviour for modes, fan and functions;
7. compile-time assertions and focused tests for the new profile;
8. control-matrix and register-map updates with evidence grades; and
9. a compile-tested example using an exact real model.

Unknown families must remain conservative. Avoid exposing controls based on a
similar-looking remote or another Toshiba range. Multi-split observations also
need care: an outdoor-unit value may be shared, allocated or system-wide rather
than local to one IDU.

## Contribution checklist

- Keep raw capture data free of Wi-Fi credentials and Home Assistant keys.
- Separate direct observation, datasheet-derived values and inference.
- State what remains untested.
- Run the example compile checks.
- Add a dated `CHANGELOG.txt` entry for significant behaviour or documentation
  changes.
