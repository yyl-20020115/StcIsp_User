#include "stc15.h"
#include "intrins.h"

#define FOSC            24000000UL
#define T1MS            (65536 - FOSC/1000)

#define DFU_TAG         0x12abcd34      //DFU强制执行标志
long xdata DfuFlag _at_ 0x0efc;         //DFU标志, 定义在xdata的最后4字节

void sys_init();

int cnt200;

void tm0_isr() interrupt 1
{
    if (cnt200++ >= 200)
    {
        cnt200 = 0;
        P0 = ~P0;
    }
}

void main()
{
	P32 = 0;
	P32 = 1;
    sys_init();
    
    while (1)
    {
        if (P32 == 0)
        {
            DfuFlag = DFU_TAG;          //当需要执行用户ISP代码时,将强制执行标志赋值到DFU标志变量中
            IAP_CONTR = 0x20;           //然后执行软复位
        }
    }
}

void sys_init()
{
    P0M0 = 0x00;
    P0M1 = 0x00;
    P3M0 = 0x00;
    P3M1 = 0x00;
    
    TMOD &= ~0x0f;
    AUXR |= 0x80;
    TL0 = T1MS;
    TH0 = T1MS >> 8;
    TR0 = 1;
    ET0 = 1;
    
    DfuFlag = 0;                        //上电正常执行用户AP,时需要将DFU标志清零
    cnt200 = 0;
    
    EA = 1;
}

