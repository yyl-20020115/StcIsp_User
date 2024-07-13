#include "stc.h"
#include "uart.h"


BOOL bUartRxReady;

BYTE UartrecvIndex;
BYTE UartRecvStep;
BYTE UartRecvSum;

BYTE xdata UartTxBuffer[256];
BYTE xdata UartRxBuffer[256];

void uart_init()
{
    AUXR &= ~0x01;
    SCON = 0x52;
    
    AUXR |= 0x40;
    TMOD &= ~0xf0;
    TL1 = BAUD;
    TH1 = BAUD >> 8;
    TR1 = 1;
    
    uart_recv_done();
}


void uart_isr()
{
    BYTE dat;
    
    if (RI)
    {
        RI = 0;
        
        UartRecvSum += (dat = SBUF);
        switch (UartRecvStep)
        {
        case 0:
L_CheckHead:
            UartRecvStep = ((UartRecvSum = dat) == '#');
            break;
        case 1:
            UartRxBuffer[0] = dat;
            UartrecvIndex = 0;
            UartRecvStep++;
            break;
        case 2:
            UartRxBuffer[1 + UartrecvIndex++] = dat;
            if (UartrecvIndex >= UartRxBuffer[0])
                UartRecvStep++;
            break;
        case 3:
            if (dat != '$') goto L_CheckHead;
            UartRecvStep++;
            break;
        case 4:
            if (UartRecvSum != 0) goto L_CheckHead;
            bUartRxReady = 1;
            UartRecvStep++;
            break;
        default:
            break;
        }
    }
}


static BYTE send(BYTE dat)
{
    while (!TI);
    TI = 0;
    SBUF = dat;
    
    return dat;
}

void uart_send(BYTE status, BYTE size)
{
    BYTE sum;
    BYTE i;
    
    sum = send('@');
    sum += send(status);
    sum += send(size);
    if (size)
    {
        for (i = 0; i < size; i++)
        {
            sum += send(UartTxBuffer[i]);
        }
    }
    sum += send('$');
    send(-sum);
    
    while (!TI);
}

void uart_recv_done()
{
    bUartRxReady = 0;
    UartrecvIndex = 0;
    UartRecvStep = 0;
    UartRecvSum = 0;
}


