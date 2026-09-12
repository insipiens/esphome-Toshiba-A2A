# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. It deliberately distinguishes confirmed mappings from unresolved observations.

## Control and state registers

| Register | Function | Direction | Observed values / notes | Evidence |
| --- | --- | --- | --- | --- |
| `0x80` | Power state | R/W | `0x30` ON, `0x31` OFF | mapped |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | mapped |
| `0x90` | Unknown control | observed | `0x42` observed repeatedly during remote-control testing; candidate momentary louvre/FIX control, not yet assigned | experimental |
| `0x94` | Timer Off state/control | observed / candidate R/W | `0x41` active, `0x42` inactive. `0x41` was directly tied to Timer Off activation; prior Comfort Sleep attribution was incorrect. | confirmed values; write path still to be exercised deliberately |
| `0xA0` | Fan speed | R/W | Quiet `0x31`; Low `0x32`; Low-Med `0x33`; Medium `0x34`; Med-High `0x35`; High `0x36`; Auto `0x41` | mapped |
| `0xA3` | Swing / louvre / HADA | R/W | Off `0x31`; Vertical swing `0x41`; Horizontal swing `0x42`; Both `0x43`; fixed vertical positions `0x50`..`0x54`; HADA `0x60` | mapped, HADA directly observed |
| `0xB0` | HVAC mode | R/W | Auto `0x41`; Cool `0x42`; Heat `0x43`; Dry `0x44`; Fan Only `0x45` | mapped |
| `0xB3` | Target temperature | R/W | raw integer °C in ordinary operation | mapped |
| `0xBB` | Room temperature | Read | `0x7F` invalid/unavailable | mapped |
| `0xBE` | Outdoor temperature | Read | signed byte; `0x7F` invalid/unavailable | mapped |
| `0xC7` | Pure | observed / candidate R/W | `0x18` active, `0x10` inactive; directly correlated with Pure activation/deactivation from the remote. | confirmed values; write path still to be exercised deliberately |
| `0xCB` | Self-clean state | Read | `0x18` running, `0x10` off | mapped |
| `0xD8` | Daily energy | Read | 24 hourly little-endian values in extended response | mapped |
| `0xD9` | Weekly energy | declared | no parser/use yet | declared only |
| `0xDA` | Monthly energy | declared | no parser/use yet | declared only |
| `0xDB` | Yearly energy | declared | no parser/use yet | declared only |
| `0xDE` / `0xDF` | Wi-Fi LED control | Write | component uses paired writes | mapped |
| `0xE0` | Equipment information | pushed/asynchronous | IDU + ODU identity records observed in class-`0x11` messages; active read not established | observed |
| `0xE4` | IDU engineering status | Read | 8-byte extended status payload | partially characterised |
| `0xE5` | ODU/system engineering status | Read | 8-byte extended status payload | partially characterised |
| `0xEA` | Date/time sync | Write | multi-byte payload; ACK ends `0x99 0x99` | mapped |
| `0xF7` | Special functions | R/W | Standard `0x00`; Hi POWER `0x01`; Silent 1 `0x02`; ECO `0x03`; 8°C `0x04`; Sleep `0x05`; Floor `0x06`; Comfort `0x07`; Silent 2 `0x0A`; Fireplace 1 `0x20`; Fireplace 2 `0x30` | mapped |

## Raw diagnostic entities

The scalar registers can be exposed as raw numeric sensors so Home Assistant can retain long-term history while write semantics and cross-model behaviour are still being established:

```yaml
climate:
  - platform: toshiba_suzumi
    # ...normal configuration...
    register_90_raw:
      name: "Toshiba Register 0x90"
    register_94_raw:
      name: "Toshiba Timer Off Raw"
    register_c7_raw:
      name: "Toshiba Pure Raw"
```

When configured, the component publishes both unsolicited scalar updates and periodic reads of those registers. Keeping the raw byte history is useful even where a logical meaning is now known, because it preserves evidence for firmware/model differences and any additional states.

## `0xE4` IDU status

Current working interpretation:

```text
+0 IDU heat-exchanger temperature
+1 second / junction IDU temperature; exact physical location unresolved
+2 raw live fan / airflow feedback quantity; not literal RPM
+3 unknown; zero in current captures
+4 unknown; zero in current captures
+5 unknown; zero in current captures
+6 unknown; zero in current captures
+7 unknown; zero in current captures
```

Fan-only testing strongly confirms `+2` as a live fan/airflow feedback quantity. It changes independently of the command enum and tracks measured/derived airflow.

## `0xE5` ODU/system status

Current working interpretation:

```text
+0 discharge temperature
+1 suction temperature
+2 ODU heat-exchanger temperature
+3 IDU-associated load / allocation-like quantity
+4 unknown engineering quantity
+5 unknown engineering quantity
+6 ODU current-like quantity; exact physical scope unresolved
+7 unresolved; remains 0x00 in current captures
```

When ODU engineering data is unavailable, observed sentinel frame:

```text
+0=0x7F +1=0x7F +2=0x7F +3=0xFE +4=0xFE +5=0xFE +6=0xFE +7=0x00
```

This is evidence that `+4` and `+5` are real engineering fields rather than simple padding, even though they have so far been zero whenever valid.

### Observed `E5 +6` behaviour

`+6` tracks ODU electrical activity closely enough to remain classified as current-like, but the exact physical scope and scaling are still empirical. The component currently applies `raw / 10 * 0.827`.

Representative captures on the same system:

| HVAC mode | Operating state | Raw `E5 +6` | Component current |
| --- | --- | ---: | ---: |
| Fan Only | compressor off | `0x00` / 0 | 0.0 A |
| Fan Only | ODU data unavailable | `0xFE` / 254 | invalid |
| Dry | compressor idle | `0x01` / 1 | 0.1 A |
| Dry | compressor running | `0x24` / 36 | 3.0 A |
| Cool | compressor idle / just starting | `0x00`..`0x01` / 0..1 | 0.0..0.1 A |
| Cool | moderate running | `0x1E` / 30 | 2.5 A |
| Cool | stronger running | `0x24`..`0x26` / 36..38 | 3.0..3.1 A |
| Heat | compressor just starting | `0x01` / 1 | 0.1 A |
| Heat | moderate running | `0x26` / 38 | 3.1 A |
| Heat | high running | `0x60` / 96 | about 7.9 A |
| Heat | very high running | `0x70` / 112 | about 9.3 A |

The large range in Heat mode is particularly useful: `+6` is clearly not merely a mode flag or discrete state.

A current working hypothesis is that `E5 +6` may be an ODU electrical quantity used together with the per-IDU `E5 +3` allocation/load-like field for Toshiba's per-IDU energy accounting. This remains an inference requiring simultaneous multi-IDU validation.

## Louvre controls still unresolved

The physical high-wall remote exposes separate FIX buttons for up/down and left/right direction. Current protocol knowledge is incomplete:

- fixed vertical positions are mapped through `0xA3 = 0x50..0x54`;
- HADA is mapped as `0xA3 = 0x60`;
- fixed horizontal position is not yet mapped;
- `0x90 = 0x42` is a current candidate for a momentary FIX/louvre command and must be tested deliberately before assignment.

## Evidence labels

- **mapped** — repeatable protocol mapping used by the component.
- **confirmed values** — values directly observed under known physical control changes, but full read/write semantics may still need testing.
- **partially characterised** — field exists and some meaning is strongly supported, but exact scale or physical scope is unresolved.
- **experimental** — observed but function not yet assigned.
- **declared only** — present in code/protocol vocabulary without sufficient live testing.
