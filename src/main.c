#include <stdio.h>
#include <stdint.h>

extern void initialise_monitor_handles(void);
uint32_t count = 0;

int main() 
{
    initialise_monitor_handles();

    while (1)
    {
        printf("[%ld]Hello, World!\n", ++count);
        for (uint32_t i = 0; i < 1000000; i++);
    }

    return 0;
}
