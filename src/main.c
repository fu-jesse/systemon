/**
******************************************************************************
* Copyright (c) 2026 Nordic Semiconductor ASA
* Filename : main.c
* Date(DD-MM-YYYY)    Modification    Name
* ----------------    ------------    ----
* 14-10-2026          Created         JF
*******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/linker/linker-defs.h>
#if defined(CONFIG_APP_RAM_POWER_DOWN_UNUSED)
#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
#include <ram_pwrdn.h>
#else
#include <hal/nrf_memconf.h>
#endif
#endif

/* Macro----------------------------------------------------------------------*/
#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define RAM_BASE            DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define RAM_SIZE            DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#define RAM_SECTION_SIZE    KB(32)

/* Private typedef -----------------------------------------------------------*/

/* Private function prototypes------------------------------------------------*/
static void ram_report_and_trim(void);

/* Global variables ----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Macro ---------------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int main(void)
{
    LOG_PRINTK("****************************************\n");
    LOG_PRINTK("system on sample started\n");
    LOG_PRINTK("****************************************\n");

    ram_report_and_trim();

    return 0;
}

/* Override functions -------------------------------------------------------*/

/* Private Functions ---------------------------------------------------------*/

/**
 * @brief Report the RAM used by the image and power down the unused RAM sections.
 *
 * The used RAM is everything the linker placed in RAM (data, bss, noinit,
 * stacks and static heaps). Only sections that lie completely above it and
 * completely inside the application SRAM are powered down (System ON IDLE),
 * using the RAM power down library (power_down_unused_ram) when
 * CONFIG_RAM_POWER_DOWN_LIBRARY is enabled, otherwise directly through nrf_memconf.
 *
 * The trim runs once, at the start of main(). It assumes nothing is placed in
 * RAM above _image_ram_end (e.g. no extra memory-region or retained RAM there);
 * anything above it loses its content.
 */
static void ram_report_and_trim(void)
{
    const uintptr_t used_start = (uintptr_t)_image_ram_start;
    const uintptr_t used_end = (uintptr_t)_image_ram_end;
    const uint32_t total_sections = DIV_ROUND_UP(RAM_SIZE, RAM_SECTION_SIZE);
    const uint32_t first_unused = DIV_ROUND_UP(used_end - RAM_BASE, RAM_SECTION_SIZE);
    uint32_t powered_down = 0;

    LOG_INF("RAM total %u KB (%u x %u KB sections), used %u B (%u KB), free %u KB",
            (uint32_t)(RAM_SIZE / 1024U), total_sections, (uint32_t)(RAM_SECTION_SIZE / 1024U),
            (uint32_t)(used_end - used_start), (uint32_t)DIV_ROUND_UP(used_end - used_start, 1024U),
            (uint32_t)((RAM_BASE + RAM_SIZE - used_end) / 1024U));

#if defined(CONFIG_APP_RAM_POWER_DOWN_UNUSED)
    /* Rounded down on purpose: a trailing section that is only partly inside the
     * application SRAM (nRF54LM20: 511 KB) is not owned by the app and stays powered.
     */
    const uint32_t last_owned = RAM_SIZE / RAM_SECTION_SIZE;

#if defined(CONFIG_RAM_POWER_DOWN_LIBRARY)
    if (first_unused < last_owned)
    {
        powered_down = last_owned - first_unused;
    }

    power_down_unused_ram();
#else
    for (uint32_t i = first_unused; i < last_owned && i < 32U; i++)
    {
        nrf_memconf_ramblock_control_enable_set(NRF_MEMCONF, 0, i, false);
        powered_down++;
    }
#endif
#endif

    LOG_INF("RAM sections powered: %u/%u (sections %u..%u powered down: %u)",
            total_sections - powered_down, total_sections, first_unused,
            (powered_down > 0U) ? (first_unused + powered_down - 1U) : first_unused,
            powered_down);
}

/* Macro ---------------------------------------------------------------------*/

