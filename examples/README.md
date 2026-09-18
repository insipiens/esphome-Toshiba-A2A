# Examples

Start with [`toshiba-a2a-template.yaml`](toshiba-a2a-template.yaml). It is the
normal installation example and includes the universal package from `main`.

| File | Purpose |
|---|---|
| `toshiba-a2a-template.yaml` | Copy-first universal installation template |
| `kitchen-package-template.yaml` | P2 package compile/reference fixture with a multi-network example |
| `office-j2-package-template.yaml` | J2 compile/reference fixture demonstrating `disable_features` |
| `engineering_telemetry.yaml` | Standalone research configuration exposing engineering fields |
| `output_estimation.yaml` | Standalone development fixture for the output estimator |
| `diagnostic_capture.yaml` | Active-controller Focused Monitor example |
| `passive-uart-sniffer.yaml` | Receive-only, dual-channel genuine-adaptor sniffer |

The Kitchen and Office files contain installation-shaped network and web-server
configuration for CI/reference purposes. They are not the recommended starting
point for a new node. Diagnostic and research examples likewise should not be
combined casually with a production controller.

See [Installation](../docs/INSTALLATION.md),
[Usage and options](../docs/USAGE.md), and
[Passive UART sniffer](../docs/SNIFFER.md).
