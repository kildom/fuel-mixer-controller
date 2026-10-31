/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Custom GATT service for reading and writing the parameter table.
 *
 * Both characteristics use a 5-byte packet: [ID][value as int32 little-endian].
 * - Write characteristic: sets parameter ID to the value.
 * - Read characteristic: returns the parameter selected by parameter 0 and advances
 *   parameter 0 to the next ID (wrapping around after the last one).
 *
 * Description characteristic exposes the JSON description of all parameters (src/params_desc.json)
 * in fragments:
 * - Write: 2 bytes, uint16 little-endian byte offset of the next fragment (optional; the offset
 *   is reset to 0 on each new connection).
 * - Read: [total JSON length uint16 LE][fragment offset uint16 LE][up to DESC_CHUNK_SIZE bytes].
 *   Each read advances the offset by the fragment length, wrapping to 0 after the last fragment.
 */

#include <string.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "app_params.h"
#include "param_service.h"
#include "parameters.h"
#include "params_desc_gen.h"

LOG_MODULE_REGISTER(param_svc, LOG_LEVEL_DBG);

BUILD_ASSERT(PARAMS_DESC_COUNT == PARAM_COUNT,
	     "src/params_desc.json does not describe all parameters of params_table");

/* Minified JSON, generated at build time from src/params_desc.json (not NUL-terminated). */
static const uint8_t params_desc[] = {
#include "params_desc.json.inc"
};

#define DESC_HEADER_SIZE 4
#define DESC_CHUNK_SIZE  200

static uint16_t desc_offset;
static uint8_t desc_packet[DESC_HEADER_SIZE + DESC_CHUNK_SIZE];
static uint16_t desc_packet_len;

/* Last packet returned to the client, reused for long reads (offset > 0). */
static uint8_t read_packet[PARAM_PACKET_SIZE];

static ssize_t param_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			  uint16_t len, uint16_t offset)
{
	if (offset == 0) {
		uint8_t id = (uint8_t)params_get(PARAM_NEXT_READ);
		int32_t value;

		if (params_read(id, &value)) {
			return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		}

		read_packet[0] = id;
		sys_put_le32((uint32_t)value, &read_packet[1]);

		params_set(PARAM_NEXT_READ, (id + 1) % params_count);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, read_packet, sizeof(read_packet));
}

static ssize_t param_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != PARAM_PACKET_SIZE) {
		LOG_WRN("Invalid write length %u", len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (params_write(data[0], (int32_t)sys_get_le32(&data[1]))) {
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	return len;
}

static ssize_t desc_read(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
			 uint16_t len, uint16_t offset)
{
	/* Offset > 0 is a continuation (long read) of the packet prepared for offset 0. */
	if (offset == 0) {
		uint16_t chunk = MIN(sizeof(params_desc) - desc_offset, DESC_CHUNK_SIZE);

		sys_put_le16(sizeof(params_desc), &desc_packet[0]);
		sys_put_le16(desc_offset, &desc_packet[2]);
		memcpy(&desc_packet[DESC_HEADER_SIZE], &params_desc[desc_offset], chunk);
		desc_packet_len = DESC_HEADER_SIZE + chunk;

		LOG_DBG("Description fragment: offset %u, length %u of %u", desc_offset, chunk,
			(unsigned int)sizeof(params_desc));

		desc_offset += chunk;
		if (desc_offset >= sizeof(params_desc)) {
			desc_offset = 0;
		}
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, desc_packet, desc_packet_len);
}

static ssize_t desc_write(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	uint16_t new_offset;

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len != sizeof(uint16_t)) {
		LOG_WRN("Invalid description offset length %u", len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	new_offset = sys_get_le16(buf);
	if (new_offset >= sizeof(params_desc)) {
		LOG_WRN("Invalid description offset %u", new_offset);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	desc_offset = new_offset;

	return len;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (!err) {
		desc_offset = 0;
	}
}

BT_CONN_CB_DEFINE(param_svc_conn_callbacks) = {
	.connected = connected,
};

BT_GATT_SERVICE_DEFINE(param_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_PARAM_SVC),
	BT_GATT_CHARACTERISTIC(BT_UUID_PARAM_WRITE,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, param_write, NULL),
	BT_GATT_CUD("Parameter write", BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(BT_UUID_PARAM_READ, BT_GATT_CHRC_READ, BT_GATT_PERM_READ,
			       param_read, NULL, NULL),
	BT_GATT_CUD("Parameter read", BT_GATT_PERM_READ),
	BT_GATT_CHARACTERISTIC(BT_UUID_PARAM_DESC, BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, desc_read, desc_write, NULL),
	BT_GATT_CUD("Parameter description (JSON)", BT_GATT_PERM_READ),
);
