/*
 * Host-side stubs for the small subset of Zephyr used by logic.c.
 */

#ifndef TEST_MOCK_ZEPHYR_KERNEL_H_
#define TEST_MOCK_ZEPHYR_KERNEL_H_

#define irq_lock() 0
#define irq_unlock(key) ((void)(key))

#endif
