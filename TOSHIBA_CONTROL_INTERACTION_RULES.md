# Toshiba control interaction rules

This document defines the shared user-control interaction policy for Toshiba residential IDUs in this project.

It is deliberately separate from `TOSHIBA_CONTROL_MATRIX.md` and `TOSHIBA_REGISTER_MAP.md`:

- `TOSHIBA_CONTROL_MATRIX.md` defines which controls are available for each family and operating mode.
- `TOSHIBA_REGISTER_MAP.md` defines UART transport/register evidence.
- this document defines how user controls interact with one another once they are available.

The interaction rules below are treated as common Toshiba behaviour across the supported families. Family/model tables remain responsible only for capability, mode availability and family-specific command encoding.

## Control domains

| Domain | Controls | Interaction model |
| --- | --- | --- |
| Power Select | 100% / 75% / 50% | independent of ordinary special-function selection |
| Special function | Standard, ECO, Hi POWER, Comfort Sleep, Fireplace 1, Fireplace 2, 8 °C Heat, Floor, and other model-supported special modes | mutually exclusive: only one special function may be active at a time |
| Silent operation | Silent 1 / Silent 2 | overrides the active special function and Power Select |
| Air treatment | PURE | independent of Power Select, Silent and the special-function group |

## Shared selection rules

1. Selecting an ordinary special function replaces the currently active special function.
2. Selecting another Power Select level changes only Power Select and does not cancel the currently active ordinary special function.
3. Selecting Silent 1 or Silent 2 overrides both the current Power Select state and the active special function.
4. PURE is independent and remains unchanged by Power Select, special-function or Silent changes.
5. The IDU-reported state is authoritative. Home Assistant entities are views onto the underlying Toshiba state and must not represent impossible combinations.

## Interaction matrix

| New user selection | Power Select | Active special function | Silent | PURE |
| --- | --- | --- | --- | --- |
| Power Select 100/75/50 | change to selected level | unchanged | unchanged unless Toshiba reports otherwise | unchanged |
| ECO | unchanged | replace with ECO | not active | unchanged |
| Hi POWER | unchanged | replace with Hi POWER | not active | unchanged |
| Comfort Sleep | unchanged | replace with Comfort Sleep | not active | unchanged |
| Fireplace 1/2 | unchanged | replace with selected Fireplace level | not active | unchanged |
| 8 °C Heat | unchanged | replace with 8 °C Heat | not active | unchanged |
| Floor | unchanged | replace with Floor | not active | unchanged |
| Standard | unchanged | clear active special function | not active | unchanged |
| Silent 1/2 | overridden/reset according to Toshiba state | clear/replace active special function | set selected Silent level | unchanged |
| PURE on/off | unchanged | unchanged | unchanged | change only PURE |

## Mode availability and effective operating mode

Interaction policy and availability are separate concerns. A control must first be valid for the unit's current operating context before the interaction rules above are applied.

For functions tied specifically to heating, validity is based on effective operation rather than only on the selected HVAC mode:

```text
effective_heating =
    selected HVAC mode is HEAT
    OR
    selected HVAC mode is AUTO and current climate action is HEATING
```

Therefore a heat-only function may be selected in explicit Heat mode, or while Auto is actually heating. It is not valid merely because Auto is selected while the unit is idle, cooling or otherwise not heating.

The family/mode capability table remains the source of truth for which controls are heat-only, cool-capable, fan-capable, etc. This document does not duplicate those family-specific availability lists.

## Home Assistant implementation rule

Separate Home Assistant switches/selects may be retained for usability, but they must not be implemented as independent Toshiba states when the underlying control is mutually exclusive.

The implementation should maintain one authoritative special-function state and project that state onto the individual UI entities. When Toshiba reports a different state, all affected entities must be updated together.

Likewise, invalid selections must be rejected without optimistically leaving an entity ON when the IDU cannot actually enter that state.

## Architectural separation

The intended control stack is:

```text
Shared interaction policy
        ↓
Family capability profile
        ↓
Mode / effective-mode availability
        ↓
Family-specific UART encoding and readback
```

This keeps common Toshiba behaviour in one place while allowing J2FVG, P2KVSG and future families to retain their own capability and transport mappings.
