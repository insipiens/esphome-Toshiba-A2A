# Provenance and modification notice

This repository contains a modified and substantially extended version of the software originally published as:

**pedobry/esphome_toshiba_suzumi**  
https://github.com/pedobry/esphome_toshiba_suzumi

The original project established the ESPHome Toshiba UART implementation from which this project was derived. Its code, protocol mappings, component architecture and documentation formed the starting point for the work in this repository.

## Original work

Credit for the original implementation belongs to **pedobry and the contributors to `esphome_toshiba_suzumi`**.

The original project's own technical references are retained here as part of that lineage:

- toremick/shorai-esp32 — https://github.com/toremick/shorai-esp32
- Vpowgh/TConnect — https://github.com/Vpowgh/TConnect

Nothing in this repository is intended to imply that those projects or their authors are responsible for, have reviewed, or endorse the later modifications made here.

## Modified work

This independent repository was established in **September 2026** after the derived code had diverged substantially from the original project.

Changes made during that development include protocol investigation and diagnostics, pushed class-0x11 message handling, E0 equipment identification, IDU/ODU model reporting, model/family capability mapping, additional E4/E5 engineering telemetry, cross-unit validation, corrected/conservative telemetry descriptions, passive UART capture, and a redesign of the Home Assistant control model so Toshiba functions are not represented solely as a single `0xF7` preset selector.

The project remains a work in progress. Its direct test population is deliberately documented in the README and must not be interpreted as validation of the wider Toshiba model range supported or discussed by the upstream project.

## Licence

The original project is distributed under the **GNU General Public License, version 3**. This derived project continues under the same licence. See [LICENSE](LICENSE).

The source is intentionally marked as modified so that problems introduced by this project are not attributed to the original author or contributors.
