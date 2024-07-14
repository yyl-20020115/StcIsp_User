#include "stc15.h"
#include "intrins.h"

#define T1MS										(65536 - FOSC/1000)
#define FOSC                    24000000UL
#define BAUD                    (65536 - FOSC/4/115200)


#define DFU_TAG				 0x12abcd34			//DFUǿ��ִ�б�־
long xdata DfuFlag _at_ 0x0efc;				 //DFU��־, ������xdata�����4�ֽ�

#define DEFAULT_LEADING_SYMBOL 0x7f
#define DEFAULT_LEADING_SIZE   0x40


void Delay100us()       //@24.000MHz
{
    unsigned char i, j;

    i = 3;
    j = 82;
    do
    {
        while (--j);
    } while (--i);
}


void sys_init();

unsigned char got = 0;
int symbols_count = 0;

void uart_int() interrupt 4
{
	if(TI){
		TI = 0;
	}
	if(RI){
		RI = 0;
		got = SBUF;
		if(got == DEFAULT_LEADING_SYMBOL)
		{
			symbols_count++;
		}else{
			symbols_count = 0;
		}
	}
}

void main()
{
	
		sys_init();
	  while (1)
		{
				Delay100us();

				if (symbols_count>=DEFAULT_LEADING_SIZE)
				{
  					symbols_count = 0;
						DfuFlag = DFU_TAG;					//����Ҫִ���û�ISP����ʱ,��ǿ��ִ�б�־��ֵ��DFU��־������
						IAP_CONTR = 0x20;					 //Ȼ��ִ����λ
				}
		}
}



void sys_init()
{
		P0M0 = 0x00;
		P0M1 = 0x00;
		P3M0 = 0x00;
		P3M1 = 0x00;
		
		DfuFlag = 0;												//�ϵ�����ִ���û�AP,ʱ��Ҫ��DFU��־����

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

