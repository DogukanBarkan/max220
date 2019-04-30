/*
 * cmdInterface.h
 *
 *  Created on: Jan 30, 2019
 *      Author: Yagmur.Gok
 */

#ifndef SOURCE_CMDUI_CMDINTERFACE_H_
#define SOURCE_CMDUI_CMDINTERFACE_H_

#define COMM_SUCCESS        0
#define COMM_GENERAL_ERROR  -1
#define COMM_INVALID_PARAM  -254
#define COMM_NOT_RECOGNIZED -255

#define FLASH_ERR_GENERAL   -1
#define FLASH_ERR_CHECKSUM  -2
#define FLASH_ERR_AUTH      -3

enum{
	HOSTMODEAPPLICATION = 0,
	HOSTMODEBOOTLOADER  = 1
};

enum{
	HOSTCOMMMODE_ECHO_OFF = 0,
	HOSTCOMMMODE_ECHO_ON  = 1,
};



void cmdIntf_build_command(char ch);
uint8_t get_internal_operating_mode(void);


#endif /* SOURCE_CMDUI_CMDINTERFACE_H_ */
