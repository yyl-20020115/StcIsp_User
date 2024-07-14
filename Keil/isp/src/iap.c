#include "stc.h"
#include "iap.h"

#define IAP_CMD_IDLE  0
#define IAP_CMD_READ  1
#define IAP_CMD_WRITE 2
#define IAP_CMD_ERASE 3
void iap_init()
{
    IAP_CONTR = 0x81;
}

BOOL iap_check_addr(WORD addr)
{
    return (addr >= LDR_SIZE);
}

BYTE iap_read_byte_directly(WORD addr)
{
    return *(BYTE code *)(addr);
}

BYTE iap_read_byte(WORD addr)
{
	  BYTE dat;

    IAP_CMD =IAP_CMD_READ;
    IAP_ADDRH = BYTE1(addr);
    IAP_ADDRL = BYTE0(addr);
    IAP_TRIG = 0x5a;
    IAP_TRIG = 0xa5;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
		IAP_CMD = IAP_CMD_IDLE;
		IAP_CMD = 0;
		IAP_TRIG = 0; 
		IAP_ADDRH = 0x80;
		IAP_ADDRL = 0;
	
    dat = IAP_DATA;
		//dat = iap_read_byte_directly(addr);
  	return dat;
}

BOOL iap_write_byte(WORD addr, BYTE dat)
{
    if (!iap_check_addr(addr))
        return 0;

    IAP_CMD = IAP_CMD_WRITE;
    IAP_ADDRH = BYTE1(addr);
    IAP_ADDRL = BYTE0(addr);
    IAP_DATA = dat;
    IAP_TRIG = 0x5a;
    IAP_TRIG = 0xa5;

    _nop_();
    _nop_();
    _nop_();
    _nop_();
		IAP_CMD = IAP_CMD_IDLE;
		IAP_CMD = 0;
		IAP_TRIG = 0; 
		IAP_ADDRH = 0x80;
		IAP_ADDRL = 0;

    return (iap_read_byte_directly(addr) == dat);
}

void iap_erase_page(WORD addr)
{
    if (!iap_check_addr(addr))
        return;

    IAP_CMD = IAP_CMD_ERASE;
    IAP_ADDRH = BYTE1(addr);
    IAP_ADDRL = BYTE0(addr);
    IAP_TRIG = 0x5a;
    IAP_TRIG = 0xa5;
    _nop_();
    _nop_();
    _nop_();
    _nop_();
		IAP_CMD = IAP_CMD_IDLE;
		IAP_CMD = 0;
		IAP_TRIG = 0; 
		IAP_ADDRH = 0x80;
		IAP_ADDRL = 0;

}

