# Passive UART sniffer

The passive sniffer is a development tool for observing traffic between a
genuine Toshiba Wi-Fi adaptor and the indoor unit. It is distinct from the
controller's **Toshiba focused monitor**:

- the passive sniffer listens to both existing wire directions without
  transmitting;
- Focused Monitor records traffic seen by an installed ESPHome controller,
  including that controller's own requests and the IDU replies.

Use the passive sniffer when establishing what the genuine adaptor and remote
application actually send.

## Hardware

The tested sniffer uses an ESP32-C3 SuperMini with two UART receivers:

| Genuine-adaptor test pad | ESP32-C3 | Observed direction |
|---|---|---|
| VCC | `5V` | Power |
| GND | `GND` | Common reference |
| RXD | `GPIO20` RX only | IDU → Wi-Fi adaptor |
| TXD | `GPIO21` RX only | Wi-Fi adaptor → IDU |

The RXD/TXD names above are from the adaptor's perspective. Both ESP32
connections are inputs: the sniffer must not drive either signal line.

Before connecting, measure the idle voltage of both test pads. The example
assumes the labelled pads are on the adaptor's 3.3 V logic side. Never connect
a 5 V logic signal directly to an ESP32 GPIO; use appropriate level shifting if
measurement shows that it is required.

The genuine adaptor draws about 20 mA in the tested installation. The ESP32
adds its own load, so confirm that the available accessory supply is suitable
for the combined arrangement. Do not power the ESP32 separately from USB while
its GPIOs are attached to an unpowered adaptor unless the electrical path has
been deliberately designed for that condition.

## ESPHome configuration

[`examples/passive-uart-sniffer.yaml`](../examples/passive-uart-sniffer.yaml)
configures two RX-only UARTs at Toshiba's observed `9600-8-E-1` format and
labels every logged byte burst by direction. The logger uses the ESP32-C3 USB
serial/JTAG interface so neither monitored GPIO is consumed by logging.

Set the local Wi-Fi/API/OTA secrets, compile the example for the specified
ESP32-C3 board and verify logging before attaching the test-pad wires.

## Capturing an experiment

1. Start a log and allow the adaptor/IDU exchange to settle.
2. Perform exactly one named action from the genuine remote or application.
3. Wait long enough to capture the command, acknowledgement and any delayed
   state publication.
4. Record the wall-clock time, starting state, selected HVAC mode and the exact
   button/action.
5. Repeat the action from the original starting state when checking whether a
   value is deterministic.

Do not infer a command merely because a byte changed near the same time.
Separate repeatable observation from interpretation, and retain the raw traffic
when submitting a finding.

## Useful capture metadata

Include the following with a trace:

- exact IDU and ODU model;
- adaptor model and relevant firmware/app version if known;
- heating/cooling/fan mode, target temperature and fan setting;
- action time and action description;
- which log label represents each physical direction;
- whether other indoor units on the same multi-split system were operating.
