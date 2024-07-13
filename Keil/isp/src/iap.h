#ifndef __IAP_H__
#define __IAP_H__

void iap_init();
BOOL iap_check_addr(WORD addr);
BYTE iap_read_byte(WORD addr);
BOOL iap_write_byte(WORD addr, BYTE dat);
void iap_erase_page(WORD addr);

#endif
