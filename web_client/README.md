# BLE Relays web client

Browser GUI for the BLE Relays parameter service, using the Web Bluetooth API.
No build step or dependencies; plain HTML/CSS/JS.

## Running

Web Bluetooth needs a Chromium-based browser (Chrome, Edge) and a secure context
(`https://` or `http://localhost`). Serve this directory locally:

```bash
cd _my/ble_relays/web_client
python3 -m http.server 8000 --bind 127.0.0.1
```

Then open <http://localhost:8000/>. Do not use the `http://0.0.0.0:8000/` link printed by
`http.server`: `0.0.0.0` (and LAN IP addresses) are not secure contexts, so the browser hides
Web Bluetooth.

On Linux, Web Bluetooth is disabled by default in Chrome. Open
`chrome://flags/#enable-experimental-web-platform-features`, set it to **Enabled** and restart
the browser. The host Bluetooth adapter must also be on (`rfkill unblock bluetooth`,
`bluetoothctl power on`).

## Usage

- **Connect** opens the browser device chooser, filtered to devices advertising the parameter
  service UUID, so only compatible devices are listed.
- After connecting, the client reads the JSON parameter description from the device and builds
  the table from it. **Disconnect** closes the connection.
- After connecting, all parameters are read once. Then only parameters marked `live` are
  re-read continuously, with a 1 s pause between scans. The pulsing green dot and the time of
  the last scan show that scanning is active; changed values flash briefly.
- **Refresh** reads all parameters again (for example after another client changed them). It
  interrupts a live scan in progress and is disabled until the full read finishes.
- Enter a signed decimal integer (-2147483648..2147483647) and click **Write** (or press Enter, Escape reverts).
  An edited field is not overwritten by scanning until it is written or reverted.
- A write is sent right away, even in the middle of a scan or during the pause between scans,
  and the parameter is read back immediately afterwards. An interrupted scan then continues.
- Read-only parameters show read-only fields and a disabled **Write** button. Badges mark
  read-only, persistent and live parameters.

## Parameter description

The client has no built-in knowledge of the parameters. Names, descriptions and the `readOnly`,
`persistent`, `hidden` and `live` flags come from the device description characteristic (see the
[application README](../README.md#description-characteristic)), defined in firmware in
[../src/params_desc.json](../src/params_desc.json). The page title is taken from its `device`
field.

Parameters marked `hidden` are not shown or scanned. Parameter 0 (next read index) is hidden,
because the client uses it internally to select which parameter is read.
