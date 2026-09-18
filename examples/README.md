# Examples

There are deliberately only two examples.

| File | Purpose |
|---|---|
| `toshiba-a2a-template.yaml` | Copy-first universal installation template |
| `passive-uart-sniffer.yaml` | Receive-only dual-channel genuine-adaptor sniffer for protocol work |

For a normal installation, start with
[`toshiba-a2a-template.yaml`](toshiba-a2a-template.yaml). Do not combine the
passive sniffer with a production controller: the sniffer is a separate
development tool intended to observe the genuine Toshiba adaptor without
driving either UART signal.

See [Installation](../docs/INSTALLATION.md),
[Usage and options](../docs/USAGE.md), and
[Passive UART sniffer](../docs/SNIFFER.md).
