# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. Status labels distinguish genuine Toshiba Wi-Fi-adaptor validation from inherited/component assumptions and still-unresolved fields.

The summary table is an index. Detailed sections below describe frame class, total frame length, payload layout, value encoding, examples, and unresolved fields for the registers we have characterised sufficiently.

## Register summary

| Register | Function | Direction | Current interpretation | Sniffer validation |
| --- | --- | --- | --- | --- |
| `0x80` | Power | R/W | `0x30` ON, `0x31` OFF | **confirmed** |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | **confirmed** |
| `0x90` | ON/program timer state/control | observed | `0x42` seen with no ON timer active; likely companion to `0x92` | **not yet directly tested** |
| `0x92` | ON/program timer programmed value | observed | Two-byte payload; baseline `00 00`; likely `HH MM` | **not yet directly tested** |
| `0x94` | OFF timer state/control | R/W | `0x41` active, `0x42` clear/inactive | **confirmed** |
| `0x96` | OFF timer programmed value | R/W | `HH MM`; 30 min=`00 1E`, 1 h=`01 00` | **confirmed** |
| `0xA0` | Fan command | R/W | Quiet `31`; levels `32..36`; Auto `41` | **validated / established** |
| `0xA3` | Swing / louvre / FIX / H.DA | R/W / pushed | Readback `31/41/42/43/60`; genuine adaptor uses separate packed write encoding; IDU also pushes `80` | **confirmed register; some sub-encoding unresolved** |
| `0xA4` | Structured louvre state | Read | 3-byte record; byte 0 tracks ordinary `A3` state; bytes 1-2 unresolved | **observed, partially decoded** |
| `0xB0` | HVAC mode | R/W | Auto `41`, Cool `42`, Heat `43`, Dry `44`, Fan `45` | **validated / established** |
| `0xB3` | Target temperature | R/W | Raw integer °C | **confirmed independently in `F8`** |
| `0xBB` | Room temperature | Read / pushed | Raw integer °C; `7F` unavailable | **confirmed** |
| `0xBE` | Outdoor temperature | Read / pushed | Signed byte; `7F` unavailable | **confirmed** |
| `0xC7` | PURE | R/W | `18` ON, `10` OFF | **confirmed** |
| `0xCA` | Structured status/config block | Read | Official adaptor periodically polls it; payload observed `00 E8 03 00 00` | **register use confirmed; semantics unresolved** |
| `0xCB` | Self-clean state | Read | Existing mapping `18` running, `10` off | **not deliberately re-tested with sniffer** |
| `0xCC` | Daily-view energy history | Read | 502-byte response; 24 × 20-byte hourly records; record `+8` = hourly electrical Wh | **confirmed** |
| `0xCD` | Weekly-view energy history | Read | 218-byte response; 7 × 28-byte daily records; record `+8` = daily uint32 LE Wh | **confirmed** |
| `0xCE` | Monthly-view energy history | Read | 890-byte response; 31 × 28-byte day records; record `+8` = daily uint32 LE Wh | **confirmed** |
| `0xCF` | Yearly-view energy history | Read | 358-byte response; 12 × 28-byte month records; record `+8` = monthly uint32 LE Wh | **confirmed** |
| `0xD8` | Legacy/component daily-energy interpretation | Read | Existing component treats this as 24 hourly values | **not validated against official app path** |
| `0xD9` | Legacy weekly energy | declared | No current parser/use | **declared only** |
| `0xDA` | Legacy monthly energy | declared | No current parser/use | **declared only** |
| `0xDB` | Legacy yearly energy | declared | No current parser/use | **declared only** |
| `0xDE` | Wireless/Wi-Fi LED | Write | `00` OFF, `05` ON | **confirmed** |
| `0xDF` | Additional Wi-Fi-related control | Write | Existing component uses it alongside `DE` | **not independently validated** |
| `0xE0` | Equipment/model information | pushed | IDU/ODU identity records in unsolicited class-`0x11` traffic | **confirmed** |
| `0xE4` | IDU engineering status | pushed/read | 8-byte record; `+2` is live fan/airflow feedback, not fan command and not literal RPM | **confirmed field purpose; scale unresolved** |
| `0xE5` | ODU/system engineering status | pushed/read | 8-byte engineering record; `+6` is current-like and tracks ODU electrical activity | **partially decoded** |
| `0xEA` | Date/time sync | Write | Multi-byte time/date write; ACK pattern previously mapped | **established, not part of latest sniffer validation** |
| `0xF7` | Special-function selector | R/W | Standard `00`, Hi POWER `01`, Silent 1 `02`, ECO `03`, 8°C `04`, Sleep `05`, Floor `06`, Comfort `07`, Silent 2 `0A`, Fireplace 1 `20`, Fireplace 2 `30` | **enum established; authoritative readback still to test** |
| `0xF8` | Aggregate operating configuration | Write / observed | Four bytes: `[mode][target °C][fan][special-function]` | **strongly confirmed command format** |

## Protocol framing and message classes

All observed frames begin:

```text
02 00 03
```

Bytes `5-6` contain the 16-bit big-endian protocol length. Complete frame length is:

```text
frame_length = protocol_length + 8
```

Observed message classes:

```text
0x10  Wi-Fi adaptor request/write     register at byte 12
0x90  IDU response                    register at byte 14
0x11  IDU unsolicited/push            register at byte 12
0x91  Wi-Fi ACK of unsolicited push   no register field
```

### Common short write form

Single-byte control writes generally use a 15-byte frame:

```text
02 00 03 10 00 00 07 01 30 01 00 02 RR VV CS
                                    ^^ ^^ ^^
                                    reg value checksum
```

A generic successful write ACK is normally 16 bytes:

```text
02 00 03 90 00 00 08 01 30 01 00 00 00 01 RR CS
```

The ACK confirms receipt but generally does not echo the resulting state.

### Common read request form

Observed register reads use a 14-byte request:

```text
02 00 03 10 00 00 06 01 30 01 00 01 RR CS
```

Response size depends on the register.

### Unsolicited IDU push form

The IDU also emits class-`0x11` state/engineering updates. For a one-byte register payload the total frame is 15 bytes; for an 8-byte engineering payload (`E4`/`E5`) it is 22 bytes.

The Wi-Fi adaptor acknowledges these with a 13-byte class-`0x91` frame. That ACK contains no register field.

### Sniffer/reassembly validation

Length-field reassembly has been validated against both short 13-22 byte traffic and the large official energy frames:

```text
CC = 502 bytes
CD = 218 bytes
CE = 890 bytes
CF = 358 bytes
```

The logger may split a complete frame into multiple display lines, but this is only presentation chunking; protocol reassembly occurs first.

## Register `0x80` — Power

**Direction:** Wi-Fi adaptor → IDU, class `0x10` write.

**Write payload length:** 1 byte.

**Total write frame length:** 15 bytes.

```text
30 = ON
31 = OFF
```

Captured ON example:

```text
02 00 03 10 00 00 07 01 30 01 00 02 80 30 02
```

Captured OFF example:

```text
02 00 03 10 00 00 07 01 30 01 00 02 80 31 01
```

Both receive the generic 16-byte `0x90` ACK ending in register `80`.

Power state is separate from `F8`. On shutdown the official adaptor can write `80 31` and then reiterate the stored `F8` configuration.

## Register `0x87` — Power Select

**Direction:** Wi-Fi adaptor → IDU, class `0x10` write.

**Payload length:** 1 byte.

**Total write frame length:** 15 bytes.

```text
32 = 50%
4B = 75%
64 = 100%
```

Example captured at 100%:

```text
02 00 03 10 00 00 07 01 30 01 00 02 87 64 C7
```

The IDU returns the generic 16-byte write ACK.

## Registers `0x90` / `0x92` — ON/program timer

These remain deliberately provisional.

Observed idle values:

```text
90 = 42
92 = 00 00
```

The symmetry with confirmed OFF-timer registers `94`/`96` strongly suggests:

```text
90 = ON/program timer enable/state
92 = ON/program timer HH MM
```

However, the app's ON/program timer has not yet been deliberately exercised under the sniffer. Do not promote these values to confirmed until a set/change/clear capture is made.

## Registers `0x94` / `0x96` — OFF timer

The OFF timer is a two-register operation.

### `0x96` programmed interval

**Direction:** Wi-Fi adaptor → IDU, class `0x10` write.

**Payload length:** 2 bytes.

```text
+0 = hours
+1 = minutes
```

Examples:

```text
30 minutes -> 96 00 1E
1 hour     -> 96 01 00
```

### `0x94` enable/state

**Payload length:** 1 byte.

```text
41 = active / enable
42 = inactive / clear
```

Official sequence:

```text
set 30 minutes:
96 00 1E
94 41

set 1 hour:
96 01 00
94 41

clear:
94 42
```

The programmed interval and enable state are independent. Clearing does not require zeroing `0x96`.

## Register `0xA0` — Fan command

**Direction:** R/W in the established component protocol; command values independently corroborated by `F8` captures.

**Logical payload:** one-byte fan selector.

```text
31 = Quiet
32 = Low / level 1
33 = Low-Med / level 2
34 = Medium / level 3
35 = Med-High / level 4
36 = High / level 5
41 = Auto
```

These are command enums, not live fan speed. Live fan/airflow feedback is carried by `E4 +2`.

## Register `0xA3` — Louvre / FIX / swing / H.DA

`A3` has different state/readback and command encodings.

### Ordinary state/readback values

```text
31 = no swing
41 = vertical swing
42 = horizontal swing
43 = both swing
60 = H.DA
```

### Write frame

**Direction:** Wi-Fi adaptor → IDU, class `0x10`.

**Payload length:** 1 byte.

**Total frame length:** 15 bytes.

```text
02 00 03 10 00 00 07 01 30 01 00 02 A3 VV CS
```

ACK:

```text
02 00 03 90 00 00 08 01 30 01 00 00 00 01 A3 8F
```

The ACK does not contain the resulting louvre state.

### FIX position captures

Vertical sweep:

```text
88 89 8A 8B 8C 8D
```

Horizontal sweep:

```text
85 8D 95 9D A5 AD
```

The physical UI exposes five FIX positions on each axis. Six values occurred during each sweep, so one value in each sequence is a transition/default/current-state effect rather than a sixth physical position.

The numerical pattern strongly supports packed horizontal and vertical subfields:

```text
vertical changes:   +1
horizontal changes: +8
```

Exact one-to-one mapping of the five physical positions still requires a position-by-position test.

### Swing / H.DA commands

```text
Vertical swing         -> AE
Horizontal swing       -> B6
Vertical + horizontal  -> 80
No swing transition    -> 80
H.DA                   -> 60
```

`AE`, `B6` and `60` are confirmed command values. `80` is repeatable, but its exact semantic is unresolved.

Importantly, `A3 80` is also emitted by the IDU itself in unsolicited class-`0x11` traffic immediately after power-on. Therefore `80` is not merely a Wi-Fi-adaptor-only command token.

## Register `0xA4` — Structured louvre state

**Direction:** Read.

**Payload length:** 3 bytes.

Observed examples:

```text
41 00 00
42 00 00
43 00 00
60 00 00
```

Current interpretation:

```text
+0 = ordinary A3-style louvre/swing state
+1 = unresolved
+2 = unresolved
```

One isolated `53 2D 42` capture occurred during heavy sweep traffic and remains untrusted until reproduced.

## Register `0xB0` — HVAC mode

**Logical payload:** 1 byte.

```text
41 = Auto
42 = Cool
43 = Heat
44 = Dry
45 = Fan Only
```

These values are independently visible as `F8 +0`, which strongly corroborates the established mapping.

## Register `0xB3` — Target temperature

**Logical payload:** 1 byte.

Normal operation uses raw integer °C.

Examples independently captured in `F8 +1`:

```text
13 = 19°C
14 = 20°C
16 = 22°C
18 = 24°C
```

## Register `0xBB` — Room temperature

**Direction:** Read and unsolicited IDU push (`0x11`).

**Payload length:** 1 byte.

```text
00..7E = raw integer °C in observed normal range
7F     = invalid / unavailable
```

Example:

```text
BB 18 = 24°C
```

A one-byte unsolicited push uses a 15-byte total frame and is acknowledged by the adaptor with class `0x91`.

## Register `0xBE` — Outdoor temperature

**Direction:** Read and unsolicited IDU push (`0x11`).

**Payload length:** 1 byte.

The normal value is interpreted as a signed byte temperature.

```text
7F = invalid / unavailable
```

Examples:

```text
BE 13 = 19°C
BE 14 = 20°C
BE 15 = 21°C
BE 7F = unavailable after ODU data disappears
```

## Register `0xC7` — PURE

**Direction:** R/W.

**Payload length:** 1 byte.

```text
18 = ON
10 = OFF
```

Both directions were captured as genuine Toshiba Wi-Fi-adaptor writes. The immediate ACK is the generic write acknowledgement ending in register `C7`.

## Register `0xCA` — Structured status/config block

The genuine adaptor periodically requests `CA`.

Observed payload:

```text
00 E8 03 00 00
```

**Observed payload length:** 5 bytes.

The register's use by the official adaptor is confirmed; individual field semantics are not yet decoded.

## Register `0xCB` — Self-clean state

Existing component mapping:

```text
18 = running
10 = off
```

This mapping has not yet been deliberately re-tested with the genuine-adaptor sniffer, so it remains established component knowledge rather than part of the latest validation campaign.

## Registers `0xCC`-`0xCF` — Official Energy Monitoring

Opening the Toshiba Energy Monitoring page causes explicit polling in order:

```text
CC
CD
CE
CF
```

All four use a 14-byte class-`0x10` read request:

```text
02 00 03 10 00 00 06 01 30 01 00 01 RR CS
```

### Common response header

Each dataset begins with a six-byte timestamp immediately after the register byte:

```text
7E MM DD HH mm ss
```

The month is zero-based.

Example:

```text
7E 08 0D 10 05 26 = 13 Sep, 16:05:38
```

### `0xCC` — Daily view / hourly records

**Total response frame:** 502 bytes.

**Protocol length field:** `0x01EE`.

**Dataset:** 24 × 20-byte records = 480 bytes.

Each slot corresponds to one hour of the day.

Known record field:

```text
+8..+9 = electrical consumption, uint16 little-endian Wh
```

Example current-hour value:

```text
E6 00 = 230 Wh
```

The 20-byte hourly record contains additional fields that remain unresolved.

### `0xCD` — Weekly view / daily records

**Total response frame:** 218 bytes.

**Protocol length field:** `0x00D2`.

**Dataset:** 7 × 28-byte records = 196 bytes.

Each slot corresponds to one day in the weekly graph.

Known record field:

```text
+8..+11 = electrical consumption, uint32 little-endian Wh
```

Example:

```text
B5 00 00 00 = 181 Wh = 0.181 kWh
```

The app displayed `0.18 kWh` for that day.

### `0xCE` — Monthly view / day-of-month records

**Total response frame:** 890 bytes.

**Protocol length field:** `0x0372`.

**Dataset:** 31 × 28-byte records = 868 bytes.

Each slot corresponds to a day of the month.

Known record field:

```text
+8..+11 = electrical consumption, uint32 little-endian Wh
```

A capture a few seconds after the corresponding `CD` poll showed the same current-day accumulator advanced by 1 Wh, confirming that `CD` and `CE` expose the same electrical-consumption quantity over different calendar views.

### `0xCF` — Yearly view / monthly records

**Total response frame:** 358 bytes.

**Protocol length field:** `0x015E`.

**Dataset:** 12 × 28-byte records = 336 bytes.

Each slot corresponds to a month.

Known record field:

```text
+8..+11 = electrical consumption, uint32 little-endian Wh
```

Example:

```text
2E 08 00 00 = 2094 Wh = 2.094 kWh
```

The app displayed `2.1 kWh` for September.

### Common 28-byte record observations

The `CD`/`CE`/`CF` records contain several other changing 32-bit and smaller quantities plus repeated sentinels such as:

```text
7F FF FF FF
```

Those additional fields are clearly meaningful but remain unassigned. Do not label them as runtime, heat/cool split, delivered heat, compressor energy, or allocation until directly correlated.

### `D8`-`DB` relationship

The inherited component contains `D8`-`DB` energy labels, but the official app demonstrably uses `CC`-`CF` for Energy Monitoring. The relationship between the two families remains unresolved and must not be assumed.

## Registers `0xD8`-`0xDB` — Legacy/inherited energy labels

Current inherited interpretation:

```text
D8 = daily energy / 24 hourly values
D9 = weekly energy
DA = monthly energy
DB = yearly energy
```

Only `D8` has an existing component interpretation; `D9`-`DB` are largely declared protocol vocabulary. None has yet been validated as the official app's energy path.

## Register `0xDE` — Wireless/Wi-Fi LED

**Direction:** Wi-Fi adaptor → IDU, class `0x10` write.

**Payload length:** 1 byte.

```text
00 = LED OFF
05 = LED ON
```

The official adaptor used `DE` directly in the controlled test. The independent purpose of legacy/component register `DF` remains unresolved.

## Register `0xE0` — Equipment/model information

**Direction:** unsolicited IDU push, class `0x11`.

This register carries IDU/ODU identity information and has been observed directly from the genuine adaptor connection.

Payloads are longer structured records rather than one-byte state values. Exact subfield documentation should be derived from the separately decoded equipment-information captures rather than guessed here.

The key protocol fact is that `E0` is IDU-originated state/identity traffic and the adaptor acknowledges it with class `0x91`.

## Register `0xE4` — IDU engineering status

**Direction:** unsolicited push and readable engineering state.

**Payload length:** 8 bytes.

**Typical unsolicited total frame length:** 22 bytes.

Current layout:

```text
+0 = IDU heat-exchanger / coil-related temperature
+1 = second IDU temperature / junction-related value; exact physical location unresolved
+2 = live fan/airflow feedback quantity
+3 = unresolved
+4 = unresolved
+5 = unresolved
+6 = unresolved
+7 = unresolved
```

`+2` is confirmed as a live fan/airflow feedback quantity because it varies independently of the commanded fan enum.

Representative values:

```text
E4 +2 = 00   fan stopped / early heating startup
E4 +2 = 43   live fan feedback during Fan Only test
E4 +2 = 55   earlier/restricted Hi POWER heating phase
E4 +2 = 61   sustained full Hi POWER value after roughly 12-14 minutes
E4 +2 = 33   after target reduction / Hi POWER removal
```

`61` is the highest sustained raw value observed so far on this unit. It is a raw protocol value, not literal RPM.

Example full payload:

```text
E4 32 32 61 00 00 00 00 00
   ^^ ^^ ^^ ----------------
   +0 +1 +2      unresolved
```

## Register `0xE5` — ODU/system engineering status

**Direction:** unsolicited push and readable engineering state.

**Payload length:** 8 bytes.

**Typical unsolicited total frame length:** 22 bytes.

Current interpretation:

```text
+0 = temperature-like engineering quantity
+1 = temperature-like engineering quantity
+2 = temperature-like engineering quantity
+3 = load/allocation-like quantity; falls to zero when demand disappears
+4 = unresolved
+5 = unresolved
+6 = current-like quantity tracking ODU electrical activity
+7 = unresolved
```

Representative heating payload:

```text
E5 3A 11 12 59 00 00 6A 00
```

`+6` correlates strongly with measured electrical current, but the exact physical scope and engineering scale remain empirical. Any correction factor applied by the ESPHome component is implementation calibration, not part of the proven wire protocol.

Observed unavailable/sentinel form:

```text
7F 7F 7F FE FE FE FE 00
```

During target reduction before shutdown, both `E5 +3` and `E5 +6` fell to zero while the indoor fan continued running. This supports their association with active compressor/load demand rather than HVAC mode alone.

## Register `0xEA` — Date/time sync

**Direction:** Write.

This is a multi-byte time/date command previously mapped by the component. The latest sniffer campaign did not focus on re-validating its internal fields.

The ACK pattern ends with:

```text
99 99
```

Detailed byte layout should remain tied to the existing implementation until a deliberate genuine-adaptor time-sync capture is made.

## Register `0xF7` — Special-function selector

Established enum:

```text
00 = Standard
01 = Hi POWER
02 = Silent 1
03 = ECO
04 = 8°C
05 = Sleep
06 = Floor
07 = Comfort
0A = Silent 2
20 = Fireplace 1
30 = Fireplace 2
```

This enum is independently corroborated by `F8 +3`, where `00`, `01`, and `03` have been observed in genuine-adaptor traffic under corresponding operating conditions.

What remains unresolved is authoritative state feedback: the physical-remote test is still required to determine whether the IDU pushes or returns the active special function via `F7`, `F8`, or another state register.

## Register `0xF8` — Aggregate operating configuration

`F8` is now one of the best-characterised control messages.

### Write frame

**Direction:** Wi-Fi adaptor → IDU, class `0x10`.

**Payload length:** 4 bytes.

**Total frame length:** 18 bytes.

```text
02 00 03 10 00 00 0A 01 30 01 00 05 F8 MM TT FF SS CS
                                             |  |  |  |
                                             |  |  |  +-- special function
                                             |  |  +----- fan command
                                             |  +-------- target °C
                                             +----------- HVAC mode
```

Payload structure:

```text
+0 = HVAC mode
+1 = target temperature, raw °C
+2 = commanded fan enum
+3 = special-function selector using F7 enum
```

### Observed mode values

```text
41 = Auto
42 = Cool
43 = Heat
44 = Dry
45 = Fan Only
```

### Observed fan values

```text
31 = Quiet
32..36 = manual fan levels
41 = Auto
```

### Observed special-function values

```text
00 = Standard
01 = Hi POWER
03 = ECO
```

The remaining `F7` enum values are established protocol vocabulary but have not all been re-exercised inside `F8` during this sniffer campaign.

### Captured examples

Fan Only at 22°C:

```text
F8 45 16 32 00
F8 45 16 33 00
F8 45 16 34 00
F8 45 16 35 00
F8 45 16 36 00
```

Heat:

```text
F8 43 18 41 00   Heat, 24°C, Auto, Standard
F8 43 14 41 00   Heat, 20°C, Auto, Standard
F8 43 16 41 00   Heat, 22°C, Auto, Standard
F8 43 16 41 01   Heat, 22°C, Auto, Hi POWER
F8 43 13 41 03   Heat, 19°C, Auto, ECO
```

A controlled Hi POWER OFF→ON toggle changed only byte `+3`:

```text
F8 43 16 41 00
F8 43 16 41 01
```

The later `03` capture matched the established ECO enum, proving byte `+3` is better understood as the special-function selector rather than a generic bitfield.

### ACK

Every observed `F8` write is followed by the generic 16-byte response ending:

```text
F8 3A
```

The ACK does not echo the resulting operating state.

Therefore the `F8` command format is decoded, but authoritative IDU readback of the active special function remains unresolved.

## Validation labels

- **confirmed** — directly correlated with genuine Toshiba Wi-Fi-adaptor traffic and reproduced.
- **validated / established** — mapping is strongly established by repeated component and sniffer observations, even if not every enum member was re-exercised in the latest campaign.
- **partially decoded** — register and some fields are validated, but exact scaling or remaining subfields are unresolved.
- **not yet directly tested** — plausible inherited/structural interpretation awaiting a deliberate genuine-adaptor test.
- **declared only** — exists in inherited protocol vocabulary without sufficient live validation.
