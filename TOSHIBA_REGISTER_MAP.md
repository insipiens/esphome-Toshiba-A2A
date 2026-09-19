# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. Status labels distinguish genuine Toshiba Wi-Fi-adaptor validation from inherited/component assumptions and still-unresolved fields.

The summary table is an index. Detailed sections below describe framing, payload lengths, field offsets, values, examples, and unresolved points.

## Register summary

| Register | Function | Direction | Current interpretation | Sniffer validation |
| --- | --- | --- | --- | --- |
| `0x80` | Logical/requested power state | R/W / pushed | `0x30` ON/armed, `0x31` OFF. `0x30` may be pushed while an ON timer is armed even though the unit is not yet physically running. | **confirmed value mapping; armed-state interpretation draft** |
| `0x87` | Power Select | R/W | `0x32` 50%, `0x4B` 75%, `0x64` 100% | **confirmed** |
| `0x90` | ON timer state/control | R/W / pushed | `0x41` armed/enabled; IDU pushes `0x42` at timer expiry | **confirmed on P2KVSG and J2FVG** |
| `0x92` | ON timer programmed delay | Write | `HH MM`; `00 1E` = 30 min, `01 00` = 1 h, `0C 00` = 12 h | **confirmed on P2KVSG and J2FVG** |
| `0x94` | OFF timer state/control | R/W | `0x41` active, `0x42` inactive/cancelled | **confirmed on P2KVSG and J2FVG** |
| `0x96` | OFF timer programmed delay | R/W | `HH MM`; `00 1E` = 30 min, `01 00` = 1 h | **confirmed on P2KVSG and J2FVG** |
| `0x99` | Clock/calendar and programme structure | Write / pushed variants observed | Long Wi-Fi→IDU onboarding frame begins `[year-1900][month-1][day][hour][minute][second][weekday]`; other long `0x99` structures may carry programme/schedule data | **clock/calendar prefix confirmed from two independent captures; wider structure partially decoded** |
| `0xA0` | Fan command | R/W | Quiet `31`; levels `32..36`; Auto `41` | **validated / established** |
| `0xA3` | Swing / louvre / FIX / H.DA | R/W / pushed | Readback `31/41/42/43/60`; command encoding is family-specific: P2 uses packed H/V FIX values, while J2FVG uses vertical FIX `50..54`; IDU also pushes `80` | **confirmed register; family-specific FIX encoding confirmed on P2; J2FVG mapping directly verified on B13J2** |
| `0xA4` | Structured louvre state | Read | 3-byte record; byte 0 tracks ordinary `A3` state; bytes 1-2 unresolved | **observed, partially decoded** |
| `0xB0` | HVAC mode | R/W | Auto `41`, Cool `42`, Heat `43`, Dry `44`, Fan `45` | **validated / established** |
| `0xB3` | Target temperature | R/W | Raw integer °C | **confirmed independently in `F8`** |
| `0xBB` | Room temperature | Read / pushed | Raw integer °C; `7F` unavailable | **confirmed** |
| `0xBE` | Outdoor temperature | Read / pushed | Signed byte; `7F` unavailable | **confirmed** |
| `0xC7` | PURE | R/W | `18` ON, `10` OFF | **confirmed** |
| `0xCA` | Filter/service information | Read | `+1..+2` = filter interval in hours, uint16 LE; e.g. `E8 03` = 1000 h, `F4 01` = 500 h. Other bytes unresolved. | **partially decoded / confirmed interval field** |
| `0xCB` | Defrost control/state | Write / pushed | commands: `00` stop, `01` Strong Defrost request, `02` Normal Defrost request; pushed state: `10` inactive, `11` Strong Defrost active; `12` is the expected Normal Defrost active partner but remains unobserved | **commands `00/01/02` confirmed; states `10/11` confirmed; `12` inferred only** |
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
| `0xE0` | Equipment/model information | pushed | Two 50-byte records: IDU then ODU; each contains model plus three fixed ASCII identity fields; older J2 firmware can report IDU model as literal `NULL` while other identity fields remain populated | **confirmed across B13J2, B10J2 and B10P2** |
| `0xE4` | IDU engineering status | pushed/read | 8-byte record; `+2` is live IDU fan-speed feedback at approximately 10 rpm/count (RPM/10), distinct from the fan command enum; `FE/FF` are treated as unavailable rather than speed values | **confirmed field purpose/scale; unavailable handling based on observed sentinel form** |
| `0xE5` | ODU/system engineering status | pushed/read | 8-byte engineering record; `+6` is current-like and tracks ODU electrical activity | **partially decoded** |
| `0xEA` | Date/time sync | Write | Multi-byte time/date write; ACK pattern previously mapped | **established** |
| `0xF7` | Special-function selector | R/W | Standard `00`, Hi POWER `01`, Silent 1 `02`, ECO `03`, 8°C `04`, Sleep `05`, Floor `06`, Comfort `07`, Silent 2 `0A`, Fireplace 1 `20`, Fireplace 2 `30` | **enum established; authoritative readback still to test** |
| `0xF8` | Aggregate operating configuration | Write / observed | Four bytes: `[mode][target °C][fan][special-function]` | **confirmed across P2KVSG and J2FVG genuine-adaptor captures** |

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

### DRAFT interpretation — logical/armed ON state

During an ON-timer experiment, the adaptor first set `80 31`, then programmed and enabled the ON timer. The IDU subsequently pushed:

```text
80 30
```

while the unit remained physically stopped and only the timer lamp was illuminated. In the same armed state, `E4 +2` remained `00`.

Observed fact: `80 30` can be present while the indoor fan is not running.

Draft inference: `80 30` is better understood as logical/requested/armed ON state rather than proof of present physical operation. This distinction should remain provisional until reproduced across more timer and non-timer transitions.

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

The ON timer is directly validated with the genuine adaptor and has also been reproduced from the ESP controller.

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
92 00 1E   = 30 minutes
92 01 00   = 1 hour
92 0C 00   = 12 hours
```

The exposed controller UI currently uses the same `HH MM` encoding across 30 minutes through 12 hours in 30-minute steps. The genuine remote does not expose every one of those values. A direct ESP test of the non-remote value `92 0A 1E` (10 h 30 min) received the normal `0x92` ACK, `90 41` was then ACKed, and the IDU entered the usual armed `0x80 = 0x30` state with zero airflow. This establishes acceptance/arming of that value at protocol level; expiry at 10 h 30 min has not yet been observed.

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
IDU push: 80 30  draft interpretation: logical state becomes armed ON
```

Confirmed manual cancellation includes an unsolicited IDU state push followed by the adaptor writing the same inactive value:

```text
IDU -> WiFi   90 42
WiFi -> IDU   90 42
IDU -> WiFi   generic ACK
```

The programmed value and enable state are separate registers. In a direct ESP-controller test, `92 00 1E` and `90 41` both received normal ACKs, `0x90` read back `0x41`, and the IDU then reported `0x80 = 0x30` while airflow remained zero, matching the previously observed armed-not-yet-running state.

### ON-timer expiry

At expiry the IDU autonomously pushes:

```text
90 42
```

No F8/start command from the Wi-Fi adaptor precedes the transition. In the J2FVG 30-minute test, non-zero `E4 +2` fan feedback appeared about 10 seconds later, confirming timer expiry and physical blower startup are separate events.

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

A direct ESP-controller test while the unit was running in Fan Only programmed `96 00 1E` (30 minutes) and then `94 41`. Both writes received the normal ACKs and the unit continued operating normally with live `E4 +2` fan feedback, confirming that the OFF timer command path is accepted and arms without disturbing the current operating mode. Expiry at 30 minutes was not included in this capture.

## Register `0x99` — Clock/calendar and programme structure

A long class-`0x10` `0x99` frame is sent by the genuine Wi-Fi adaptor during onboarding/pairing. Two independent captures prove that the leading bytes are a clock/calendar synchronisation record rather than a capability query.

Observed leading payloads:

```text
7E 08 10 0F 12 13 03 ...   captured at about 2026-09-16 15:18:19
7E 08 10 0F 21 14 03 ...   captured at about 2026-09-16 15:33:20
```

They decode exactly as:

```text
+0 = year since 1900   0x7E = 126 -> 2026
+1 = zero-based month  0x08 = September
+2 = day of month      0x10 = 16
+3 = hour              0x0F = 15
+4 = minute
+5 = second
+6 = weekday           0x03 = Wednesday when Sunday=0
+7 = 00
+8 = 00
```

In these onboarding captures the remaining fixed-width structure is filled with `FF`, and the IDU returns a short ACK-like response for register `0x99`.

This establishes the clock/calendar prefix for the Wi-Fi→IDU long form. It does **not** establish that every long `0x99` frame has only this purpose: other previously observed long `0x99` traffic, including IDU-originated programme/schedule-like structures, remains only partially decoded.

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

These are command enums, not live fan speed. Live fan-speed feedback is carried independently by `E4 +2`.

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

### J2FVG vertical FIX positions

The J2FVG family uses the following vertical FIX command mapping, directly verified by a genuine-adaptor sweep on `RAS-B13J2FVG-E1`:

```text
A3 50 = Position 1 / top
A3 51 = Position 2
A3 52 = Position 3 / centre
A3 53 = Position 4
A3 54 = Position 5 / bottom
```

Each command received the normal `A3` ACK. These values are command encodings; the ACK does not echo the selected position as authoritative state/readback.

This is a family-specific difference from the P2 packed two-axis FIX encoding below. Implementations must therefore select the A3 FIX encoder from the identified IDU family rather than applying the P2 packed encoding to J2FVG consoles.

### P2 FIX position captures

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

## Register `0xCA` — Filter/service information

The genuine adaptor polls `CA` periodically, observed approximately once per minute while the unit's normal section is open in the Toshiba app.

**Response payload length:** 5 bytes.

Current layout:

```text
+0       status/flag byte                  unresolved
+1..+2   configured filter interval hours  uint16 little-endian
+3..+4   unresolved
```

Confirmed examples from the app's **Set filter time** control:

```text
00 E8 03 00 00   -> configured filter time = 1000 hours
01 F4 01 00 00   -> configured filter time = 500 hours
```

`E8 03` = `0x03E8` = 1000 and `F4 01` = `0x01F4` = 500, confirming the little-endian hours field at `+1..+2`.

The simultaneous `+0` change from `00` to `01` is observed but not yet assigned; it may represent a status/configuration flag and should not be named without another controlled test. The meaning of `+3..+4` is also unresolved.

## Register `0xCB` — Defrost control/state

Direct genuine-adaptor testing now shows that `CB` carries both defrost requests and IDU-originated defrost state, with different value ranges depending on direction/message class.

### Wi-Fi adaptor -> IDU commands

Confirmed one-byte writes:

```text
CB 00 = stop/cancel forced defrost
CB 01 = request Strong Defrost
CB 02 = request Normal Defrost
```

`CB 01` was captured when the Toshiba app explicitly started **Strong Defrost**. `CB 02` was captured from the separate **Start Defrost** action used for normal defrost.

A generic class-`0x90` ACK confirms that the IDU received the request. It does **not** prove that normal defrost was actually entered. In one controlled test, `CB 02` was sent twice and acknowledged twice, but the IDU refused to enter normal defrost under the current operating conditions and emitted no active-state transition.

### IDU -> Wi-Fi pushed state

Confirmed class-`0x11` state pushes:

```text
CB 10 = defrost inactive / stopped
CB 11 = Strong Defrost active
```

Observed Strong Defrost sequence:

```text
WiFi -> IDU   CB 01   request Strong Defrost
IDU  -> WiFi  ACK
IDU  -> WiFi  CB 11   Strong Defrost active
```

Observed stop sequence:

```text
WiFi -> IDU   CB 00   stop/cancel forced defrost
IDU  -> WiFi  ACK
IDU  -> WiFi  CB 10   inactive / stopped
```

This produces a clear command/state pattern:

```text
commands: 00, 01, 02
states:   10, 11, 12?
```

`CB 12` is therefore the obvious candidate for **Normal Defrost active**, but it has not yet been observed and must remain explicitly provisional until a successful normal-defrost activation produces the corresponding push.

### Additional observed command

During one Strong Defrost start sequence, the adaptor sent a later:

```text
CB 03
```

approximately four seconds after `CB 01`/`CB 11`. Its purpose is unresolved and it should not yet be assigned a semantic name.

The older inherited `CB 18 = self-clean running / CB 10 = off` interpretation is superseded by these direct defrost captures for the tested P2 unit; any self-clean mapping must be re-established separately rather than conflated with defrost state.

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

A later full-day capture proved that several fields aggregate exactly into the corresponding current-day `CD` record. For that capture the 24 hourly `CC +8` values summed to `726 Wh`, and the current-day `CD +8` field was exactly `D6 02 00 00 = 726 Wh`.

Two additional additive relationships were observed in the same capture:

```text
sum(CC hourly +0 uint32)  = CD current-day +0 uint32
sum(CC hourly +10 uint32) = CD current-day +12 uint32
```

The physical meanings of those two counters remain unresolved, so they should be treated as confirmed additive accumulators but not named yet.

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

In the aggregation capture, the current-day `CD` record contained `D6 02 00 00 = 726 Wh`, exactly matching the sum of the 24 hourly `CC +8` values.

### `0xCE` — Monthly view

**Total response frame:** 890 bytes.

**Protocol length:** `0x0372`.

**Dataset:** 31 × 28-byte day records = 868 bytes.

Known field:

```text
+8..+11 = electrical consumption, uint32 LE Wh
```

The current-day 28-byte record can be byte-for-byte identical to the corresponding `CD` current-day record, confirming that `CD` and `CE` use the same daily-record schema over different calendar windows.

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

### Per-IDU versus system-wide energy quantities

The Toshiba app exposes both individual-unit and system-wide energy figures. Some unresolved fields/register families may therefore represent one scope or the other. With only one genuine Wi-Fi adaptor currently attached under the sniffer, identical values cannot distinguish per-IDU from system-wide quantities. That distinction should remain unassigned until the second indoor unit is instrumented with its own genuine adaptor and the corresponding datasets can be compared directly.

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

**Total frame length:** 114 bytes.

The 100-byte payload is two consecutive 50-byte equipment records:

```text
byte 12       E0
bytes 13..62  IDU record
bytes 63..112 ODU record
byte 113      checksum
```

Each 50-byte record has the same fixed-width ASCII layout:

```text
+0..20   model field       21 bytes, NUL padded
+21..33  identity field 1  13 bytes, meaning unresolved
+34..42  identity field 2   9 bytes, meaning unresolved
+43..49  identity field 3   7 bytes, meaning unresolved
```

The exact semantics of the three identity fields are deliberately left unresolved. They are fixed equipment/platform identity values, not operating-state data. Blank or literal `NULL` fields must not overwrite a previously known value.

Confirmed captures:

```text
RAS-B13J2FVG-E1
62100295
20855000
012C3E
RAS-5M34G3AVG-E1
62100159
22611900
63E83E
```

```text
NULL
NULL
20854600
012C26
RAS-5M34G3AVG-E1
62100159
22611900
63E83E
```

```text
RAS-B10P2KVSGB-E
62300009
24081500
022020
RAS-5M34G3AVG-E1
62100159
22611900
63E83E
```

The identical four-field ODU record across all three captures, combined with the differing IDU-side fields, confirms the record boundary and field locality. The older B10J2 firmware can publish literal `NULL` for its first two IDU fields while still providing identity fields 2 and 3.

This `NULL` model observation is **not** evidence that the B10J2 family lacks controls such as FIX. In the project package the mandatory declared model remains authoritative for family/protocol routing. The absence of a Toshiba-reported E0 model is used only as a conservative UI-confidence signal: optional FIX entities are not advertised until a usable model is actually reported by E0.

`0xE0` is pushed asynchronously. Ordinary short active reads have timed out on tested J2 units, so implementations should not assume the record can be obtained by polling during initialisation. The Wi-Fi side acknowledges a received `E0` push with the standard class-`0x91` ACK.

## Register `0xE4` — IDU engineering status

**Payload length:** 8 bytes.

**Typical unsolicited total frame:** 22 bytes.

Current layout:

```text
+0 = IDU coil/heat-exchanger-related temperature
+1 = second IDU temperature / junction-related value; exact physical location unresolved
+2 = live IDU fan-speed feedback, approximately RPM / 10 (about 10 rpm per count)
+3..+7 = unresolved
```

`E4 +2` is the live physical fan-speed feedback, not the `0xA0`/`F8 +2` fan-command enum. Across the tested units, the common scale is approximately 10 rpm per register count. The differing numeric ranges between chassis families represent genuinely different blower speeds rather than a family-specific protocol scale.

Examples of the established scale include:

```text
24  -> ~240 rpm
60  -> ~600 rpm
61  -> ~610 rpm
103 -> ~1030 rpm
```

The older Office J2 firmware has also returned `FE` in this field when usable live fan feedback is not available. `FE` must not be interpreted as decimal 254 and passed through the RPM/airflow conversion. The runtime therefore treats `FE` and `FF` as unavailable engineering values and leaves the previous derived airflow/output state untouched. The observed sentinel behaviour is clear; the protocol's formal definition of the sentinel range remains undocumented.

The lower-speed J2FVG console values and the substantially higher P2KVSGB high-wall values are physically consistent with their different blower and air-path geometry. The P2 high-wall fan runs faster; this is not evidence of an alternate E4 encoding.

Representative P2KVSGB `+2` observations include:

```text
00 = fan stopped / early heating startup / armed ON timer but not yet running
43 = live fan feedback during Fan Only test
55 = earlier/restricted Hi POWER heating phase (~550 rpm)
61 = sustained full Hi POWER heating value after roughly 12-14 minutes (~610 rpm)
33 = after target reduction / Hi POWER removal (~330 rpm)
37 = first observed fan activity about five seconds after one ON-timer expiry (~370 rpm)
```

The installed `RAS-B10P2KVSGB-E` has also produced an observed fan-speed envelope extending to `103` (~1030 rpm) in other operating regimes. Airflow conversion remains model-specific because fan diameter, blower geometry and air path determine m³/h for a given RPM.

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

A controlled `RAS-B13J2FVG-E1` Fan-mode sweep independently reproduced the complete fan-command sequence in `F8 +2`: Quiet `31`, levels 1-5 `32..36`, and Auto `41`. This matches the P2KVSG mapping and confirms the four-byte F8 command structure across both directly tested families.

Every observed write receives the generic ACK ending `F8 3A`. The ACK does not echo resulting state.

On shutdown the adaptor can write `80 31` and then reiterate the stored `F8` configuration, so `F8` represents operating configuration, not proof that the unit is presently active.

## Validation labels

- **confirmed** — directly correlated with genuine Toshiba Wi-Fi-adaptor traffic and reproduced.
- **validated / established** — strongly established by repeated component and sniffer observations, even if every enum member was not re-exercised in the latest campaign.
- **partially decoded** — register and some fields are validated, but exact scaling or remaining subfields are unresolved.
- **draft / provisional inference** — observed wire behaviour is real, but the proposed semantic interpretation needs more reproductions before being promoted.
- **not independently validated** — inherited/component interpretation awaiting direct genuine-adaptor confirmation.
- **declared only** — exists in inherited protocol vocabulary without sufficient live validation.
