> [!WARNING]
> **AI-GENERATED PROJECT — USE AT YOUR OWN RISK**
>
> This project was created primarily with the assistance of Artificial Intelligence (AI), with limited human involvement in reviewing, validating, and testing the generated code.
>
> The code may contain bugs, security vulnerabilities, incorrect assumptions, or other issues. It has not necessarily undergone sufficient review or testing to be considered production-ready.
>
> **You are solely responsible for reviewing, testing, validating, and securing this project before using it, especially in production or other critical environments.**
>
> Use of this project in production is entirely at your own risk. The maintainers make no guarantees regarding its correctness, reliability, security, or suitability for any particular purpose.

# Fuel Mixer

BLE peripheral for nRF52840 DK (PCA10056) exposing a table of 32-bit parameters over a custom
GATT service. Selected parameters are stored persistently (Zephyr settings on NVS).

## Build and flash

```bash
ncs7 west build -p -b nrf52840dk/nrf52840 -d build
ncs7 west flash -d build --dev-id <serial>
```

Logs are on UART (VCOM0, 115200 baud).

## GATT service

Advertised name: `Fuel Mixer`.

| Item                 | UUID                                   | Properties              |
| -------------------- | -------------------------------------- | ----------------------- |
| Service              | `dbf0c631-883a-4d52-b455-4a9e5cd94bc3` |                         |
| Parameter write      | `dbf0c632-883a-4d52-b455-4a9e5cd94bc3` | Write, Write w/o resp.  |
| Parameter read       | `dbf0c633-883a-4d52-b455-4a9e5cd94bc3` | Read                    |
| Description (JSON)   | `dbf0c634-883a-4d52-b455-4a9e5cd94bc3` | Read, Write             |

The parameter read characteristic returns a 5-byte packet: `[ID (1 byte)][value (int32, signed, little-endian)]`.
The write characteristic accepts a 9-byte packet with the same first 5 bytes, followed by
`[validation value (uint32, little-endian)]`. The validation value must be `0x113cfdf9`;
writes with another length or validation value are rejected. This guards against accidental
writes from incompatible clients, but is not cryptographic authentication.

Each read returns the parameter whose ID is stored in parameter 0, then advances parameter 0 to
the next ID (wrapping to 0 after the last one). Write parameter 0 to select which parameter is
read next.

Write errors: wrong length -> `Invalid Attribute Value Length`, invalid validation value or
unknown ID -> `Value Not Allowed`.

### Description characteristic

Provides the JSON description of all parameters in fragments of up to 200 bytes:

- Write (optional): `[offset (uint16, LE)]` sets the byte offset of the next fragment. The
  offset is reset to 0 on every new connection.
- Read: `[total length (uint16, LE)][fragment offset (uint16, LE)][JSON data]`. Each read
  advances the offset by the fragment length and wraps to 0 after the last fragment.

A client writes offset 0, then reads until it has `total length` bytes, checking that each
fragment offset matches the number of bytes received so far.

JSON format (minified on the device):

```json
{
  "version": 1,
  "device": "Fuel Mixer",
  "parameters": [
    { "id": 0, "name": "Next read index", "hidden": true },
    { "id": 1, "name": "Outputs", "description": "...", "persistent": true }
  ]
}
```

`parameters[i].id` must be `i`; `name` is required. `description` is optional. Flags
`readOnly`, `persistent`, `hidden` and `live` are optional booleans; a missing flag means `false`
(omit false flags to save space). `hidden` marks parameters a GUI should not show (parameter 0).
`live` marks parameters whose value changes on its own (for example inputs), so a GUI should
poll them continuously; other parameters only change when written.

## Parameters

The table is defined in [src/app_params.c](src/app_params.c); IDs are in
[src/app_params.h](src/app_params.h). Their description for clients is in
[src/params_desc.json](src/params_desc.json). The build validates and minifies it with
[scripts/gen_params_desc.py](scripts/gen_params_desc.py) and embeds it into the firmware; the
build fails if the JSON is invalid or the number of described parameters differs from the table.
When adding a parameter, update all three files. Application logic (worker thread with a 100 ms loop
and the parameter read/write callbacks it relies on) is in [src/logic.c](src/logic.c). IO API is in [src/io.h](src/io.h) (named outputs/inputs, see
the GPIO table below). GPIO assignment is in
[boards/nrf52840dk_nrf52840.overlay](boards/nrf52840dk_nrf52840.overlay).

## GPIOs

| Name    | Direction | Pin   | LED (mirror)  |
|---------|-----------|-------|---------------|
| Motor 1 | output    | P1.01 | LED1 (P0.13)  |
| Motor 2 | output    | P1.02 | LED2 (P0.14)  |
| Pump    | output    | P1.03 | LED3 (P0.15)  |
| Heater  | output    | P1.04 | LED4 (P0.16)  |
| Loader  | input, pull-up | P1.07 | -        |
| Sensor  | input, pull-up | P1.08 | -        |
| Button  | input, pull-up | P0.11 | -        |

## Web client

A browser GUI using Web Bluetooth is in [docs/](docs/README.md).
