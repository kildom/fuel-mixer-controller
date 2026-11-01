'use strict';

/*
 * BLE Relays web client (Web Bluetooth).
 *
 * Protocol: the write and read characteristics use a 5-byte packet
 * [ID][int32 little-endian value]. Each read of the read characteristic returns the parameter
 * selected by parameter 0 and advances parameter 0 to the next ID (wrapping around).
 *
 * The description characteristic provides a JSON description of all parameters in fragments:
 * write [uint16 LE offset] to select the start, then each read returns
 * [uint16 LE total length][uint16 LE offset][data] and advances the offset.
 */

const SERVICE_UUID = '8f5b0001-3c2a-4e1d-9b7e-2f4a6c8d1e01';
const WRITE_CHAR_UUID = '8f5b0002-3c2a-4e1d-9b7e-2f4a6c8d1e01';
const READ_CHAR_UUID = '8f5b0003-3c2a-4e1d-9b7e-2f4a6c8d1e01';
const DESC_CHAR_UUID = '8f5b0004-3c2a-4e1d-9b7e-2f4a6c8d1e01';

const PARAM_NEXT_READ = 0;
const MAX_PARAMS = 256;
/* Boolean flags of a parameter description; a missing flag means false. */
const DESC_FLAGS = ['readOnly', 'persistent', 'hidden', 'live'];
const DESC_HEADER_SIZE = 4;
const SCAN_DELAY_MS = 1000;
const MIN_VALUE = -2147483648;
const MAX_VALUE = 2147483647;

const ui = {
	connectBtn: document.getElementById('connect-btn'),
	disconnectBtn: document.getElementById('disconnect-btn'),
	refreshBtn: document.getElementById('refresh-btn'),
	status: document.getElementById('status'),
	scanIndicator: document.getElementById('scan-indicator'),
	scanText: document.getElementById('scan-text'),
	title: document.getElementById('title'),
	error: document.getElementById('error'),
	table: document.getElementById('params-table'),
	body: document.getElementById('params-body'),
	hint: document.getElementById('hint'),
};

let device = null;
let readChar = null;
let writeChar = null;
let descChar = null;
/*
 * Parameter descriptions read from the device; index is the parameter ID.
 * Fields: name, description, readOnly, persistent, hidden (not shown and not scanned),
 * live (re-read on every scan; other parameters are read on connect, write and refresh).
 * Missing flags are normalized to false.
 */
let parameters = [];
let connected = false;
let scanGeneration = 0;
/* Visible rows by parameter ID. */
let rows = new Map();

/*
 * All GATT traffic after connecting is done by the scan loop, so operations never overlap.
 * Writes requested by the user are queued here and handled by the loop with priority.
 */
let pendingWrites = [];
let wakeScanLoop = null;
/* Set by the Refresh button: the scan loop reads all visible parameters in the next pass. */
let refreshRequested = false;

function parseValue(text) {
	const s = text.trim();
	if (!/^[+-]?\d+$/.test(s)) {
		return null;
	}
	const value = Number(s);
	return value >= MIN_VALUE && value <= MAX_VALUE ? value : null;
}

function showError(message) {
	ui.error.textContent = message;
	ui.error.hidden = !message;
}

function paramInfo(id) {
	return parameters[id] || { name: `Parameter ${id}`, description: 'Unknown parameter', readOnly: true };
}

/* Sleep that ends early when wakeScanLoop() is called. */
function interruptibleSleep(ms) {
	return new Promise((resolve) => {
		const done = () => {
			clearTimeout(timer);
			wakeScanLoop = null;
			resolve();
		};
		const timer = setTimeout(done, ms);
		wakeScanLoop = done;
	});
}

/* ---------- GATT protocol ---------- */

async function gattWrite(id, value) {
	const packet = new DataView(new ArrayBuffer(5));
	packet.setUint8(0, id);
	packet.setInt32(1, value, true);
	await writeChar.writeValueWithResponse(packet.buffer);
}

async function gattRead() {
	const data = await readChar.readValue();
	if (data.byteLength !== 5) {
		throw new Error(`Unexpected read length ${data.byteLength}`);
	}
	return { id: data.getUint8(0), value: data.getInt32(1, true) };
}

/* Read the fragmented JSON description of all parameters and validate it. */
async function readDescription() {
	const start = new DataView(new ArrayBuffer(2));
	start.setUint16(0, 0, true);
	await descChar.writeValueWithResponse(start.buffer);

	let bytes = new Uint8Array(0);
	let total = null;

	while (total === null || bytes.length < total) {
		const data = await descChar.readValue();
		if (data.byteLength < DESC_HEADER_SIZE) {
			throw new Error(`Description fragment too short (${data.byteLength} bytes)`);
		}

		const fragTotal = data.getUint16(0, true);
		const fragOffset = data.getUint16(2, true);
		const fragLength = data.byteLength - DESC_HEADER_SIZE;

		if (total === null) {
			total = fragTotal;
		}
		if (fragTotal !== total || fragOffset !== bytes.length ||
		    (fragLength === 0 && bytes.length < total)) {
			throw new Error(`Unexpected description fragment (offset ${fragOffset}, ` +
					`length ${fragLength}, total ${fragTotal})`);
		}

		const merged = new Uint8Array(bytes.length + fragLength);
		merged.set(bytes);
		merged.set(new Uint8Array(data.buffer, data.byteOffset + DESC_HEADER_SIZE, fragLength),
			   bytes.length);
		bytes = merged;
	}

	const desc = JSON.parse(new TextDecoder().decode(bytes));
	const list = desc && desc.parameters;

	if (!Array.isArray(list) || list.length === 0 || list.length > MAX_PARAMS) {
		throw new Error('Invalid description: "parameters" must be a non-empty array');
	}
	desc.parameters = list.map((p, index) => {
		if (!p || p.id !== index || typeof p.name !== 'string') {
			throw new Error(`Invalid description of parameter at index ${index}`);
		}
		const flags = {};
		for (const flag of DESC_FLAGS) {
			flags[flag] = p[flag] === true;
		}
		return { ...p, description: p.description ?? '', ...flags };
	});

	return desc;
}

/* ---------- Table ---------- */

function buildTable() {
	ui.body.replaceChildren();
	rows = new Map();

	for (let id = 0; id < parameters.length; id++) {
		const info = paramInfo(id);
		if (info.hidden) {
			continue;
		}

		const tr = document.createElement('tr');

		const idCell = document.createElement('td');
		idCell.textContent = id;

		const nameCell = document.createElement('td');
		nameCell.className = 'name';
		nameCell.textContent = info.name;
		for (const [flag, label] of [['readOnly', 'read-only'], ['persistent', 'persistent']]) {
			if (info[flag]) {
				const badge = document.createElement('span');
				badge.className = 'badge';
				badge.textContent = label;
				nameCell.append(badge);
			}
		}
		if (info.live) {
			const badge = document.createElement('span');
			badge.className = 'badge live';
			badge.textContent = 'live';
			nameCell.append(badge);
		}
		const desc = document.createElement('small');
		desc.textContent = info.description || '';
		nameCell.append(desc);

		const valueCell = document.createElement('td');
		valueCell.className = 'value';
		valueCell.textContent = '—';

		const input = document.createElement('input');
		input.className = 'value-input';
		input.type = 'text';
		input.inputMode = 'numeric';
		input.readOnly = info.readOnly;
		const inputCell = document.createElement('td');
		inputCell.append(input);

		const writeBtn = document.createElement('button');
		writeBtn.type = 'button';
		writeBtn.textContent = 'Write';
		writeBtn.disabled = info.readOnly;
		const btnCell = document.createElement('td');
		btnCell.append(writeBtn);

		tr.append(idCell, nameCell, valueCell, inputCell, btnCell);
		ui.body.append(tr);

		const row = { id, info, valueCell, input, writeBtn, value: null, edited: false };
		rows.set(id, row);

		if (!info.readOnly) {
			input.addEventListener('input', () => {
				row.edited = true;
				input.classList.add('edited');
				input.classList.toggle('invalid', parseValue(input.value) === null);
			});
			input.addEventListener('keydown', (event) => {
				if (event.key === 'Enter') {
					writeRow(row);
				} else if (event.key === 'Escape') {
					resetInput(row);
				}
			});
			writeBtn.addEventListener('click', () => writeRow(row));
		}
	}

	ui.table.hidden = false;
	ui.hint.hidden = true;
}

function resetInput(row) {
	row.edited = false;
	row.input.classList.remove('edited', 'invalid');
	if (row.value !== null) {
		row.input.value = String(row.value);
	}
}

function updateRow(id, value) {
	const row = rows.get(id);
	if (!row) {
		return;
	}

	if (row.value !== value) {
		row.valueCell.textContent = String(value);
		if (row.value !== null) {
			row.valueCell.classList.add('changed');
			setTimeout(() => row.valueCell.classList.remove('changed'), 50);
		}
		row.value = value;
	}

	/* Do not overwrite a value the user is editing. */
	if (!row.edited) {
		row.input.value = String(value);
	}
}

async function writeRow(row) {
	const value = parseValue(row.input.value);
	if (value === null) {
		row.input.classList.add('invalid');
		showError(`Invalid value for "${row.info.name}". Use a decimal integer ${MIN_VALUE}..${MAX_VALUE}.`);
		return;
	}

	row.writeBtn.disabled = true;
	try {
		await new Promise((resolve, reject) => {
			pendingWrites.push({ row, value, resolve, reject });
			if (wakeScanLoop) {
				wakeScanLoop();
			}
		});
		showError('');
	} catch (err) {
		showError(`Write of "${row.info.name}" failed: ${err.message}`);
	} finally {
		row.writeBtn.disabled = row.info.readOnly || !connected;
	}
}

function rejectPendingWrites(reason) {
	const writes = pendingWrites;
	pendingWrites = [];
	for (const w of writes) {
		w.reject(new Error(reason));
	}
}

/* ---------- Scanning ---------- */

function setScanning(active) {
	ui.scanIndicator.classList.toggle('active', active);
}

async function scanLoop(generation, count) {
	const alive = () => connected && generation === scanGeneration;
	const allIds = [...rows.keys()];
	const liveIds = allIds.filter((id) => rows.get(id).info.live);
	/* Device read pointer (parameter 0) if known, so it is rewritten only when needed. */
	let devicePtr = null;
	/* The first pass after connecting reads all parameters, next passes only live ones. */
	let scanIds = allIds;
	let scanPos = 0;

	/* Read one parameter, moving the device read pointer first if needed. */
	const readParam = async (id) => {
		if (devicePtr !== id) {
			devicePtr = null;
			await gattWrite(PARAM_NEXT_READ, id);
		}
		const result = await gattRead();
		devicePtr = (result.id + 1) % count;
		updateRow(result.id, result.value);
		return result;
	};

	/* Handle user writes: write the value, then immediately read it back. */
	const processWrites = async () => {
		while (pendingWrites.length && alive()) {
			const w = pendingWrites.shift();
			setScanning(true);
			try {
				devicePtr = w.row.id === PARAM_NEXT_READ ? null : devicePtr;
				await gattWrite(w.row.id, w.value);
				resetInput(w.row);
				await readParam(w.row.id);
				w.resolve();
			} catch (err) {
				devicePtr = null;
				w.reject(err);
			}
		}
	};

	ui.scanIndicator.hidden = false;

	while (alive()) {
		await processWrites();

		if (refreshRequested) {
			/* Restart the pass with all parameters, even if a live scan is in progress. */
			refreshRequested = false;
			scanIds = allIds;
			scanPos = 0;
		}

		if (scanPos < scanIds.length) {
			setScanning(true);
			try {
				await readParam(scanIds[scanPos]);
				scanPos++;
			} catch (err) {
				devicePtr = null;
				if (alive()) {
					showError(`Scan failed: ${err.message}`);
				}
				scanPos = scanIds.length;
			}
			continue;
		}

		const time = new Date().toLocaleTimeString();
		ui.scanText.textContent = scanIds === allIds
			? `All parameters read at ${time}`
			: `Live scan (${liveIds.length}) at ${time}`;
		setScanning(false);
		ui.refreshBtn.disabled = false;
		scanIds = liveIds;

		/* Pause between scans; user writes are still handled immediately. */
		const deadline = Date.now() + SCAN_DELAY_MS;
		while (alive() && Date.now() < deadline && !refreshRequested) {
			await interruptibleSleep(deadline - Date.now());
			await processWrites();
			setScanning(false);
		}
		scanPos = 0;
	}

	setScanning(false);
	ui.scanIndicator.hidden = true;
}

/* ---------- Connection ---------- */

function setConnectedUi(isConnected) {
	connected = isConnected;
	ui.connectBtn.hidden = isConnected;
	ui.connectBtn.disabled = false;
	ui.disconnectBtn.hidden = !isConnected;
	ui.refreshBtn.hidden = !isConnected;
	/* Enabled again when the initial full read finishes. */
	ui.refreshBtn.disabled = true;
	ui.status.classList.toggle('connected', isConnected);
	ui.status.textContent = isConnected ? `Connected to ${device.name || device.id}` : 'Disconnected';

	for (const row of rows.values()) {
		row.writeBtn.disabled = row.info.readOnly || !isConnected;
	}
}

function onDisconnected() {
	scanGeneration++;
	refreshRequested = false;
	rejectPendingWrites('Disconnected');
	if (wakeScanLoop) {
		wakeScanLoop();
	}
	readChar = null;
	writeChar = null;
	descChar = null;
	setConnectedUi(false);
	setScanning(false);
	ui.scanIndicator.hidden = true;
}

async function connect() {
	showError('');

	if (!window.isSecureContext) {
		showError(`Web Bluetooth requires a secure context, but "${location.origin}" is not one. ` +
			'Open the page as http://localhost:<port>/ or http://127.0.0.1:<port>/ ' +
			'(not http://0.0.0.0 or a LAN IP address), or serve it over https://.');
		return;
	}

	if (!navigator.bluetooth) {
		showError('Web Bluetooth is not available in this browser. Use Chrome or Edge. ' +
			'On Linux it is disabled by default: open ' +
			'chrome://flags/#enable-experimental-web-platform-features, set it to "Enabled" ' +
			'and restart the browser.');
		return;
	}

	if (navigator.bluetooth.getAvailability && !(await navigator.bluetooth.getAvailability())) {
		showError('No Bluetooth adapter is available. Check that Bluetooth is turned on ' +
			'(for example "rfkill unblock bluetooth" and "bluetoothctl power on").');
		return;
	}

	ui.connectBtn.disabled = true;
	ui.status.textContent = 'Connecting…';

	try {
		device = await navigator.bluetooth.requestDevice({
			filters: [{ services: [SERVICE_UUID] }],
		});
		device.removeEventListener('gattserverdisconnected', onDisconnected);
		device.addEventListener('gattserverdisconnected', onDisconnected);

		const server = await device.gatt.connect();
		const service = await server.getPrimaryService(SERVICE_UUID);
		readChar = await service.getCharacteristic(READ_CHAR_UUID);
		writeChar = await service.getCharacteristic(WRITE_CHAR_UUID);
		descChar = await service.getCharacteristic(DESC_CHAR_UUID);

		ui.status.textContent = 'Reading parameter description…';
		const desc = await readDescription();
		parameters = desc.parameters;
		ui.title.textContent = desc.device || device.name || 'BLE device';
		document.title = ui.title.textContent;

		buildTable();
		setConnectedUi(true);

		scanLoop(++scanGeneration, parameters.length);
	} catch (err) {
		if (device && device.gatt.connected) {
			device.gatt.disconnect();
		}
		ui.connectBtn.disabled = false;
		ui.status.textContent = 'Disconnected';
		if (err.name !== 'NotFoundError') {
			showError(`Connection failed: ${err.message}`);
		}
	}
}

function refresh() {
	if (!connected) {
		return;
	}
	ui.refreshBtn.disabled = true;
	refreshRequested = true;
	if (wakeScanLoop) {
		wakeScanLoop();
	}
}

function disconnect() {
	if (device && device.gatt.connected) {
		device.gatt.disconnect();
	} else {
		onDisconnected();
	}
}

ui.connectBtn.addEventListener('click', connect);
ui.disconnectBtn.addEventListener('click', disconnect);
ui.refreshBtn.addEventListener('click', refresh);
