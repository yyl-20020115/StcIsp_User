#include "stc.h"
#include "uart.h"
#include "iap.h"
#include "dfu.h"

DWORD xdata DfuFlag _at_ 0x0efc;

void dfu_check()
{
    if ((DFU_FORCEPIN != 0) &&
        (DfuFlag != DFU_TAG) &&
        (*(BYTE code *)(LDR_SIZE) == 0x02) &&
        (*(WORD code *)(LDR_SIZE + 1) >= LDR_SIZE + 3))
    {
        ((void (code *)())(LDR_SIZE))();
    }
    
    DfuFlag = 0;
}

void dfu_events()
{
    BYTE cmd;
    WORD addr;
    BYTE size;
    BYTE ret;
    BYTE *ptr; 
    BYTE status;

    if (!bUartRxReady)
        return;

    cmd = UartRxBuffer[1];
    addr = *(WORD *)&UartRxBuffer[4];
    size = UartRxBuffer[6];
    ptr = &UartRxBuffer[7];
    status = STATUS_OK;
    ret = 0;

    switch (cmd)
    {
    case DFU_CMD_CONNECT:
        UartTxBuffer[0] = LDR_VERSION >> 8;
        UartTxBuffer[1] = LDR_VERSION;
        ret = 2;
        break;
    case DFU_CMD_READ:
//#ifdef DEBUG
        ret = size;
        ptr = &UartRxBuffer[0];
        while (size--)
        {
            *ptr++ = iap_read_byte(addr++);
        }
//#else
//        status = STATUS_ERRORCMD;
//#endif
        break;
    case DFU_CMD_PROGRAM:
        while (size--)
        {
            if (!iap_write_byte(addr, *ptr))
            {
                status = STATUS_PROGRAMERR;
                break;
            }
            addr++;
            ptr++;
        }
        break;
    case DFU_CMD_ERASE:
        addr = LDR_SIZE;
        while (addr < 0xf400)
        {
            iap_erase_page(addr);
            addr += 0x200;
        }
        break;
    case DFU_CMD_REBOOT:
        IAP_CONTR = 0x20;
        while (1);
        break;
    default:
        status = STATUS_ERRORCMD;
        break;
    }

    uart_send(status, ret);
    uart_recv_done();
}
