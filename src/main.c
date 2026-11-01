/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE relays: BLE peripheral exposing a table of parameters over a custom GATT service.
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "io.h"
#include "logic.h"
#include "param_service.h"
#include "parameters.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define WORKER_STACK_SIZE 1024
#define WORKER_PRIORITY   5

static void worker_thread(void *p1, void *p2, void *p3);

K_THREAD_DEFINE(worker_tid, WORKER_STACK_SIZE, worker_thread, NULL, NULL, NULL, WORKER_PRIORITY,
		0, SYS_FOREVER_MS);

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_PARAM_SVC_VAL),
};

static void adv_work_handler(struct k_work *work)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));

	if (err) {
		LOG_ERR("Advertising failed to start (%d)", err);
		return;
	}

	LOG_INF("Advertising started");
}

static K_WORK_DEFINE(adv_work, adv_work_handler);

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (err) {
		LOG_WRN("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
		return;
	}

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void recycled(void)
{
	k_work_submit(&adv_work);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.recycled = recycled,
};

static void worker_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_MSEC(1000));

	while (true) {
		procesuj((int)TIME_FROM_MS(k_uptime_get()));
		k_sleep(K_MSEC(TIME_TO_MS(1)));
	}
}

void logic_start(void)
{
	k_thread_start(worker_tid);
}

int main(void)
{
	int err;

	LOG_INF("BLE relays application starting");

	err = io_init();
	if (err) {
		LOG_ERR("GPIO init failed (%d)", err);
		return err;
	}

	err = params_init();
	if (err) {
		LOG_ERR("Parameters init failed (%d)", err);
		return err;
	}

	logic_start();

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (%d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	k_work_submit(&adv_work);

	return 0;
}
