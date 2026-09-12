# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. It deliberately distinguishes confirmed mappings from unresolved observations.

## Control and state registers

| Register | Function | Direction | Observed values / notes | Evidence |
| --- | --- | --- | --- | --- |
| `0x80` | Power state | R/W | `0x30` ON, `0x31` OFF | mapped |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | mapped |
| `0x90` | Probable ON timer state/control | observed | `0x42` observed when no ON timer was active. Strongly suspected companion to `0x92`, mirroring the confirmed OFF-timer pair `0x94`/`0x96`; direct ON-timer test still required. | probable |
| `0x92` | Probable ON timer programmed value | observed | Two-byte payload. Baseline `00 00`. Strongly suspected `HH MM`, mirroring confirmed OFF-timer register `0x96`; direct ON-timer test still required. | probable |
| `0x94` | OFF timer state/control | observed / candidate R/W | `0x41` active, `0x42` inactive. Directly correlated with OFF-timer activation/deactivation. Prior Comfort Sleep attribution was incorrect. | confirmed |
| `0x96` | OFF timer programmed value | observed / candidate R/W | Two-byte `HH MM` payload. Direct tests: 30 min -> `00 1E`; 6 h -> `06 00`. Value remained fixed while timer was active, so this is the programmed duration/value rather than a visible countdown. | confirmed |
| `0xA0` | Fan speed | R/W | Quiet `0x31`; Low `0x32`; Low-Med `0x33`; Medium `0x34`; Med-High `0x35`; High `0x36`; Auto `0x41` | mapped |
| `0xA3` | Swing / louvre / H.DA | R/W | Off `0x31`; Vertical swing `0x41`; Horizontal swing `0x42`; Both `0x43`; fixed vertical positions `0x50`..`0x54`; H.DA `0x60` | mapped; four-position SWING-button cycle directly confirmed `41 -> 42 -> 43 -> 60` |
| `0xA4` | Structured louvre/swing state | Read | 3-byte payload. Byte 0 mirrors `0xA3` during swing testing: `41 00 00`, `42 00 00`, `43 00 00`, `60 00 00`. Bytes 1-2 unresolved; likely candidates for FIX/louvre position data. One isolated `53 2D 42` capture occurred during heavy sweep traffic and is treated as suspect/corrupted until reproduced. | partially characterised |
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

## Current diagnostic sweep

For protocol discovery, the component can sweep the user-control/state bank and log successful replies at INFO level while preserving complete payloads. This avoids creating dozens of temporary Home Assistant entities.

The most useful current captures are:

```text
0x90 = 42              probable ON timer inactive
0x92 = 00 00           probable ON timer value
0x94 = 41 / 42         OFF timer active / inactive
0x96 = HH MM           OFF timer programmed value
0xA3 = 41/42/43/60     Vertical / Horizontal / Both / H.DA
0xA4 = xx 00 00        structured louvre state; byte 0 mirrors A3
0xC7 = 18 / 10         Pure active / inactive
```

Large structured responses were observed near the top of the swept range, particularly around `0xCD`, together with checksum collisions during normal traffic. Treat `0xCC`-`0xCF` cautiously during automatic polling until their framing and purpose are understood.

## Timer bank

The timer registers now show a clear paired structure:

```text
0x90  probable ON timer enable/state
0x92  probable ON timer programmed value (likely HH MM)

0x94  OFF timer enable/state
0x96  OFF timer programmed value (HH MM)
```

Confirmed OFF-timer examples:

```text
OFF timer 00:30 -> 0x94=41, 0x96=00 1E
OFF timer 06:00 -> 0x94=41, 0x96=06 00
cancelled         -> 0x94=42, 0x96=00 00
```

The repeated fixed `0x96` value while the timer remained enabled indicates that this register holds the programmed timer value, not an externally readable remaining-time countdown.

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

The physical high-wall remote exposes separate FIX buttons for up/down and left/right direction. Current protocol knowledge is now:

- `0xA3 = 0x41` Vertical swing;
- `0xA3 = 0x42` Horizontal swing;
- `0xA3 = 0x43` Both;
- `0xA3 = 0x60` H.DA;
- fixed vertical positions are already mapped as `0xA3 = 0x50..0x54`;
- `0xA4` is a 3-byte structured louvre/swing state and byte 0 mirrors the current `0xA3` swing mode;
- `0xA4` bytes 1-2 are unresolved and are prime candidates for FIX up/down and left/right position data;
- fixed horizontal position is not yet mapped.

The next direct experiment is to operate the two FIX controls independently while watching both unsolicited traffic and the `0xA3`/`0xA4` sweep results.

## Evidence labels

- **mapped** — repeatable protocol mapping used by the component.
- **confirmed** — directly correlated with a known physical control and reproduced.
- **confirmed values** — values directly observed under known physical control changes, but full read/write semantics may still need testing.
- **probable** — strong structural/behavioural inference awaiting one direct confirmation test.
- **partially characterised** — field exists and some meaning is strongly supported, but exact scale or physical scope is unresolved.
- **experimental** — observed but function not yet assigned.
- **declared only** — present in code/protocol vocabulary without sufficient live testing.
