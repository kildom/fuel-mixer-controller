/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PARAM_SERVICE_H_
#define PARAM_SERVICE_H_

#include <zephyr/bluetooth/uuid.h>

/* Parameter service UUID: dbf0c631-883a-4d52-b455-4a9e5cd94bc3 */
#define BT_UUID_PARAM_SVC_VAL \
	BT_UUID_128_ENCODE(0xdbf0c631, 0x883a, 0x4d52, 0xb455, 0x4a9e5cd94bc3)

/* Write characteristic UUID: dbf0c632-883a-4d52-b455-4a9e5cd94bc3 */
#define BT_UUID_PARAM_WRITE_VAL                                                                    \
	BT_UUID_128_ENCODE(0xdbf0c632, 0x883a, 0x4d52, 0xb455, 0x4a9e5cd94bc3)

/* Read characteristic UUID: dbf0c633-883a-4d52-b455-4a9e5cd94bc3 */
#define BT_UUID_PARAM_READ_VAL                                                                     \
	BT_UUID_128_ENCODE(0xdbf0c633, 0x883a, 0x4d52, 0xb455, 0x4a9e5cd94bc3)

/* Description characteristic UUID: dbf0c634-883a-4d52-b455-4a9e5cd94bc3 */
#define BT_UUID_PARAM_DESC_VAL                                                                     \
	BT_UUID_128_ENCODE(0xdbf0c634, 0x883a, 0x4d52, 0xb455, 0x4a9e5cd94bc3)

#define BT_UUID_PARAM_SVC   BT_UUID_DECLARE_128(BT_UUID_PARAM_SVC_VAL)
#define BT_UUID_PARAM_WRITE BT_UUID_DECLARE_128(BT_UUID_PARAM_WRITE_VAL)
#define BT_UUID_PARAM_READ  BT_UUID_DECLARE_128(BT_UUID_PARAM_READ_VAL)
#define BT_UUID_PARAM_DESC  BT_UUID_DECLARE_128(BT_UUID_PARAM_DESC_VAL)

#endif /* PARAM_SERVICE_H_ */
