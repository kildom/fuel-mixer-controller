/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PARAM_SERVICE_H_
#define PARAM_SERVICE_H_

#include <zephyr/bluetooth/uuid.h>

/* Parameter service UUID: 8f5b0001-3c2a-4e1d-9b7e-2f4a6c8d1e01 */
#define BT_UUID_PARAM_SVC_VAL BT_UUID_128_ENCODE(0x8f5b0001, 0x3c2a, 0x4e1d, 0x9b7e, 0x2f4a6c8d1e01)

/* Write characteristic UUID: 8f5b0002-3c2a-4e1d-9b7e-2f4a6c8d1e01 */
#define BT_UUID_PARAM_WRITE_VAL                                                                    \
	BT_UUID_128_ENCODE(0x8f5b0002, 0x3c2a, 0x4e1d, 0x9b7e, 0x2f4a6c8d1e01)

/* Read characteristic UUID: 8f5b0003-3c2a-4e1d-9b7e-2f4a6c8d1e01 */
#define BT_UUID_PARAM_READ_VAL                                                                     \
	BT_UUID_128_ENCODE(0x8f5b0003, 0x3c2a, 0x4e1d, 0x9b7e, 0x2f4a6c8d1e01)

/* Description characteristic UUID: 8f5b0004-3c2a-4e1d-9b7e-2f4a6c8d1e01 */
#define BT_UUID_PARAM_DESC_VAL                                                                     \
	BT_UUID_128_ENCODE(0x8f5b0004, 0x3c2a, 0x4e1d, 0x9b7e, 0x2f4a6c8d1e01)

#define BT_UUID_PARAM_SVC   BT_UUID_DECLARE_128(BT_UUID_PARAM_SVC_VAL)
#define BT_UUID_PARAM_WRITE BT_UUID_DECLARE_128(BT_UUID_PARAM_WRITE_VAL)
#define BT_UUID_PARAM_READ  BT_UUID_DECLARE_128(BT_UUID_PARAM_READ_VAL)
#define BT_UUID_PARAM_DESC  BT_UUID_DECLARE_128(BT_UUID_PARAM_DESC_VAL)

#endif /* PARAM_SERVICE_H_ */
