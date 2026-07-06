# Architecture (Current Refactor State)

This document summarizes the current component layout after moving BACnet object sources into `components/bacnet_objects`.

## High-Level Layout

- `main`
  - Contains `main.c` entrypoint and app-level startup flow.
  - `main.c` remains unchanged in this refactor.
- `components/bacnet_app`
  - Owns BACnet runtime orchestration (`bacnet_app.c`, BACnet/IP, MS/TP, COV).
  - Initializes BACnet stack services and creates BACnet objects.
- `components/bacnet_objects`
  - Owns BACnet object implementations:
    - `analog_value.c/.h`
    - `binary_value.c/.h`
    - `analog_input.c/.h`
    - `binary_input.c/.h`
    - `binary_output.c/.h`

## Wrapper Removal

The legacy wrapper translation units in `components/bacnet_app` were removed:

- `legacy_user_settings.c`
- `legacy_wifi_helper.c`
- `legacy_mstp_rs485.c`

These wrappers previously included sources from `main/`.

## Temporary Source Ownership Note

Per current constraints:

- `User_Settings` is **not moved yet**.
- `board_support` is **not added yet**.
- variant branches are **not created yet**.

To keep behavior unchanged while removing wrappers, `bacnet_app` currently compiles:

- `../../main/User_Settings.c`
- `../../main/wifi_helper.c`
- `../../main/mstp_rs485.c`

This is an intermediate state and can be replaced later by dedicated components when `User_Settings` and board support are split out.
