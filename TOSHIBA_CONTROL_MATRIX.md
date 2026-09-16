# Toshiba control matrix

This file is the user-facing control matrix for the directly tested Toshiba IDU families. Protocol detail belongs in `TOSHIBA_REGISTER_MAP.md`; manual notes belong in `J2_MANUAL_CONTROL_NOTES.md`.

Legend: `yes` = observed/documented for this family and mode, `no` = unavailable, `?` = not yet established.

## Tested families

- `J2FVG` floor/console: directly tested on `RAS-B13J2FVG-E1`; older `RAS-B10J2FVG` units have different firmware behaviour.
- `P2KVSG` high-wall: directly tested on installed `RAS-B10P2KVSGB-E`.

The consuming YAML must declare the exact IDU model. That model selects the runtime family profile and family-specific command encoding. Pushed `0xE0` identity remains diagnostic evidence and can confirm the declared model where the firmware reports it. Exact-model airflow/performance data remains separate from this control matrix.

## J2FVG mode matrix

| Control | Auto | Cool | Heat | Dry | Fan | Scope | Transport |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Power Select | yes | yes | yes | yes | yes | shared ODU | `87` |
| ECO | yes | yes | yes | no | no | IDU demand | `F7/F8` |
| Hi POWER | yes | yes | yes | no | no | IDU demand | `F7/F8` |
| Silent Operation | yes | yes | yes | no | no | shared ODU | `F7/F8` |
| PURE | yes | yes | yes | yes | yes | IDU local | `C7` |
| Comfort Sleep | yes | yes | yes | no | no | IDU demand | mapping still under test |
| Fireplace | no | no | yes | no | no | IDU local | `F7/F8` |
| 8 °C Heat | no | no | yes | no | no | IDU demand | `F7/F8` |
| Floor | no | no | yes | no | no | IDU local | `F7/F8` |
| Vertical FIX | yes | yes | yes | ? | yes | IDU local | `A3 50..54` |
| Start Defrost | ? | no | yes | no | no | shared ODU | `CB` |
| Strong Defrost | ? | no | yes | no | no | shared ODU | `CB` |

### J2FVG fan matrix

| Fan choice | Auto | Cool | Heat | Dry | Fan |
| --- | ---: | ---: | ---: | ---: | ---: |
| Auto | yes | yes | yes | yes | yes |
| Quiet | yes | yes | yes | no | yes |
| Levels 1-5 | yes | yes | yes | no | yes |

Fan command values are common to the tested J2/P2 units: Quiet `31`, levels 1-5 `32..36`, Auto `41`.

### J2FVG compatibility/dependency rules

| Control A | Control B | Behaviour |
| --- | --- | --- |
| Power Select | ECO | cancels ECO in Auto/Cool/Heat |
| Power Select | Hi POWER | cancels Hi POWER in Auto/Cool/Heat |
| Power Select | Silent Operation | returns Silent to Standard in Auto/Cool/Heat |
| Power Select | PURE | PURE remains unchanged |
| Fireplace | 8 °C Heat | mutually exclusive user state |
| Floor | Fan | Floor forces fan to Auto |

`Normal / Fireplace 1 / Fireplace 2 / 8 °C Heat` are presented as one mutually-exclusive user state on the J2 remote. `PURE` remains independent.

## P2KVSG mode matrix

This matrix is directly observed on the genuine Toshiba app connected to `RAS-B10P2KVSGB-E`.

| Control | Auto | Cool | Heat | Dry | Fan | Scope | Transport |
| --- | ---: | ---: | ---: | ---: | ---: | --- | --- |
| Power Select | yes | yes | yes | yes | yes | shared ODU | `87` |
| ECO | yes | yes | yes | no | no | IDU demand | `F7/F8` |
| Hi POWER | yes | yes | yes | no | no | IDU demand | `F7/F8` |
| Silent Operation | yes | yes | yes | no | no | shared ODU | `F7/F8` |
| PURE | yes | yes | yes | yes | yes | IDU local | `C7` |
| 8 °C Heat | no | no | yes | no | no | IDU demand | `F7/F8` |
| Start Defrost | yes | no | yes | no | no | shared ODU | `CB` |
| Vertical FIX | yes | yes | yes | ? | yes | IDU local | packed `A3` |
| Horizontal FIX | yes | yes | yes | ? | yes | IDU local | packed `A3` |

### P2KVSG fan matrix

| Fan choice | Auto | Cool | Heat | Dry | Fan |
| --- | ---: | ---: | ---: | ---: | ---: |
| Auto | yes | yes | yes | yes | yes |
| Quiet | yes | yes | yes | no | yes |
| Levels 1-5 | yes | yes | yes | no | yes |

### P2KVSG compatibility rules

| Control A | Control B | Behaviour |
| --- | --- | --- |
| Power Select | ECO | cancels ECO in Auto/Cool/Heat |
| Power Select | Hi POWER | cancels Hi POWER in Auto/Cool/Heat |
| Power Select | Silent Operation | returns Silent to Standard in Auto/Cool/Heat |
| Power Select | PURE | PURE remains unchanged |
| Dry mode | Fan | forces Auto |

## Home Assistant entity model

| Toshiba control | ESPHome / Home Assistant representation |
| --- | --- |
| HVAC mode + target temperature | climate entity |
| Fan | one ordered list: Auto / Quiet / Low / Low-Medium / Medium / Medium-High / High |
| Power Select | select: 100% / 75% / 50% |
| ECO | switch |
| Hi POWER | switch |
| Silent Operation | select: Standard / Silent 1 / Silent 2 |
| PURE | switch |
| Fireplace | select: Off / Fireplace 1 / Fireplace 2 |
| 8 °C Heat | switch |
| Floor | switch |
| Vertical FIX | five-position select |
| Horizontal FIX | five-position select, P2 only |
| Start/Strong Defrost | button/action with separate pushed state |

The UART register is transport, not the public UI model. Family capability, HVAC-mode availability and compatibility rules determine which entities/actions are valid.

## Family identification

The exact IDU model is mandatory in the consuming YAML and is the configuration authority for family selection. The runtime maps that model to `J2FVG`, `P2KVSG`, or a conservative unknown profile before control commands are issued.

`0xE0` remains the Toshiba-reported identity source when available. A declared model therefore continues to select the correct family protocol even on older firmware whose `0xE0` IDU-model field is blank/`NULL`.

For J2, the Home Assistant FIX control is now a stable package-level template select rather than a dynamically discovered entity. The underlying protocol select remains internal. If no usable IDU model has ever been learned from `0xE0`, the public selector reports `Not available` and ignores FIX requests. If a valid E0 model is known but no current fixed-position state has been decoded, it reports `Position unknown`; otherwise it mirrors `Top / Upper / Centre / Lower / Bottom` and forwards those choices to the J2 `A3 50..54` control path. This lets the same J2 package serve both the B13 reference unit and older B10 firmware without a per-device `expose_fix` setting.
