/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_timer0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>

#define TIMER_LOAD_ADDR		DT_INST_REG_ADDR_BY_NAME(0, load)
#define TIMER_RELOAD_ADDR	DT_INST_REG_ADDR_BY_NAME(0, reload)
#define TIMER_EN_ADDR		DT_INST_REG_ADDR_BY_NAME(0, en)
#define TIMER_UPDATE_VALUE_ADDR	DT_INST_REG_ADDR_BY_NAME(0, update_value)
#define TIMER_VALUE_ADDR	DT_INST_REG_ADDR_BY_NAME(0, value)
#define TIMER_EV_STATUS_ADDR	DT_INST_REG_ADDR_BY_NAME(0, ev_status)
#define TIMER_EV_PENDING_ADDR	DT_INST_REG_ADDR_BY_NAME(0, ev_pending)
#define TIMER_EV_ENABLE_ADDR	DT_INST_REG_ADDR_BY_NAME(0, ev_enable)

#define TIMER_EV		0x1
#define TIMER_IRQ		DT_INST_IRQN(0)
#define TIMER_DISABLE		0x0
#define TIMER_ENABLE		0x1
#define TIMER_UPDATE_VALUE	0x1

static void litex_timer_irq_handler(const void *device)
{
	int key = irq_lock();

	litex_write8(TIMER_EV, TIMER_EV_PENDING_ADDR);
	sys_clock_announce(1);

	irq_unlock(key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	static struct k_spinlock lock;
	uint32_t timer_value;
	k_spinlock_key_t key = k_spin_lock(&lock);

	litex_write8(TIMER_UPDATE_VALUE, TIMER_UPDATE_VALUE_ADDR);
	timer_value = (uint32_t)litex_read64(TIMER_VALUE_ADDR);

	k_spin_unlock(&lock, key);

	return timer_value;
}

uint64_t sys_clock_cycle_get_64(void)
{
	static struct k_spinlock lock;
	uint64_t timer_value;
	k_spinlock_key_t key = k_spin_lock(&lock);

	litex_write8(TIMER_UPDATE_VALUE, TIMER_UPDATE_VALUE_ADDR);
	timer_value = litex_read64(TIMER_VALUE_ADDR);

	k_spin_unlock(&lock, key);

	return timer_value;
}

/* tickless kernel is not supported */
uint32_t sys_clock_elapsed(void)
{
	return 0;
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	IRQ_CONNECT(TIMER_IRQ, DT_INST_IRQ(0, priority),
			litex_timer_irq_handler, NULL, 0);
	irq_enable(TIMER_IRQ);

	litex_write8(TIMER_DISABLE, TIMER_EN_ADDR);

	litex_write32(k_ticks_to_cyc_floor32(1), TIMER_RELOAD_ADDR);
	litex_write32(k_ticks_to_cyc_floor32(1), TIMER_LOAD_ADDR);

	litex_write8(TIMER_ENABLE, TIMER_EN_ADDR);
	litex_write8(litex_read8(TIMER_EV_PENDING_ADDR), TIMER_EV_PENDING_ADDR);
	litex_write8(TIMER_EV, TIMER_EV_ENABLE_ADDR);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
