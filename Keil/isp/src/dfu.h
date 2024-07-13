#ifndef __DFU_H__
#define __DFU_H__

#define DFU_FORCEPIN            P33

#define DFU_TAG       	        0x12abcd34

#define DFU_CMD_CONNECT         0xa0
#define DFU_CMD_READ            0xa1
#define DFU_CMD_PROGRAM         0xa2
#define DFU_CMD_ERASE           0xa3
#define DFU_CMD_REBOOT          0xa4

#define STATUS_OK               0x00
#define STATUS_ERRORCMD         0x01
#define STATUS_OUTOFRANGE       0x02
#define STATUS_PROGRAMERR       0x03
#define STATUS_ERRORWRAP        0xff

void dfu_check();
void dfu_events();

extern DWORD xdata DfuFlag;
extern char *USER_STCISPCMD;

#endif

