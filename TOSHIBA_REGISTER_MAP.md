# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. It deliberately distinguishes confirmed mappings from unresolved observations.

## Control and state registers

| Register | Function | Direction | Observed values / notes | Evidence |
| --- | --- | --- | --- | --- |
| `0x80` | Power state | R/W | `0x30` ON, `0x31` OFF | mapped |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | mapped |
| `0x90` | Probable ON timer state/control | observed | `0x42` observed when no ON timer was active. Strongly suspected companion to `0x92`, mirroring the confirmed OFF-timer pair `0x94`/`0x96`; direct ON-timer test still required. | probable |
| `0x92` | Probable ON timer programmed value | observed | Two-byte payload. Baseline `00 00`. Strongly suspected `HH MM`, mirroring confirmed OFF-timer register `0x96`; direct ON-timer test still required. | probable |
| `0x94` | OFF timer state/control | R/W | `0x41` enable/start, `0x42` disable/clear. Genuine Wi-Fi adaptor writes `0x96` first, then `0x94=41`; clearing writes only `0x94=42`. | confirmed |
| `0x96` | OFF timer programmed value | R/W | Two-byte `HH MM` programmed interval. Genuine adaptor captures: 30 min -> `00 1E`; 1 h -> `01 00`. Stored interval is separate from enable state. | confirmed |
| `0xA0` | Fan speed | R/W | Quiet `0x31`; Low `0x32`; Low-Med `0x33`; Medium `0x34`; Med-High `0x35`; High `0x36`; Auto `0x41` | mapped |
| `0xA3` | Swing / louvre / H.DA | R/W / pushed | Readback/state values observed: Off `0x31`; Vertical swing `0x41`; Horizontal swing `0x42`; Both `0x43`; H.DA `0x60`. Genuine Toshiba Wi-Fi adaptor write-side commands use a different encoding. IDU-originated class-`0x11` pushes of `A3=80` were observed immediately after power-on. Exact `0x80` state meaning remains unresolved. | mapped readback; write encoding partially characterised |
| `0xA4` | Structured louvre/swing state | Read | 3-byte payload. Byte 0 mirrors `0xA3` during swing testing: `41 00 00`, `42 00 00`, `43 00 00`, `60 00 00`. Bytes 1-2 unresolved. One isolated `53 2D 42` capture occurred during heavy sweep traffic and is treated as suspect/corrupted until reproduced. | partially characterised |
| `0xB0` | HVAC mode | R/W | Auto `0x41`; Cool `0x42`; Heat `0x43`; Dry `0x44`; Fan Only `0x45` | mapped |
| `0xB3` | Target temperature | R/W | raw integer °C in ordinary operation | mapped |
| `0xBB` | Room temperature | Read / pushed | raw integer °C; `0x7F` invalid/unavailable | mapped |
| `0xBE` | Outdoor temperature | Read / pushed | signed byte; `0x7F` invalid/unavailable | mapped |
| `0xC7` | Pure | R/W | `0x18` ON, `0x10` OFF. Both values directly captured as genuine Wi-Fi adaptor writes and previously observed in state/readback. | confirmed |
| `0xCA` | Structured status/configuration block | Read | Genuine Wi-Fi adaptor periodically requests this register. Observed reply payload `00 E8 03 00 00`; exact semantics unresolved. | observed |
| `0xCB` | Self-clean state | Read | `0x18` running, `0x10` off | mapped |
| `0xCC` | Daily-view energy history | Read | Official app polling. 502-byte response. 24 × 20-byte hourly records. Record `+8` is hourly electrical consumption as little-endian Wh. | confirmed |
| `0xCD` | Weekly-view energy history | Read | Official app polling. 218-byte response. 7 × 28-byte daily records. Record `+8` is daily electrical consumption as little-endian uint32 Wh. | confirmed |
| `0xCE` | Monthly-view energy history | Read | Official app polling. 890-byte response. 31 × 28-byte day-of-month records. Record `+8` is daily electrical consumption as little-endian uint32 Wh. | confirmed |
| `0xCF` | Yearly-view energy history | Read | Official app polling. 358-byte response. 12 × 28-byte monthly records. Record `+8` is monthly electrical consumption as little-endian uint32 Wh. | confirmed |
| `0xD8` | Daily energy | Read | Existing component interpretation: 24 hourly little-endian values in extended response. Relationship to official app `0xCC`-`0xCF` history blocks remains unresolved. | mapped; relationship unresolved |
| `0xD9` | Weekly energy | declared | no parser/use yet | declared only |
| `0xDA` | Monthly energy | declared | no parser/use yet | declared only |
| `0xDB` | Yearly energy | declared | no parser/use yet | declared only |
| `0xDE` | Wireless/Wi-Fi LED control | Write | Genuine adaptor: `0x00` LED OFF, `0x05` LED ON. Immediate ACK ends `DE 54`. | confirmed |
| `0xDF` | Wi-Fi-related control | Write | Existing component uses this alongside `0xDE`; exact independent purpose still unresolved. | partially characterised |
| `0xE0` | Equipment information | pushed/asynchronous | IDU + ODU identity records observed in class-`0x11` messages. Genuine adaptor receives this unsolicited and acknowledges it. | confirmed pushed record |
| `0xE4` | IDU engineering status | pushed/read | 8-byte extended status payload. `+2` is raw live fan/airflow feedback, not commanded fan enum or literal RPM. In Hi POWER heating it held `0x55` during the earlier restricted phase then rose to and sustained `0x61` after roughly 12–14 minutes. | partially characterised |
| `0xE5` | ODU/system engineering status | Read / pushed | 8-byte extended status payload. `+6` is current-like and tracks ODU electrical activity. | partially characterised |
| `0xEA` | Date/time sync | Write | multi-byte payload; ACK ends `0x99 0x99` | mapped |
| `0xF7` | Special functions | R/W | Standard `0x00`; Hi POWER `0x01`; Silent 1 `0x02`; ECO `0x03`; 8°C `0x04`; Sleep `0x05`; Floor `0x06`; Comfort `0x07`; Silent 2 `0x0A`; Fireplace 1 `0x20`; Fireplace 2 `0x30` | mapped |
| `0xF8` | Aggregate operating-state command | Write / observed | 4-byte state `[mode][target °C][fan command][flags]`. Heat observed as `43 <target> 41 <flags>` with Auto fan. Byte 3 bit 0 is confirmed Hi POWER: `00` OFF, `01` ON. Immediate ACK ends `F8 3A`. | confirmed |

## Protocol frame classes and sniffer decoding

Observed message classes use the common `02 00 03` prefix and the 16-bit big-endian protocol length at bytes 5-6. Complete frame length is:

```text
frame_length = protocol_length + 8
```

Register location depends on message class:

```text
0x10  Wi-Fi adaptor request/write     register at byte 12
0x90  IDU response                    register at byte 14
0x11  IDU unsolicited/push            register at byte 12
0x91  Wi-Fi ACK of unsolicited push   no register field
```

This distinction matters for passive logging: treating all IDU traffic as `0x90` mislabels the checksum of `0x11` pushes as the register, and treating `0x91` ACKs as requests invents a register that is not present.

Length-field reassembly has been validated against both short control traffic and the large official energy frames. Examples reconstructed as one protocol frame before display chunking:

```text
CC = 502 bytes
CD = 218 bytes
CE = 890 bytes
CF = 358 bytes
```

Short 13-22 byte request, ACK and push frames continued to decode normally with the same reassembler.

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

A vertical-FIX run produced:

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

The physical UI/remote exposes five vertical FIX positions and five horizontal FIX positions. Six distinct writes were captured in each sweep, so one code in each sequence is a transition/default/current-state effect rather than a sixth physical position. Exact five-position assignment still requires one-position-at-a-time correlation.

The numerical structure is strong evidence that `0xA3` FIX writes contain separate horizontal and vertical subfields: vertical changes by `+1`, horizontal by `+8`, and `0x8D` appears in both runs.

### Swing-mode write captures

A controlled test produced:

```text
Vertical swing         -> A3=AE
Horizontal swing       -> A3=B6
Vertical + horizontal  -> A3=80
No swing               -> A3=80
H.DA                   -> A3=60
```

A repeat reproduced `A3=B6` followed by `A3=80` during the horizontal/V+H transition. `0xAE`, `0xB6` and `0x60` are therefore confirmed genuine-adaptor command values. `0x80` is repeatable but does not yet uniquely distinguish Both, Off, a transition, or a neutral combined-louvre state.

Importantly, the IDU itself also emitted unsolicited class-`0x11` pushes of:

```text
A3 80
```

immediately after power-on. Therefore `0x80` is not merely a Wi-Fi-adaptor-only command encoding; it also exists as an IDU-originated state/update value. Its exact semantic remains unresolved.

Readback/state values remain:

```text
31 = no swing
41 = vertical swing
42 = horizontal swing
43 = both swing
60 = H.DA
```

The old assumption that fixed vertical positions were simply `0x50..0x54` is not supported by genuine-adaptor captures and should be treated as obsolete until independently proven on another model/firmware.

## Genuine Wi-Fi adaptor `0xF8` aggregate-state writes

Fan Only at 22°C, fan levels 1..5:

```text
Fan 1 -> F8 45 16 32 00
Fan 2 -> F8 45 16 33 00
Fan 3 -> F8 45 16 34 00
Fan 4 -> F8 45 16 35 00
Fan 5 -> F8 45 16 36 00
```

Heat captures independently confirm the same structure. Examples:

```text
F8 43 18 41 00   Heat, 24°C, Auto fan, Hi POWER off
F8 43 14 41 00   Heat, 20°C, Auto fan, Hi POWER off
F8 43 16 41 00   Heat, 22°C, Auto fan, Hi POWER off
F8 43 16 41 01   Heat, 22°C, Auto fan, Hi POWER on
```

A direct OFF→ON Hi POWER toggle with mode, target and fan unchanged produced:

```text
F8 43 16 41 00
F8 43 16 41 01
```

Thus the four-byte payload is now:

```text
byte 0 = HVAC mode
byte 1 = target temperature, raw °C
byte 2 = commanded fan enum
byte 3 = flags
         bit 0: Hi POWER, 0=off, 1=on
```

The full command frame is:

```text
02 00 03 10 00 00 0A 01 30 01 00 05 F8 <mode> <target> <fan> <flags> <checksum>
```

Every observed `0xF8` write was acknowledged with the generic response ending `F8 3A`.

`E4 +2` is independent of the `F8` commanded fan enum. For example, Fan Only with command `0x33` coincided with `E4 +2 = 0x43`, while Hi POWER Heat with Auto fan later produced raw live values up to `0x61`.

## Genuine Wi-Fi adaptor OFF-timer writes

The genuine adaptor implements the OFF timer as a two-register operation:

```text
30 minutes:
96 00 1E
94 41

1 hour:
96 01 00
94 41

clear timer:
94 42
```

Thus:

```text
0x96 = programmed interval, HH MM
0x94 = enable/state: 41 active, 42 inactive
```

The adaptor writes the interval first and then enables the timer. Clearing the timer does not require zeroing `0x96`; it simply writes `0x94=42`.

The app also exposes an ON/program timer. No direct ON-timer programming capture has yet been made, so `0x90`/`0x92` remain probable rather than confirmed.

## Pure control

```text
PURE ON  -> C7 18
PURE OFF -> C7 10
```

The immediate ACK is the generic register-write acknowledgement ending `C7 6B`.

## Wireless LED control

```text
Wireless LED OFF -> DE 00
Wireless LED ON  -> DE 05
```

Both are acknowledged with the generic register-write ACK ending `DE 54`.

## Official Energy Monitoring traffic

Opening the Energy Monitoring page causes the genuine Wi-Fi adaptor to request, in order:

```text
CC
CD
CE
CF
```

Observed complete response lengths:

```text
CC: length field 0x01EE -> 502-byte frame
CD: length field 0x00D2 -> 218-byte frame
CE: length field 0x0372 -> 890-byte frame
CF: length field 0x015E -> 358-byte frame
```

### Timestamp header

Each block begins its dataset with:

```text
7E MM DD HH mm ss
```

The month is zero-based. Example on 13 September:

```text
7E 08 0D 10 05 26 = 13 Sep, 16:05:38
```

### Calendar record structure

After framing/header/checksum overhead, the data areas divide exactly into calendar-sized record arrays:

```text
CC: 480 bytes = 24 × 20-byte records   -> 24 hourly slots / daily chart
CD: 196 bytes =  7 × 28-byte records   -> 7 daily slots / weekly chart
CE: 868 bytes = 31 × 28-byte records   -> 31 day-of-month slots / monthly chart
CF: 336 bytes = 12 × 28-byte records   -> 12 monthly slots / yearly chart
```

This matches the official app's daily, weekly, monthly and yearly graphs.

### Electrical-energy field

The displayed electrical-consumption accumulator has been identified at logical record offset `+8`.

For the 28-byte `CD`/`CE`/`CF` records it is a little-endian 32-bit integer in Wh. Direct UI-correlated examples:

```text
CD current day:  B5 00 00 00 = 181 Wh -> app 0.18 kWh
CF September:    2E 08 00 00 = 2094 Wh -> app 2.1 kWh
```

Subsequent captures moved both by exactly the same amount:

```text
CD: B5 -> B6     181 -> 182 Wh
CF: 2E08 -> 2F08 2094 -> 2095 Wh
```

Later during heating:

```text
CD current day: AA 01 00 00 = 426 Wh
CE current day: AB 01 00 00 = 427 Wh a few seconds later
CF September:   25 09 00 00 = 2341 Wh
```

The one-Wh difference between consecutive CD and CE polls while the unit was consuming power further confirms a common live electrical-energy accumulator.

For the 20-byte `CC` hourly record, the corresponding `+8` field is little-endian 16-bit Wh. At 16:05 the current 16:00-hour record contained:

```text
E6 00 = 230 Wh
```

while the completed 15:00-hour record contained:

```text
1D 00 = 29 Wh
```

Thus the confirmed logical mapping is:

```text
CC record +8 = hourly electrical consumption, uint16 LE, Wh
CD record +8 = daily electrical consumption, uint32 LE, Wh
CE record +8 = day-of-month electrical consumption, uint32 LE, Wh
CF record +8 = monthly electrical consumption, uint32 LE, Wh
```

Other accumulating fields exist in the 28-byte records and change alongside the energy field, but their meanings are unresolved. Do not currently label them as per-IDU/system totals, runtime, heat/cool split or allocation quantities.

The relationship between `CC`-`CF` and the older/current-component `D8`-`DB` interpretation remains unresolved; do not discard either family solely because the official app polls `CC`-`CF`.

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

Fan testing strongly confirms `+2` as a live fan/airflow feedback quantity. It changes independently of the command enum and tracks measured/derived airflow.

Hi POWER Heat produced the following useful progression:

```text
early/restricted phase: E4 +2 = 0x55
~12-14 min after heat start: E4 +2 = 0x61
one minute later:           E4 +2 = 0x61
```

The sustained `0x61` is the highest live fan/airflow value observed so far on this unit and is the current candidate for full Hi POWER fan output after the startup restriction expires. Keep this as a raw protocol value: `0x61` is hexadecimal and must not be conflated with a displayed decimal engineering value.

Representative frames:

```text
E4 2A 2B 55 00 00 00 00 00
E4 32 33 61 00 00 00 00 00
E4 32 32 61 00 00 00 00 00
```

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

`+6` tracks ODU electrical activity closely enough to remain classified as current-like, but exact physical scope and scaling remain empirical. The component currently applies `raw / 10 * 0.827`.

Representative captures:

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

During the Hi POWER heat run, `E5 +6` rose while `E4 +2` was already on its earlier fan plateau, e.g.:

```text
E5 2D 13 13 53 00 00 34 00
E5 32 11 11 53 00 00 72 00
```

This reinforces that indoor live airflow and ODU electrical loading are independently varying quantities.

A working hypothesis is that `E5 +6` may be an ODU electrical quantity used together with per-IDU `E5 +3` allocation/load-like data for Toshiba's per-IDU energy accounting. This remains an inference requiring simultaneous multi-IDU validation.

## Current diagnostic sweep

Useful current captures:

```text
0x90 = 42              probable ON timer inactive
0x92 = 00 00           probable ON timer value
0x94 = 41 / 42         OFF timer active / inactive
0x96 = HH MM           OFF timer programmed interval
0xA3 = 31/41/42/43/60  readback/state: Off / Vertical / Horizontal / Both / H.DA
0xA3 = 80              observed as genuine IDU-originated push; exact meaning unresolved
0xA4 = xx 00 00        structured louvre state; byte 0 mirrors A3
0xC7 = 18 / 10         Pure ON / OFF
0xCC-0xCF              official Energy Monitoring/history datasets
0xDE = 00 / 05         Wireless LED OFF / ON
0xF8                    aggregate [mode,target,fan,flags], flags bit0=Hi POWER
```

## Evidence labels

- **mapped** — repeatable protocol mapping used by the component.
- **confirmed** — directly correlated with a known physical control and reproduced.
- **confirmed association** — register usage is directly tied to an official app function, while internal record semantics remain unresolved.
- **confirmed values** — values directly observed under known physical control changes, but full read/write semantics may still need testing.
- **probable** — strong structural/behavioural inference awaiting one direct confirmation test.
- **partially characterised** — field exists and some meaning is strongly supported, but exact scale or physical scope is unresolved.
- **experimental** — observed but function not yet assigned.
- **declared only** — present in code/protocol vocabulary without sufficient live testing.
