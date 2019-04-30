/*
 * demoDefinitions.h
 *
 *  Created on: Feb 13, 2019
 *      Author: Yagmur.Gok
 */

#ifndef SOURCE_DEMODEFINITIONS_H_
#define SOURCE_DEMODEFINITIONS_H_


#include "USBSerial.h"

extern Serial daplink;
extern USBSerial microUSB;
//#define SERIALOUT printf
#define SERIALOUT microUSB.printf
#define SERIALIN microUSB._getc
#define SERIAL_AVAILABLE microUSB.readable

#define CONSOLE_STR_BUF_SZ  ((int)1024)

#define RAW_DATA_ONLY

#endif /* SOURCE_DEMODEFINITIONS_H_ */
