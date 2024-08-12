#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef USE_HIGH_FREQUENCY
#define FOSC                    24000000UL
#else
#define FOSC                    11059200UL
#endif

#define BAUD                    (65536 - FOSC/4/115200)

//#define DEBUG

#define LDR_SIZE                0x1000
#define LDR_VERSION             0x0100



#endif
