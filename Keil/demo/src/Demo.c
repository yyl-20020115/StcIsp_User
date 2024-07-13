#include "stc15.h"
#include "intrins.h"

#define FOSC            24000000UL
#define T1MS            (65536 - FOSC/1000)

#define DFU_TAG         0x12abcd34      //DFUǿ��ִ�б�־
long xdata DfuFlag _at_ 0x0efc;         //DFU��־, ������xdata�����4�ֽ�

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
            DfuFlag = DFU_TAG;          //����Ҫִ���û�ISP����ʱ,��ǿ��ִ�б�־��ֵ��DFU��־������
            IAP_CONTR = 0x20;           //Ȼ��ִ����λ
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
    
    DfuFlag = 0;                        //�ϵ�����ִ���û�AP,ʱ��Ҫ��DFU��־����
    cnt200 = 0;
    
    EA = 1;
}

