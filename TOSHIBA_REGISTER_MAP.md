# Toshiba UART register / command map

This document records protocol findings from direct captures on the test system. Status labels distinguish genuine Toshiba Wi-Fi-adaptor validation from inherited/component assumptions and still-unresolved fields.

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

Frames begin:

```text
02 00 03
```

Bytes 5-6 contain the 16-bit big-endian protocol length. Complete frame length is:

```text
frame_length = protocol_length + 8
```

Register position depends on the message class:

```text
0x10  Wi-Fi adaptor request/write     register at byte 12
0x90  IDU response                    register at byte 14
0x11  IDU unsolicited/push            register at byte 12
0x91  Wi-Fi ACK of unsolicited push   no register field
```

Length-field reassembly has been validated against both short 13-22 byte control/status traffic and the large official energy frames:

```text
CC = 502 bytes
CD = 218 bytes
CE = 890 bytes
CF = 358 bytes
```

## `0xA3` louvre / FIX / swing encoding

Ordinary state/readback values:

```text
31 = no swing
41 = vertical swing
42 = horizontal swing
43 = both swing
60 = H.DA
```

The genuine Wi-Fi adaptor uses a different packed write encoding for several louvre operations.

Vertical FIX sweep:

```text
88 89 8A 8B 8C 8D
```

Horizontal FIX sweep:

```text
85 8D 95 9D A5 AD
```

The physical UI exposes five FIX positions on each axis. Six values occurred during each sweep, so one value in each sequence is a transition/default/current-state effect rather than a sixth physical position. Exact one-to-one position mapping remains to be established.

The numerical pattern strongly supports separate packed horizontal and vertical subfields: vertical changes in `+1` steps and horizontal in `+8` steps.

Genuine-adaptor swing writes:

```text
Vertical swing         -> A3=AE
Horizontal swing       -> A3=B6
Vertical + horizontal  -> A3=80
No swing transition    -> A3=80
H.DA                   -> A3=60
```

`AE`, `B6` and `60` are confirmed command values. `80` is repeatable but its exact state semantic remains unresolved. Importantly, the IDU itself also emitted unsolicited class-`0x11` pushes of `A3 80` immediately after power-on, so `80` is not merely a Wi-Fi-adaptor-only command code.

## `0xF8` aggregate operating configuration

Fan Only at 22°C, fan levels 1-5:

```text
F8 45 16 32 00
F8 45 16 33 00
F8 45 16 34 00
F8 45 16 35 00
F8 45 16 36 00
```

Heat captures:

```text
F8 43 18 41 00   Heat, 24°C, Auto, Standard
F8 43 14 41 00   Heat, 20°C, Auto, Standard
F8 43 16 41 00   Heat, 22°C, Auto, Standard
F8 43 16 41 01   Heat, 22°C, Auto, Hi POWER
F8 43 13 41 03   Heat, 19°C, Auto, ECO/special-function value 03
```

A controlled Hi POWER OFF→ON toggle changed only the fourth byte:

```text
F8 43 16 41 00
F8 43 16 41 01
```

The later `03` capture matches the established `F7` value for ECO. The best current interpretation is therefore:

```text
byte 0 = HVAC mode
byte 1 = target temperature, raw °C
byte 2 = commanded fan enum
byte 3 = special-function selector using the F7 enum
```

This supersedes the earlier interpretation of byte 3 as a generic flags byte.

Observed special-function values:

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

Every observed `F8` write is followed by the generic `F8 3A` ACK; the ACK does **not** echo the resulting state. Therefore the command encoding is decoded, but authoritative IDU readback of the active special function is still an open question. A physical-remote test is planned to determine whether the IDU pushes or returns this state via `F7`, `F8`, or another register.

On shutdown the official adaptor writes power OFF separately (`80 31`) and then can reiterate the current `F8` configuration. `F8` therefore represents stored operating configuration rather than proof that the unit is actively running.

## OFF timer (`0x94` / `0x96`)

Genuine adaptor captures:

```text
30 minutes:
96 00 1E
94 41

1 hour:
96 01 00
94 41

clear:
94 42
```

Thus:

```text
0x96 = programmed interval HH MM
0x94 = state/control: 41 active, 42 inactive
```

The programmed interval and enable state are separate. Clearing does not require zeroing `0x96`.

The app also exposes an ON/program timer. `0x90`/`0x92` remain genuinely unconfirmed because that app control has not yet been exercised under the sniffer.

## PURE (`0xC7`)

```text
PURE ON  -> C7 18
PURE OFF -> C7 10
```

Both directions were captured as genuine Toshiba Wi-Fi-adaptor writes.

## Wireless LED (`0xDE`)

```text
LED OFF -> DE 00
LED ON  -> DE 05
```

The official adaptor used `DE` directly in this test. The independent purpose of legacy/component register `DF` remains unresolved.

## Official Energy Monitoring (`0xCC`-`0xCF`)

Opening the Toshiba Energy Monitoring page causes explicit polling in order:

```text
CC
CD
CE
CF
```

### Timestamp

Each dataset begins with:

```text
7E MM DD HH mm ss
```

The month is zero-based. Example:

```text
7E 08 0D 10 05 26 = 13 Sep, 16:05:38
```

### Calendar structure

```text
CC: 480 data bytes = 24 × 20-byte records  -> hourly slots / daily view
CD: 196 data bytes =  7 × 28-byte records  -> daily slots / weekly view
CE: 868 data bytes = 31 × 28-byte records  -> day-of-month / monthly view
CF: 336 data bytes = 12 × 28-byte records  -> monthly slots / yearly view
```

### Electrical-energy field

The electrical-consumption field is at logical record offset `+8`.

For `CD`, `CE` and `CF` it is a little-endian uint32 value in Wh. UI-correlated examples:

```text
CD current day: B5 00 00 00 = 181 Wh  -> app 0.18 kWh
CF September:   2E 08 00 00 = 2094 Wh -> app 2.1 kWh
```

Later simultaneous-period captures showed equal increments in the day and month counters, confirming that they are the same electrical-consumption quantity accumulated over different calendar periods.

For `CC`, the corresponding hourly field is a little-endian 16-bit Wh quantity at the same logical `+8` offset. Example at 16:05:

```text
E6 00 = 230 Wh
```

Several other counters in the 28-byte records change coherently but are not yet assigned physical meanings. They should remain explicitly unresolved rather than being guessed as runtime, output energy, heating/cooling split, or allocation quantities.

The relationship between the official `CC`-`CF` history path and inherited `D8`-`DB` register names remains unresolved. `D8`-`DB` must not be treated as equivalent to the official app datasets without separate validation.

## `0xE4` IDU engineering status

Current working layout:

```text
+0 IDU heat-exchanger / coil-related temperature
+1 second IDU temperature / junction-related value; exact physical location unresolved
+2 live fan/airflow feedback quantity; not commanded fan enum and not literal RPM
+3..+7 unresolved in current captures
```

`+2` is now well established as live fan/airflow feedback. It changes independently of the `A0`/`F8` fan command.

Representative observations include:

```text
E4 +2 = 0x00   fan stopped / early heating startup
E4 +2 = 0x43   live fan feedback during Fan Only test
E4 +2 = 0x55   earlier/restricted Hi POWER heating phase
E4 +2 = 0x61   sustained full Hi POWER value after roughly 12-14 minutes
E4 +2 = 0x33   after target reduction / Hi POWER removal
```

`0x61` is the highest sustained value observed so far on this unit. It should be described as a raw live feedback value until its engineering scaling is independently established.

## `0xE5` ODU/system engineering status

Current working interpretation:

```text
+0 temperature-like engineering quantity
+1 temperature-like engineering quantity
+2 temperature-like engineering quantity
+3 load/allocation-like quantity; zero when demand falls away
+4 unresolved
+5 unresolved
+6 current-like quantity tracking ODU electrical activity
+7 unresolved
```

`+6` correlates strongly with measured electrical current but exact physical scope and scaling remain empirical. The component's correction/scaling must therefore be documented separately from the protocol fact that the raw field tracks ODU electrical activity.

During target reduction before shutdown, both `E5 +3` and `E5 +6` fell to zero while the indoor fan continued to run, supporting the interpretation that these fields are associated with active compressor/load demand rather than merely HVAC mode.

## Validation labels

- **confirmed** — directly correlated with genuine Toshiba Wi-Fi-adaptor traffic and reproduced.
- **validated / established** — mapping is strongly established by repeated component and sniffer observations, even if not every enum member was re-exercised in the latest campaign.
- **partially decoded** — register and some fields are validated, but exact scaling or remaining subfields are unresolved.
- **not yet directly tested** — plausible inherited/structural interpretation awaiting a deliberate genuine-adaptor test.
- **declared only** — exists in the inherited protocol vocabulary without sufficient live validation.
