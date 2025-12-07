#ifndef __MAIN_H__
#define __MAIN_H__

/* STACK memory calculations */

#define SIZE_TASK_STACK      1024U
#define SIZE_SCHED_STACK     1024U

#define SRAM_START           0x20000000U
#define SRAM_SIZE            (128U * 1024U) // 128 KB
#define SRAM_END             ((SRAM_START) + (SRAM_SIZE))

#define TASK1_STACK_START    SRAM_END
#define TASK2_STACK_START    ((SRAM_END) - (SIZE_TASK_STACK))
#define TASK3_STACK_START    ((SRAM_END) - (2 * (SIZE_TASK_STACK)))
#define TASK4_STACK_START    ((SRAM_END) - (3 * (SIZE_TASK_STACK)))
#define SCHED_STACK_START    ((SRAM_END) - (4 * (SIZE_TASK_STACK)))

#define TICK_HZ              1000U

#define HSI_CLK              16000000U  // 16 MHz
#define SYSTICK_TIM_CLK      HSI_CLK    // Assuming AHB clock is 16 MHz

#define MAX_TASKS       4U
#define DUMMY_XPSR      0x00100000U  // Default value for xPSR register

#endif // __MAIN_H__