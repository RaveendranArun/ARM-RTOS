#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "led.h"

extern void initialise_monitor_handles(void);

void task1_handler(void); // This is Task 1 handler prototype
void task2_handler(void); // This is Task 2 handler prototype
void task3_handler(void); // This is Task 3 handler prototype
void task4_handler(void); // This is Task 4 handler prototype

void init_scheduler_task(uint32_t sched_top_of_stack);
void init_tasks_stack(void);
void init_systick_timer(uint32_t tick_hz);
void enable_processor_faults(void);
void switch_sp_to_psp(void);
uint32_t get_psp_value(void);
void save_psp_value(uint32_t current_psp_value);
void update_next_task(void);
void task_delay(uint32_t tick_count);

uint8_t current_task = 1;       // Index of the current running task
uint32_t global_tick_count = 0; // Global tick count for the system

typedef struct
{
    uint32_t psp_value;
    uint32_t block_count;
    uint8_t current_state;
    void (*task_handler)(void);
} TCB_t;

TCB_t user_tasks[MAX_TASKS];

int main(void) 
{
    initialise_monitor_handles(); // Initialize semihosting for printf

    enable_processor_faults(); // Enable faults for debugging

    printf("Starting RTOS Kernel\n");
    // Initialize the scheduler task stack
    init_scheduler_task(SCHED_STACK_START);

    // Initialize task stacks
    init_tasks_stack();

    led_init_all(); // Initialize all LEDs;

    // Initialize SysTick timer for generating periodic interrupts (before PSP switch)
    init_systick_timer(TICK_HZ);

    // Switch to PSP and start Task 1 (never returns to main)
    switch_sp_to_psp();

    task1_handler();

    // Code below never executes; execution transfers to task1_handler via PSP
    while (1);

    return 0;
}

void idle_task()
{
    while (1)
    {
        // Idle task does nothing, just loops
    }
}

void task1_handler(void)
{
    // This is Task 1 handler definition
    while (1)
    {
        printf("Task 1 is running\n");
        led_on(LED_GREEN);
        task_delay(1000);
        led_off(LED_GREEN);
        task_delay(1000);
    }
}

void task2_handler(void)
{
    // This is Task 2 handler definition
    while (1)
    {
        printf("Task 2 is running\n");
        led_on(LED_ORANGE);
        task_delay(500);
        led_off(LED_ORANGE);
        task_delay(500);
    }
}

void task3_handler(void)
{
    // This is Task 3 handler definition
    while (1)
    {
        printf("Task 3 is running\n");
        led_on(LED_BLUE);
        task_delay(250);
        led_off(LED_BLUE);
        task_delay(250);
    }
}

void task4_handler(void)
{
    // This is Task 4 handler definition
    while (1)
    {
        printf("Task 4 is running\n");
        led_on(LED_RED);
        task_delay(125);
        led_off(LED_RED);
        task_delay(125);
    }
}

/*
 * This function configure the SysTick timer to generate periodic interrupts
 * at the specified tick_hz frequency.
 */
void init_systick_timer(uint32_t tick_hz)
{
    uint32_t* pSRVR = (uint32_t* )0xE000E014; // SysTick Reload Value Register
    uint32_t* pSCSR = (uint32_t* )0xE000E010; // SysTick Control and Status Register

    uint32_t count_value = (SYSTICK_TIM_CLK / tick_hz);
    
    // Clear the current value of the SRVR
    *pSRVR &= ~(0xFFFFFFFF);

    // Load the count value into the SRVR
    *pSRVR = count_value - 1;

    // Configure and start the SysTick timer
    *pSCSR |= (1 << 1); // Enable SysTick exception request
    
    // Set the clock source to processor clock, enable SysTick exception and start the timer
    *pSCSR |= (1 << 2); // SysTick timer clock source as processor clock
    *pSCSR |= (1 << 0); // Enable the SysTick timer
}

/*
 * This fucntion initializes the MSP with the top of the scheduler stack
 * and then returns to the caller. The main function will continue executing
 * with MSP set to the scheduler stack. This is a naked function to avoid
 * any prologue/epilogue code generation by the compiler. 
 */
__attribute__((naked)) void init_scheduler_task(uint32_t sched_top_of_stack)
{
    __asm volatile ("MSR MSP, %0" : : "r" (sched_top_of_stack) : ); // Set MSP to scheduler stack top
    __asm volatile ("BX LR");       // Return from function call, BX will copy the value of LR to PC
}



void init_tasks_stack(void)
{
    user_tasks[0].current_state = TASK_READY_STATE;
    user_tasks[1].current_state = TASK_READY_STATE;
    user_tasks[2].current_state = TASK_READY_STATE;
    user_tasks[3].current_state = TASK_READY_STATE;
    user_tasks[4].current_state = TASK_READY_STATE;

    user_tasks[0].psp_value = IDLE_STACK_START;
    user_tasks[1].psp_value = TASK1_STACK_START;
    user_tasks[2].psp_value = TASK2_STACK_START;
    user_tasks[3].psp_value = TASK3_STACK_START;
    user_tasks[4].psp_value = TASK4_STACK_START;

    user_tasks[0].task_handler = idle_task;
    user_tasks[1].task_handler = task1_handler;
    user_tasks[2].task_handler = task2_handler;
    user_tasks[3].task_handler = task3_handler;
    user_tasks[4].task_handler = task4_handler;

    for(uint32_t i = 0; i < MAX_TASKS; ++i)
    {
        uint32_t* pPSP = (uint32_t* )(user_tasks[i].psp_value);

        // Hardware-stacked frame (built so first exception return can start the task)
        *(--pPSP) = 0x01000000;       // xPSR with T-bit set (Thumb state)
        *(--pPSP) = (uint32_t)(user_tasks[i].task_handler); // PC
        *(--pPSP) = 0xFFFFFFFD;       // LR: return to Thread mode, PSP
        *(--pPSP) = 0;                // R12
        *(--pPSP) = 0;                // R3
        *(--pPSP) = 0;                // R2
        *(--pPSP) = 0;                // R1
        *(--pPSP) = 0;                // R0

        // Software-saved registers R4-R11
        for (int j = 0; j < 8; ++j)
        {
            *(--pPSP) = 0;
        }

        user_tasks[i].psp_value = (uint32_t)pPSP; // Update PSP for the task
    }
}

void enable_processor_faults(void)
{
    uint32_t* pSHCSR = (uint32_t* )0xE000ED24; // System Handler Control and State Register

    // Enable MemManage, BusFault and UsageFault exceptions
    *pSHCSR |= (1 << 16); // MemManage fault enable
    *pSHCSR |= (1 << 17); // BusFault enable
    *pSHCSR |= (1 << 18); // UsageFault enable
}

uint32_t get_psp_value(void)
{
    return user_tasks[current_task].psp_value;
}

void save_psp_value(uint32_t current_psp_value)
{
    user_tasks[current_task].psp_value = current_psp_value;
}

void update_next_task(void)
{
    int state = TASK_BLOCKED_STATE;

    for (uint32_t i = 0; i <MAX_TASKS; ++i)
    {
        current_task++;
        current_task %= MAX_TASKS;
        state = user_tasks[current_task].current_state;
        if ((state == TASK_READY_STATE) && (current_task != 0))
        {
            break;
        }
    }
    if (state != TASK_READY_STATE)
    {
        current_task = 0; // Switch to idle task if no other task is ready
    }

}

__attribute__((naked)) void switch_sp_to_psp(void)
{
    //1. initialize the PSP with TASK1 stack start address

	//get the value of psp of current_task
	__asm volatile ("PUSH {LR}"); //preserve LR which connects back to main()
	__asm volatile ("BL get_psp_value");
	__asm volatile ("MSR PSP,R0"); //initialize psp
	__asm volatile ("POP {LR}");  //pops back LR value

	//2. change SP to PSP using CONTROL register
	__asm volatile ("MOV R0,#0X02");
	__asm volatile ("MSR CONTROL,R0");
	__asm volatile ("BX LR");
}

void schedule(void)
{
    // Trigger PendSV exception for context switching
    uint32_t* pICSR = (uint32_t* )0xE000ED04; // Interrupt Control and State Register
    *pICSR |= (1 << 28); // Set PendSV set-pending bit
}

void task_delay(uint32_t tick_count)
{
    //disable interrupts
    INTERRUPT_DISABLE();

    if (current_task == 0)
    {
        return; // Idle task should not be delayed
    }

    user_tasks[current_task].block_count = global_tick_count + tick_count;
    user_tasks[current_task].current_state = TASK_BLOCKED_STATE;
    schedule();

    //enable interrupts
    INTERRUPT_ENABLE();

}

__attribute__((naked)) void PendSV_Handler(void)
{
    /*Save the context of current task */

	//1. Get current running task's PSP value
	__asm volatile("MRS R0,PSP");
	//2. Using that PSP value store SF2( R4 to R11)
	__asm volatile("STMDB R0!,{R4-R11}");

	__asm volatile("PUSH {LR}");

	//3. Save the current value of PSP
    __asm volatile("BL save_psp_value");

	/*Retrieve the context of next task */

	//1. Decide next task to run
    __asm volatile("BL update_next_task");

	//2. get its past PSP value
	__asm volatile ("BL get_psp_value");

	//3. Using that PSP value retrieve SF2(R4 to R11)
	__asm volatile ("LDMIA R0!,{R4-R11}");

	//4. update PSP and exit
	__asm volatile("MSR PSP,R0");

	__asm volatile("POP {LR}");

	__asm volatile("BX LR");
}

void update_global_tick_count(void)
{
    global_tick_count++;
}

void unblock_tasks(void)
{
    for (int i = 1; i < MAX_TASKS; ++i)
    {
        if (user_tasks[i].current_state == TASK_BLOCKED_STATE)
        {
            if (user_tasks[i].block_count == global_tick_count)
            {
                user_tasks[i].current_state = TASK_READY_STATE;
            }
        }
    }
}

/*
 * This is the SysTick interrupt handler 
 */
void SysTick_Handler(void)
{
    uint32_t* pICSR = (uint32_t* )0xE000ED04; // Interrupt Control and State Register

    update_global_tick_count();
    unblock_tasks();    
    // Trigger PendSV exception for context switching
    *pICSR |= (1 << 28); // Set PendSV set-pending bit
}

void HardFault_Handler(void)
{
    // This is the HardFault exception handler
    printf("HardFault Exception occurred\n");
    while (1);
}

void MemManage_Handler(void)
{
    // This is the MemManage exception handler
    printf("MemManage Fault Exception occurred\n");
    while (1);
}

void BusFault_Handler(void)
{
    // This is the BusFault exception handler
    printf("BusFault Exception occurred\n");
    while (1);
}

void UsageFault_Handler(void)
{
    // This is the UsageFault exception handler
    printf("UsageFault Exception occurred\n");
    while (1);
}

