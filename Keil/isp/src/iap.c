#include "stc.h"
#include "iap.h"

void iap_init()
{
    IAP_CONTR = 0x81;
}

BOOL iap_check_addr(WORD addr)
{
    return (addr >= LDR_SIZE);
}

BYTE iap_read_byte(WORD addr)
{
    return *(BYTE code *)(addr);
}

BOOL iap_write_byte(WORD addr, BYTE dat)
{
    if (!iap_check_addr(addr))
        return 0;

    IAP_CMD = 2;
    IAP_ADDRH = BYTE1(addr);
    IAP_ADDRL = BYTE0(addr);
    IAP_DATA = dat;
    IAP_TRIG = 0x5a;
    IAP_TRIG = 0xa5;
    _nop_();
    _nop_();
    _nop_();
    _nop_();

    return (iap_read_byte(addr) == dat);
}

void iap_erase_page(WORD addr)
{
    if (!iap_check_addr(addr))
        return;

    IAP_CMD = 3;
    IAP_ADDRH = BYTE1(addr);
    IAP_ADDRL = BYTE0(addr);
    IAP_TRIG = 0x5a;
    IAP_TRIG = 0xa5;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

