#include <stdio.h>
#include <stdint.h>

extern void initialise_monitor_handles(void);

int main() 
{
    // For debugging purposes
    initialise_monitor_handles();

    while (1);

    return 0;
}
