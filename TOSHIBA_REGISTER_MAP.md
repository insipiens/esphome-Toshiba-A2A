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
| `0xA3` | Swing / louvre / H.DA | R/W | Readback/state values observed: Off `0x31`; Vertical swing `0x41`; Horizontal swing `0x42`; Both `0x43`; H.DA `0x60`. Genuine Toshiba Wi-Fi adaptor write-side commands use a different encoding for several louvre functions; see dedicated section below. | mapped readback; write encoding partially characterised |
| `0xA4` | Structured louvre/swing state | Read | 3-byte payload. Byte 0 mirrors `0xA3` during swing testing: `41 00 00`, `42 00 00`, `43 00 00`, `60 00 00`. Bytes 1-2 unresolved. One isolated `53 2D 42` capture occurred during heavy sweep traffic and is treated as suspect/corrupted until reproduced. | partially characterised |
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

## Genuine Wi-Fi adaptor `0xA3` louvre commands

Passive capture of the genuine Toshiba Wi-Fi adaptor established that the adaptor does **not** simply write the ordinary `0xA3` readback values for every louvre operation. The adaptor emits scalar writes of the normal form:

```text
02 00 03 10 00 00 07 01 30 01 00 02 A3 <value> <checksum>
```

and the IDU acknowledges them with the same generic `0xA3` write ACK:

```text
02 00 03 90 00 00 08 01 30 01 00 00 00 01 A3 8F
```

The ACK does not contain the resulting louvre position or swing state.

### FIX position captures

A vertical-FIX run produced these consecutive write values:

```text
A3=88
A3=89
A3=8A
A3=8B
A3=8C
A3=8D
```

A horizontal-FIX run produced:

```text
A3=85
A3=8D
A3=95
A3=9D
A3=A5
A3=AD
```

The physical UI/remote exposes five vertical FIX positions (top/horizontal airflow through bottom/downward airflow) and five horizontal FIX positions (far left through far right). Six distinct writes were captured in each test sequence, so the exact five-position-to-code assignment still needs one position-at-a-time correlation. Do not yet label all six captured values as six physical positions.

The numerical structure is nevertheless strong evidence that `0xA3` FIX writes contain separate horizontal and vertical subfields: the vertical sequence increments by `+1`, while the horizontal sequence increments by `+8`. The repeated `0x8D` in both runs further supports a combined two-axis command value. The exact bit-field semantics and any extra/default transition code remain unresolved.

### Swing-mode write captures

A controlled Wi-Fi-adaptor test, operated in this order:

```text
Vertical swing
Horizontal swing
Vertical + horizontal swing
No swing
H.DA
```

produced:

```text
Vertical swing         -> A3=AE
Horizontal swing       -> A3=B6
Vertical + horizontal  -> A3=80
No swing               -> A3=80
H.DA                   -> A3=60
```

A repeat of the horizontal/V+H transition reproduced:

```text
Horizontal swing -> A3=B6
next state       -> A3=80
```

Therefore `0xAE` and `0xB6` are confirmed genuine-adaptor write commands for vertical and horizontal swing respectively, and `0x60` is used directly for H.DA. `0x80` is repeatably used in the V+H / no-swing transition path, but the current captures do not distinguish whether `0x80` itself uniquely means Both, Off, a toggle/transition, or a neutral combined-louvre command. More controlled state/readback correlation is required before assigning it a unique semantic meaning.

This write-side command encoding is distinct from the previously observed `0xA3` state/readback values:

```text
Readback/state:
31 = no swing
41 = vertical swing
42 = horizontal swing
43 = both swing
60 = H.DA

Observed genuine-adaptor writes:
AE = vertical swing command
B6 = horizontal swing command
80 = V+H / no-swing transition command, exact semantics unresolved
60 = H.DA command
85..AD pattern = combined FIX-position command family, exact five-position mapping unresolved
```

The old assumption that fixed vertical positions were simply `0x50..0x54` is not supported by these genuine-adaptor captures and should be treated as obsolete until independently proven on another model/firmware.

## Current diagnostic sweep

For protocol discovery, the component can sweep the user-control/state bank and log successful replies at INFO level while preserving complete payloads. This avoids creating dozens of temporary Home Assistant entities.

The most useful current captures are:

```text
0x90 = 42              probable ON timer inactive
0x92 = 00 00           probable ON timer value
0x94 = 41 / 42         OFF timer active / inactive
0x96 = HH MM           OFF timer programmed value
0xA3 = 31/41/42/43/60  readback/state: Off / Vertical / Horizontal / Both / H.DA
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

## Louvre controls

The physical high-wall remote exposes separate FIX controls for up/down and left/right direction. Current protocol knowledge is now:

- ordinary `0xA3` readback/state values are `31` Off, `41` Vertical swing, `42` Horizontal swing, `43` Both, `60` H.DA;
- the genuine Toshiba Wi-Fi adaptor uses a distinct command encoding for FIX and several swing operations;
- vertical FIX writes followed a `88..8D` sequence during the captured test;
- horizontal FIX writes followed `85, 8D, 95, 9D, A5, AD`;
- there are physically five FIX positions on each axis; the extra captured command/state must not be mislabelled as a sixth position until individually correlated;
- vertical and horizontal FIX command values show separate `+1` and `+8` numerical fields, strongly indicating a packed two-axis command;
- genuine-adaptor swing writes observed: Vertical `AE`, Horizontal `B6`, H.DA `60`;
- `80` is repeatably used around the Both/Off transition, but exact semantics are unresolved;
- immediate IDU response to each genuine-adaptor `A3` write is only the generic `A3 8F` ACK and does not feed the resulting state back in that packet;
- `0xA4` remains a 3-byte read-side louvre/swing record whose byte 0 mirrors the ordinary `A3` readback mode.

The next useful experiment is controlled one-position-at-a-time correlation for all five vertical and horizontal FIX positions, plus explicit readback/state polling after each genuine-adaptor swing write.

## Evidence labels

- **mapped** — repeatable protocol mapping used by the component.
- **confirmed** — directly correlated with a known physical control and reproduced.
- **confirmed values** — values directly observed under known physical control changes, but full read/write semantics may still need testing.
- **probable** — strong structural/behavioural inference awaiting one direct confirmation test.
- **partially characterised** — field exists and some meaning is strongly supported, but exact scale or physical scope is unresolved.
- **experimental** — observed but function not yet assigned.
- **declared only** — present in code/protocol vocabulary without sufficient live testing.
