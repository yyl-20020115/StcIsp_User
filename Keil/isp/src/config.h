#ifndef __CONFIG_H__
#define __CONFIG_H__

#define FOSC                    24000000UL
#define BAUD                    (65536 - FOSC/4/115200)

//#define DEBUG

#define LDR_SIZE                0x1000
#define LDR_VERSION             0x0100



#endif
