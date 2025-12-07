#include <stdio.h>
#include <stdint.h>
#include "main.h"

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

uint32_t psp_of_tasks[MAX_TASKS] = {TASK1_STACK_START,
                                   TASK2_STACK_START,
                                   TASK3_STACK_START,
                                   TASK4_STACK_START};

uint32_t task_handlers[MAX_TASKS];

uint8_t current_task = 0; //Task1 is the first task to run


int main(void) 
{
    initialise_monitor_handles(); // Initialize semihosting for printf

    enable_processor_faults(); // Enable faults for debugging

    printf("Starting RTOS Kernel\n");
    // Initialize the scheduler task stack
    init_scheduler_task(SCHED_STACK_START);

    task_handlers[0] = (uint32_t)task1_handler;
    task_handlers[1] = (uint32_t)task2_handler;
    task_handlers[2] = (uint32_t)task3_handler;
    task_handlers[3] = (uint32_t)task4_handler;

    // Initialize task stacks
    init_tasks_stack();

    // Initialize SysTick timer for generating periodic interrupts (before PSP switch)
    init_systick_timer(TICK_HZ);

    // Switch to PSP and start Task 1 (never returns to main)
    switch_sp_to_psp();

    task1_handler();

    // Code below never executes; execution transfers to task1_handler via PSP
    while (1);

    return 0;
}

void task1_handler(void)
{
    // This is Task 1 handler definition
    while (1)
    {
        printf("Task 1 is running\n");
    }
}

void task2_handler(void)
{
    // This is Task 2 handler definition
    while (1)
    {
        printf("Task 2 is running\n");
    }
}

void task3_handler(void)
{
    // This is Task 3 handler definition
    while (1)
    {
        printf("Task 3 is running\n");
    }
}

void task4_handler(void)
{
    // This is Task 4 handler definition
    while (1)
    {
        printf("Task 4 is running\n");
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
    for(uint32_t i = 0; i < MAX_TASKS; ++i)
    {
        uint32_t* pPSP = (uint32_t* )(psp_of_tasks[i]);

        // Hardware-stacked frame (built so first exception return can start the task)
        *(--pPSP) = 0x01000000;       // xPSR with T-bit set (Thumb state)
        *(--pPSP) = task_handlers[i]; // PC
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

        psp_of_tasks[i] = (uint32_t)pPSP; // Update PSP for the task
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
    return psp_of_tasks[current_task];
}

void save_psp_value(uint32_t current_psp_value)
{
    psp_of_tasks[current_task] = current_psp_value;
}

void update_next_task(void)
{
    current_task = (current_task + 1) % MAX_TASKS;
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

/*
 * This is the SysTick interrupt handler 
 */
__attribute__((naked)) void SysTick_Handler(void)
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

