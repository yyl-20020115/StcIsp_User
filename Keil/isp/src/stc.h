#ifndef __STC_H__
#define __STC_H__

#include <intrins.h>
#include <stdio.h>
#include <string.h>

#include "stc15.h"
#include "config.h"

typedef bit BOOL;
typedef unsigned char BYTE;
typedef unsigned int WORD;
typedef unsigned long DWORD;

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned int ushort;
typedef unsigned long ulong;

typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;

#define BYTE0(d)                ((BYTE)(d))
#define BYTE1(d)                ((BYTE)((d) >> 8))
#define BYTE2(d)                ((BYTE)((d) >> 16))
#define BYTE3(d)                ((BYTE)((d) >> 24))

#define UNUSED(expr)            if (expr)

#endif
