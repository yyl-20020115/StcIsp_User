/*
注意:
  #ifdef STCISP_USED_CODE
	    ... 
	#endif 
	之间的代码需要被合并到目标程序中
  主函数需要调用sys_init	
*/

#define STCISP_USED_CODE
#ifdef  STCISP_USED_CODE
#include "stc15.h"
#include "intrins.h"

#define DEFAULT_LEADING_SYMBOL 0x7f
#define DEFAULT_LEADING_SIZE   0x40
#define IAP_RESET_CMD 0x20

#ifdef USE_HIGH_FREQUENCY
#define FOSC                    24000000UL
#else
#define FOSC                    11059200UL
#endif
#define BAUD                    (65536 - FOSC/4/115200)

#define DFU_TAG				 0x12abcd34			 //DFU强制执行标志
long xdata DfuFlag _at_ 0x0efc;				 //DFU标志, 定义在xdata的最后4字节

void iap_sys_init()
{
		P0M0 = 0x00;
		P0M1 = 0x00;
		P3M0 = 0x00;
		P3M1 = 0x00;
		
		DfuFlag = 0;

    AUXR &= ~0x01;
    SCON = 0x52;
    
    AUXR |= 0x40;
	
    TMOD &= ~0xf0;
  
    TL1 = BAUD;
    TH1 = BAUD >> 8;
    TR1 = 1;
	  ES=1;
		EA = 1;

}


unsigned char got = 0;
int symbols_count = 0;

void iap_uart_int() interrupt 4
{
	if(TI)
	{
		TI = 0;
	}
	if(RI)
	{
		RI = 0;
		got = SBUF;
		if(got == DEFAULT_LEADING_SYMBOL)
		{
			if (++symbols_count>=DEFAULT_LEADING_SIZE)
			{
					symbols_count = 0;
					DfuFlag = DFU_TAG;
					IAP_CONTR = IAP_RESET_CMD;
			}

		}else{
			symbols_count = 0;
		}
	}
}
#endif

int main()
{
		P0=0x55;
#ifdef  STCISP_USED_CODE
		iap_sys_init();
#endif
	  while (1)
		{
			P0=~P0;
		}
}



