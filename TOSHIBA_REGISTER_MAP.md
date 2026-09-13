# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. Status labels distinguish genuine Toshiba Wi-Fi-adaptor validation from inherited/component assumptions and still-unresolved fields.

The summary table is an index. Detailed sections below describe framing, payload lengths, field offsets, values, examples, and unresolved points.

## Register summary

| Register | Function | Direction | Current interpretation | Sniffer validation |
| --- | --- | --- | --- | --- |
| `0x80` | Logical/requested power state | R/W / pushed | `0x30` ON/armed, `0x31` OFF. `0x30` may be pushed while an ON timer is armed even though the unit is not yet physically running. | **confirmed** |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | **confirmed** |
| `0x90` | ON timer state/control | R/W / pushed | `0x41` active, `0x42` inactive/cancelled | **confirmed** |
| `0x92` | ON timer programmed delay | Write | `HH MM`; `01 00` = 1 h, `0C 00` = 12 h | **confirmed** |
| `0x94` | OFF timer state/control | R/W | `0x41` active, `0x42` inactive/cancelled | **confirmed** |
| `0x96` | OFF timer programmed delay | R/W | `HH MM`; `00 1E` = 30 min, `01 00` = 1 h | **confirmed** |
| `0xA0` | Fan command | R/W | Quiet `31`; levels `32..36`; Auto `41` | **validated / established** |
| `0xA3` | Swing / louvre / FIX / H.DA | R/W / pushed | Readback `31/41/42/43/60`; genuine adaptor uses packed FIX/swing write values; IDU also pushes `80` | **confirmed register; some sub-encoding unresolved** |
| `0xA4` | Structured louvre state | Read | 3-byte record; byte 0 tracks ordinary `A3` state; bytes 1-2 unresolved | **observed, partially decoded** |
| `0xB0` | HVAC mode | R/W | Auto `41`, Cool `42`, Heat `43`, Dry `44`, Fan `45` | **validated / established** |
| `0xB3` | Target temperature | R/W | Raw integer °C | **confirmed independently in `F8`** |
| `0xBB` | Room temperature | Read / pushed | Raw integer °C; `7F` unavailable | **confirmed** |
| `0xBE` | Outdoor temperature | Read / pushed | Signed byte; `7F` unavailable | **confirmed** |
| `0xC7` | PURE | R/W | `18` ON, `10` OFF | **confirmed** |
| `0xCA` | Structured status/config block | Read | Official adaptor polls about every 60 s; payload repeatedly observed as `00 E8 03 00 00` | **register use confirmed; semantics unresolved** |
| `0xCB` | Self-clean state | Read | Existing mapping `18` running, `10` off | **not deliberately re-tested with sniffer** |
| `0xCC` | Daily-view energy history | Read | 502-byte response; 24 × 20-byte hourly records; record `+8` = hourly Wh | **confirmed** |
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
| `0xEA` | Date/time sync | Write | Multi-byte time/date write; ACK pattern previously mapped | **established** |
| `0xF7` | Special-function selector | R/W | Standard `00`, Hi POWER `01`, Silent 1 `02`, ECO `03`, 8°C `04`, Sleep `05`, Floor `06`, Comfort `07`, Silent 2 `0A`, Fireplace 1 `20`, Fireplace 2 `30` | **enum established; authoritative readback still to test** |
| `0xF8` | Aggregate operating configuration | Write / observed | Four bytes: `[mode][target °C][fan][special-function]` | **strongly confirmed command format** |

## Protocol framing and message classes

All observed frames begin:

```text
02 00 03
```

Bytes `5-6` contain the 16-bit big-endian protocol length:

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

Single-byte control writes normally use a 15-byte frame:

```text
02 00 03 10 00 00 07 01 30 01 00 02 RR VV CS
                                    ^^ ^^ ^^
                                    reg value checksum
```

A generic successful write ACK is normally 16 bytes:

```text
02 00 03 90 00 00 08 01 30 01 00 00 00 01 RR CS
```

The ACK confirms receipt but generally does not echo resulting state.

### Common read request form

Observed register reads use a 14-byte request:

```text
02 00 03 10 00 00 06 01 30 01 00 01 RR CS
```

Response size depends on the register.

### Unsolicited IDU push form

The IDU emits class-`0x11` state/engineering updates. A one-byte payload gives a 15-byte frame; an 8-byte engineering payload (`E4`/`E5`) gives a 22-byte frame. The Wi-Fi adaptor acknowledges these with a 13-byte class-`0x91` frame containing no register field.

Length-field reassembly has been validated against both short traffic and the large official energy frames:

```text
CC = 502 bytes
CD = 218 bytes
CE = 890 bytes
CF = 358 bytes
```

## Register `0x80` — Logical/requested power state

**Direction:** Wi-Fi adaptor → IDU class `0x10`; also IDU-originated class `0x11` pushes.

**Payload length:** 1 byte.

**Write frame length:** 15 bytes.

```text
30 = ON / armed-to-run
31 = OFF
```

Captured writes:

```text
80 30   ON
80 31   OFF
```

The ON-timer experiment exposed an important distinction. When the unit was physically OFF, the adaptor programmed and enabled an ON timer, after which the IDU pushed:

```text
80 30
```

The unit remained stopped and only its timer lamp was illuminated. Therefore `80=30` is a logical/requested ON state and must not be interpreted as proof that the fan or compressor is presently operating.

During that armed state, `E4 +2` remained `00`, providing independent evidence that actual indoor fan operation had not started.

## Register `0x87` — Power Select

**Payload length:** 1 byte.

```text
32 = 50%
4B = 75%
64 = 100%
```

Example:

```text
87 64   100%
```

## Registers `0x90` / `0x92` — ON timer

The ON timer is now directly validated with the genuine adaptor.

### Availability / operating-state constraint

In the Toshiba app, the ON timer is available only while the unit is OFF. It schedules a future start after the selected delay.

This availability rule is observed at the app/UI level. It has **not** yet been established whether the IDU itself rejects an ON-timer command while already running, so implementations should distinguish an observed UI constraint from a proven wire-protocol restriction.

### `0x92` programmed delay

**Direction:** Wi-Fi adaptor → IDU, class `0x10` write.

**Payload length:** 2 bytes.

**Total frame length:** 16 bytes.

```text
+0 = hours
+1 = minutes
```

Confirmed examples:

```text
92 01 00   = 1 hour
92 0C 00   = 12 hours
```

Example complete 12-hour write:

```text
02 00 03 10 00 00 08 01 30 01 00 03 92 0C 00 12
```

### `0x90` enable/state

**Payload length:** 1 byte.

```text
41 = active / enabled
42 = inactive / cancelled
```

Confirmed set sequence:

```text
80 31            ensure logical OFF
F8 ...           stored operating configuration
92 HH MM         program delay
90 41            enable ON timer
IDU push: 80 30  logical state becomes armed ON
```

Confirmed cancellation includes an unsolicited IDU state push followed by the adaptor writing the same inactive value:

```text
IDU -> WiFi   90 42
WiFi -> IDU   90 42
IDU -> WiFi   generic ACK
```

The programmed value and enable state are separate registers.

## Registers `0x94` / `0x96` — OFF timer

The OFF timer mirrors the ON timer structurally.

### Availability / operating-state constraint

In the Toshiba app, the OFF timer is available only while the unit is ON. It schedules a future stop after the selected delay.

As with the ON timer, this is currently an observed app/UI rule. It may be enforced by the IDU as well, but that has not been deliberately tested at protocol level.

### `0x96` programmed delay

**Payload length:** 2 bytes.

```text
+0 = hours
+1 = minutes
```

Confirmed examples:

```text
96 00 1E   = 30 minutes
96 01 00   = 1 hour
```

### `0x94` enable/state

**Payload length:** 1 byte.

```text
41 = active / enable
42 = inactive / cancel
```

Confirmed sequence:

```text
set 30 minutes:
96 00 1E
94 41

set 1 hour:
96 01 00
94 41

cancel:
94 42
```

Clearing the timer does not require zeroing `0x96`.

## Register `0xA0` — Fan command

**Logical payload:** 1 byte.

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

`A3` has distinct state/readback and command encodings.

### Ordinary state/readback

```text
31 = no swing
41 = vertical swing
42 = horizontal swing
43 = both swing
60 = H.DA
```

### Write frame

**Payload length:** 1 byte.

**Total frame length:** 15 bytes.

```text
02 00 03 10 00 00 07 01 30 01 00 02 A3 VV CS
```

ACK:

```text
02 00 03 90 00 00 08 01 30 01 00 00 00 01 A3 8F
```

### FIX position captures

A prescribed vertical sweep produced:

```text
88 89 8A 8B 8C 8D
```

A prescribed horizontal sweep produced:

```text
85 8D 95 9D A5 AD
```

Decimal horizontal values are:

```text
133 141 149 157 165 173
```

The values are exactly `+8` apart. In binary the low three bits remain fixed while the upper position field increments. Conversely, the vertical sequence increments by `+1` while the upper field remains fixed. This is strong evidence of packed horizontal and vertical position subfields rather than literal degree values.

```text
vertical step:   +1
horizontal step: +8
```

The physical UI presents five horizontal FIX positions, with the third position straight ahead. The capture contains six sequential encoded states, so the exact correspondence between the extra state and the five selectable UI positions still needs to be written down from the prescribed test sequence before assigning left/centre/right labels in code.

### Swing / H.DA commands

```text
Vertical swing         -> AE
Horizontal swing       -> B6
Vertical + horizontal  -> 80
No swing transition    -> 80
H.DA                   -> 60
```

`AE`, `B6` and `60` are confirmed command values. `80` is repeatable but its exact state semantic remains unresolved. `A3 80` is also emitted unsolicited by the IDU after power-on.

## Register `0xA4` — Structured louvre state

**Payload length:** 3 bytes.

Observed:

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

## Register `0xB0` — HVAC mode

```text
41 = Auto
42 = Cool
43 = Heat
44 = Dry
45 = Fan Only
```

These values also appear independently as `F8 +0`.

## Register `0xB3` — Target temperature

One-byte raw integer °C.

Examples in `F8 +1`:

```text
13 = 19°C
14 = 20°C
16 = 22°C
18 = 24°C
```

## Register `0xBB` — Room temperature

**Direction:** Read / unsolicited push.

**Payload length:** 1 byte.

```text
7F = unavailable
```

Example:

```text
BB 18 = 24°C
```

## Register `0xBE` — Outdoor temperature

**Direction:** Read / unsolicited push.

**Payload length:** 1 byte, signed temperature.

```text
7F = unavailable
```

Observed examples:

```text
BE 13 = 19°C
BE 14 = 20°C
BE 15 = 21°C
BE 7F = unavailable after ODU data disappears
```

## Register `0xC7` — PURE

**Payload length:** 1 byte.

```text
18 = ON
10 = OFF
```

Both values were captured as genuine adaptor writes.

## Register `0xCA` — Structured status/config block

The genuine adaptor polls `CA` periodically, observed approximately once per minute.

**Response payload length:** 5 bytes.

Repeated payload:

```text
00 E8 03 00 00
```

If bytes `+1..+2` are treated as little-endian uint16 they equal `0x03E8 = 1000`, but there is currently no defensible physical interpretation for that value. `CA` remained unchanged while an ON timer was armed and after timer cancellation, arguing against it being live timer state.

## Register `0xCB` — Self-clean state

Existing component mapping:

```text
18 = running
10 = off
```

Not deliberately re-tested in this sniffer campaign.

## Registers `0xCC`-`0xCF` — Official Energy Monitoring

Opening the Toshiba Energy Monitoring page causes explicit polling in order:

```text
CC
CD
CE
CF
```

Each uses a 14-byte class-`0x10` read request.

### Common dataset timestamp

```text
7E MM DD HH mm ss
```

Month is zero-based.

Example:

```text
7E 08 0D 10 05 26 = 13 Sep, 16:05:38
```

### `0xCC` — Daily view

**Total response frame:** 502 bytes.

**Protocol length:** `0x01EE`.

**Dataset:** 24 × 20-byte hourly records = 480 bytes.

Known field:

```text
+8..+9 = electrical consumption, uint16 LE Wh
```

Example:

```text
E6 00 = 230 Wh
```

### `0xCD` — Weekly view

**Total response frame:** 218 bytes.

**Protocol length:** `0x00D2`.

**Dataset:** 7 × 28-byte daily records = 196 bytes.

Known field:

```text
+8..+11 = electrical consumption, uint32 LE Wh
```

Example:

```text
B5 00 00 00 = 181 Wh = 0.181 kWh
```

### `0xCE` — Monthly view

**Total response frame:** 890 bytes.

**Protocol length:** `0x0372`.

**Dataset:** 31 × 28-byte day records = 868 bytes.

Known field:

```text
+8..+11 = electrical consumption, uint32 LE Wh
```

### `0xCF` — Yearly view

**Total response frame:** 358 bytes.

**Protocol length:** `0x015E`.

**Dataset:** 12 × 28-byte month records = 336 bytes.

Known field:

```text
+8..+11 = electrical consumption, uint32 LE Wh
```

Example:

```text
2E 08 00 00 = 2094 Wh = 2.094 kWh
```

Several other changing counters in the 28-byte records remain unresolved. Do not assign meanings without direct correlation.

## Registers `0xD8`-`0xDB` — Legacy/inherited energy labels

Inherited interpretation:

```text
D8 = daily energy / 24 hourly values
D9 = weekly energy
DA = monthly energy
DB = yearly energy
```

The official app demonstrably uses `CC`-`CF` for its Energy Monitoring path. The relationship to `D8`-`DB` remains unresolved.

## Register `0xDE` — Wireless/Wi-Fi LED

**Payload length:** 1 byte.

```text
00 = LED OFF
05 = LED ON
```

The independent purpose of inherited `DF` remains unresolved.

## Register `0xE0` — Equipment/model information

**Direction:** unsolicited class-`0x11` push.

Carries IDU/ODU identity information in longer structured records. The adaptor acknowledges these pushes with class `0x91`.

## Register `0xE4` — IDU engineering status

**Payload length:** 8 bytes.

**Typical unsolicited total frame:** 22 bytes.

Current layout:

```text
+0 = IDU coil/heat-exchanger-related temperature
+1 = second IDU temperature / junction-related value; exact physical location unresolved
+2 = live fan/airflow feedback quantity
+3..+7 = unresolved
```

Representative `+2` values:

```text
00 = fan stopped / early heating startup / armed ON timer but not yet running
43 = live fan feedback during Fan Only test
55 = earlier/restricted Hi POWER heating phase
61 = sustained full Hi POWER value after roughly 12-14 minutes
33 = after target reduction / Hi POWER removal
```

`61` is the highest sustained raw value observed so far. It is not literal RPM.

## Register `0xE5` — ODU/system engineering status

**Payload length:** 8 bytes.

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

Unavailable/sentinel form:

```text
7F 7F 7F FE FE FE FE 00
```

`+6` correlates strongly with measured electrical current but exact scope/scaling remains empirical.

## Register `0xEA` — Date/time sync

Multi-byte date/time write. The latest campaign did not focus on re-validating each internal field. Existing ACK pattern ends `99 99`.

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

The enum is independently corroborated by `F8 +3`, where `00`, `01`, and `03` have been observed. What remains unresolved is authoritative IDU state feedback when the setting is changed somewhere other than the app.

## Register `0xF8` — Aggregate operating configuration

**Direction:** Wi-Fi adaptor → IDU, class `0x10`.

**Payload length:** 4 bytes.

**Total frame length:** 18 bytes.

```text
+0 = HVAC mode
+1 = target temperature, raw °C
+2 = fan command enum
+3 = special-function selector using F7 enum
```

Examples:

```text
F8 45 16 32 00   Fan, 22°C, fan 1, Standard
F8 45 16 36 00   Fan, 22°C, fan 5, Standard
F8 43 18 41 00   Heat, 24°C, Auto, Standard
F8 43 16 41 01   Heat, 22°C, Auto, Hi POWER
F8 43 13 41 03   Heat, 19°C, Auto, ECO
```

Every observed write receives the generic ACK ending `F8 3A`. The ACK does not echo resulting state.

On shutdown the adaptor can write `80 31` and then reiterate the stored `F8` configuration, so `F8` represents operating configuration, not proof that the unit is presently active.

## Validation labels

- **confirmed** — directly correlated with genuine Toshiba Wi-Fi-adaptor traffic and reproduced.
- **validated / established** — strongly established by repeated component and sniffer observations, even if every enum member was not re-exercised in the latest campaign.
- **partially decoded** — register and some fields are validated, but exact scaling or remaining subfields are unresolved.
- **not independently validated** — inherited/component interpretation awaiting direct genuine-adaptor confirmation.
- **declared only** — exists in inherited protocol vocabulary without sufficient live validation.
