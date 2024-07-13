#include "stc.h"
#include "uart.h"
#include "iap.h"
#include "dfu.h"

void sys_init();

void main()
{
    dfu_check();
    
    sys_init();
    uart_init();
    iap_init();
    
    while (1)
    {
        uart_isr();
        dfu_events();
    }
}

void sys_init()
{
}
