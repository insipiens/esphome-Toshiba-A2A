# Current status

This document records the current implementation state, the hardware directly tested, the findings that affect the design, and the main limitations still under investigation.

It is intentionally more detailed than the README. Protocol-level detail belongs in `TOSHIBA_REGISTER_MAP.md`; family/mode availability belongs in `TOSHIBA_CONTROL_MATRIX.md`.

## Project scope

The repository controls Toshiba residential air-to-air indoor units over the internal UART connection used by Toshiba Wi-Fi accessories. The component currently targets ESP32, primarily ESP32-C3 boards, under ESPHome.

The current codebase is a substantial rewrite and extension of `pedobry/esphome_toshiba_suzumi`. The component directory still uses the historical `toshiba_suzumi` name to avoid an unnecessary breaking rename while development is active.

## Hardware directly tested

The current test installation includes:

- `RAS-5M34G3AVG-E1` multi-split outdoor unit;
- `RAS-B13J2FVG-E1` floor/console indoor unit;
- `RAS-B10J2FVG` family floor/console indoor units, including older firmware behaviour;
- `RAS-B10P2KVSGB-E` high-wall indoor unit;
- ESP32-C3 SuperMini-based UART adapters.

Results from these units must not be treated as proof of identical behaviour across all Toshiba residential families or firmware revisions.

## Current architecture

The consuming YAML declares the exact IDU model. That declared model is used for family selection, command encoding and model-specific performance data.

Toshiba `0xE0` equipment-identification traffic is treated as reported runtime identity and diagnostic evidence. Some older firmware sends `NULL` in the IDU-model field even though the rest of the equipment-identification packet is valid.

The main control architecture is:

```text
UART traffic
   -> protocol decoder
   -> Toshiba logical state
   -> family/model capability and mode rules
   -> installation capability profile
   -> ESPHome entities
   -> Home Assistant
```

A UART register is therefore treated as a transport detail rather than automatically as the correct Home Assistant user-interface abstraction.

## Universal package

The repository now maintains one user-facing package:
`packages/toshiba-a2a.yaml`.

A normal installation supplies `device_name`, `friendly_name` and the exact
`toshiba_model`. The model is resolved internally to the appropriate family
profile, capability matrix and family-specific command encoding. Users do not
select a family in the normal installation template.

Family defaults describe the expected full behaviour. If an older/reduced IDU
controller does not implement one of those features correctly, the consuming
YAML can apply a narrow `disable_features` mask after testing. The directly
observed older B10J2 case uses `fixed_position`; this does not create a new
family or package.

The effective control exposure is therefore:

```text
configured Toshiba model
   -> internal family profile
   -> family default capabilities
   -> optional user disable_features mask
   -> ESPHome entities
```

## Home Assistant control model

The project no longer treats all Toshiba special functions as one climate-preset list. Functions are exposed separately so that the Home Assistant controls better resemble the Toshiba remote/application model.

Examples include:

- Power Select as a select;
- ECO as a switch;
- Hi POWER as a switch;
- Silent operation as a select;
- Fireplace as a select;
- 8 °C heat as a switch;
- Floor mode as a switch;
- PURE as a switch on families that support it;
- FIX/louvre position as select entities where implemented;
- defrost as explicit actions/state rather than a climate preset.

Availability by HVAC mode and family is documented in `TOSHIBA_CONTROL_MATRIX.md`.

J2 Comfort Sleep is treated as its own Toshiba operating function. It is not the ON/OFF timer facility; timer registers and scheduling remain separate protocol functions.

## FIX / louvre control

### J2FVG

Vertical FIX has been directly captured on the genuine Toshiba adaptor on the B13J2. The five positions use J2-family `A3` values:

```text
Top     50
Upper   51
Centre  52
Lower   53
Bottom  54
```

J2 vertical swing uses the separate J2 path (`31/41`) rather than the P2 packed-axis encoding.

The Home Assistant FIX selector contains only `Top`, `Upper`, `Centre`, `Lower` and `Bottom`. Its state is passive: it changes only when the component receives a J2 `A3 50..54` FIX state from the IDU. There is no periodic template polling and no synthetic `Not available` or `Position unknown` option in the dropdown.

Ordinary J2 A3 swing states are handled by the climate swing state and are not published into the FIX selector. This prevents normal `Off`/swing readback from being treated as an invalid FIX choice.

FIX entity exposure now follows the family default capability set. If an older controller does not implement the expected FIX behaviour, the installation can add `disable_features: [fixed_position]`; the component then keeps the FIX entity internal before Home Assistant API discovery. This avoids using model-reporting behaviour as a proxy for physical louvre capability and avoids active capability probing during startup.

Direct B13J2 testing confirmed passive readback of all five positions after the unit was running: `50` Top, `51` Upper, `52` Centre, `53` Lower and `54` Bottom. A FIX write while the IDU was off was ACKed but the IDU continued to report ordinary A3 Off state until operation resumed, so ACK alone is not treated as authoritative position state.

### Older B10J2 firmware

One older B10J2 unit reports `NULL` for the IDU model in `0xE0`. Direct `A3 50..54` commands on that unit have been ACKed without producing the expected physical FIX movement. The installed Office example therefore explicitly disables `fixed_position`, which hides the FIX selector without changing the J2 family definition. This is treated as firmware-specific evidence and does not remove FIX capability from the J2 family as a whole.

### P2KVSG

P2 FIX uses a packed horizontal/vertical `A3` representation. Vertical and horizontal state are therefore handled differently from J2 internally, while the same universal package exposes the appropriate FIX controls from the resolved family capability set. `disable_features: [fixed_position]` hides both P2 FIX controls if an installation needs that compatibility override.

## Fan and airflow telemetry

`0xE4 +2` is treated as live IDU fan-speed feedback on the tested units, approximately 10 rpm per count.

`FE` and `FF` are treated as unavailable/sentinel values rather than numeric fan speeds. They are excluded from derived airflow and output calculations.

Airflow conversion is model-specific because different indoor-unit fan and air-path geometries produce very different airflow for the same protocol-scale concept.

The current estimator therefore uses exact-model airflow/performance data where available rather than one generic Toshiba equation.

## Engineering and energy data

The component exposes engineering/status data from `0xE4` and `0xE5`, including selected IDU and ODU temperatures, load/current-like values and fan feedback.

Energy data is also exposed, but some larger Toshiba energy-monitoring structures remain under investigation. Register-level interpretation and evidence grades are kept in `TOSHIBA_REGISTER_MAP.md`.

## Equipment identification

The pushed `0xE0` packet has been decoded as separate IDU and ODU records containing model and additional identity fields.

Observed behaviour includes:

- valid B13J2 model reporting;
- valid P2 model reporting;
- older B10J2 firmware returning `NULL` for IDU model;
- usable ODU model reporting on the same installation.

A decoded E0 packet is logged even when the reported model matches an already-known value so that packet receipt can be distinguished from absence of E0 traffic.

E0 remains diagnostic identity evidence. It is no longer used by the package as the public FIX-entity exposure decision.

## Diagnostics

The universal package includes a `Toshiba focused monitor` diagnostic switch.

Focused Monitor now operates as a loitering, transaction-aware observer. It
does not inject exploratory register reads. While enabled it records the
board's normal UART TX traffic, correlates matching IDU responses, records
unsolicited/passive IDU publications, and highlights changed register payloads.
An unsolicited change is explicitly logged as a remote/control-origin
candidate, allowing handheld-remote actions to be distinguished from responses
to requests generated by the ESPHome controller.

Separate research examples remain available for deeper protocol work:

- `examples/diagnostic_capture.yaml`;
- `examples/engineering_telemetry.yaml`;
- `examples/output_estimation.yaml`.

## Current known limitations

- Compatibility has only been directly tested on a small number of physical units.
- Firmware differences within a nominal family are real and can affect model reporting and control behaviour.
- Some controls are shared-ODU functions on a multi-split system and should not be assumed to be purely local to one IDU.
- Several timer, maintenance and energy fields remain only partly decoded.
- Exact locality of some ODU/current/energy values is still being verified.
- Output estimation is experimental and is not a general Toshiba COP model.
- P2 airflow endpoint data remains less mature than the J2 reference data.

## Evidence policy

The project tries to keep three levels separate:

- directly observed on the physical unit;
- supported by Toshiba documentation or strong repeatable protocol evidence;
- unresolved/inferred.

Unknown values should remain unknown until repeatable evidence justifies promoting them.

## Detailed references

- `TOSHIBA_CONTROL_MATRIX.md` — current family and HVAC-mode control matrix;
- `TOSHIBA_CONTROL_INTERACTION_RULES.md` — shared control interaction/override policy;
- `TOSHIBA_REGISTER_MAP.md` — protocol/register findings and evidence grades;
- `J2_MANUAL_CONTROL_NOTES.md` — notes derived from J2 Toshiba documentation;
- `CHANGELOG.txt` — chronological development history;
- `PROVENANCE.md` — upstream attribution and project lineage.
