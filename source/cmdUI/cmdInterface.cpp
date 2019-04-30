/*
 * cmdInterface.cpp
 *
 *  Created on: Jan 30, 2019
 *      Author: Yagmur.Gok
 */

#include <mbed.h>
#include "cmdInterface.h"
//#include "SH_Max8614x_BareMetal.h"
#include "SH_Max3010x_BareMetal.h"
#include "bootldrAPI.h"
#include "SHComm.h"
#include "demoDefinitions.h"

static uint8_t hostOperatingMode = HOSTMODEAPPLICATION;
static uint8_t hostCommEchoMode  = HOSTCOMMMODE_ECHO_ON;

/* @brief    function to get the current operating mode of HOST: "APPMODE" or "BOOTLOADERMODE"
 *           associated with command:  "get_host_opmode".
 *
 * @param[in]   arg : NULL string, just to match the form of command table function pointer type
 *
 * */
static int get_host_operating_mode(const char* arg){

	int status = 0x00;
	SERIALOUT("\r\n%s host_operating_mode=%s\r\n", "get_host_mode", (hostOperatingMode ==HOSTMODEAPPLICATION)? "APPMODE":"BOOTLOADERMODE" );
    return status;
}

/* @brief    function to set the current operating mode of HOST: "APPMODE" or "BOOTLOADERMODE"
 *           associated with command:  "set_host_opmode".
 *
 * @param[in]   arg: whole command string with argument included:  "set_host_opmode X", X: 1 or 0
 *                                                                                      0 : APPMODE
 *                                                                                      1 : BOOTLOADERMODE
 *
 * */
static int set_host_operating_mode(const char* arg){

    int status = -1;
	uint32_t val;
	if(  sscanf(arg, "%*s %x", &val) == 1 ){
		hostOperatingMode = ( val > 0 )? HOSTMODEBOOTLOADER:HOSTMODEAPPLICATION;

		status = 0x00;
	}
	SERIALOUT("\r\n%s err=%d\r\n", "set_host_opmode", status);
    return status;

}


/* @brief    function to get the current firmware version of Sensor Hub".
 *
 * @param[in]   arg : NULL string, just to match the form of command table function pointer type
 *
 * */
static int get_hub_firmware_version(const char* arg){

    int idx;
	int status = -1;
    static const int MAXFWDESCSIZE = 5;
	uint8_t descArray[MAXFWDESCSIZE];
    uint8_t descSz;

    status = sh_get_ss_fw_version( &descArray[0] , &descSz);
    if(status == 0x00 && descSz > 0 &&  descSz < MAXFWDESCSIZE){
    	 SERIALOUT("\r\n Firmware Version of Sensor Hub is = ");
         for(idx = 0 ; idx != descSz ; idx++)
        	 SERIALOUT("%d.", descArray[idx]);
         SERIALOUT("\r\n");
    }


}

static int set_host_comm_echomode(const char* arg){

	int status = -1;
	uint32_t val;
	if(  sscanf(arg, "%*s %x", &val) == 1 ){
		hostCommEchoMode = ( val > 0 )? HOSTCOMMMODE_ECHO_ON:HOSTCOMMMODE_ECHO_OFF;

		status = 0x00;
	}
	SERIALOUT("\r\n%s err=%d\r\n", "set_host_comm_echomode", status);
    return status;

}



/*
static int get_hub_operating_mode(const char* arg){

	uint8_t hubMode;
	int status = sh_get_sensorhub_operating_mode(&hubMode);
	if( status == 0x00)
		SERIALOUT("\r\n hub_operating_mode=%s\r\n", (hubMode == 0x00)? "APPMODE":"BOOTLOADERMODE" );
	else
		SERIALOUT("\r\n%s err=%d\r\n", "get_sensorhub_opmode", status);

    return status;
}*/

__attribute__((__always_inline__))
uint8_t get_internal_operating_mode(void) {

	return hostOperatingMode;
}


/* HOST MCU related mode functions inoredr to keep application&bootloader side modular:
 *
 *  1. in app mode Host accepts commnands related to algo/ppg applications ( command table in SH_Max8614x_BareMetal.h)
 *     and does not check/relpy to bootloder related commnads
 *
 *  2. in bootloader mode Host accepts commands related to bootloader ( command table in bootldrAPI.h) only.
 *
 * */


#if defined(SENSOR_8614x)
	cmd_interface_t setCommEchoModeCMD  = {"set_host_echomode"   , set_host_comm_echomode , "enables/disables echoing of command console commands"};
	cmd_interface_t setHostModeCMD      = {"set_host_opmode"     , set_host_operating_mode , "sets mode of host to app or bootloader"};
	cmd_interface_t getHostModeCMD      = {"get_host_opmode"     , get_host_operating_mode , "gets mode of host app or bootloader"};
	cmd_interface_t getHubFwVersionCMD  = {"get_hub_fwversion"   , get_hub_firmware_version , "gets mode of host app or bootloader"};
	//cmd_interface_t getHubModeCMD     =  {"get_sensorhub_opmode", get_hub_operating_mode , "gets mode of host app or bootloader"};
	cmd_interface_t getHubFwVersionCMD  = {"get_hub_fwversion"   , get_hub_firmware_version , "gets mode of host app or bootloader"};
#else

#endif



/* @brief    Compares two string to check whether they match.
 *
 * @param[in]   str1, str2 : strings to compare
 *
 * */

static bool starts_with(const char* str1, const char* str2)
{
	while (*str1 && *str2) {
		if (*str1 != *str2)
			return false;
		str1++;
		str2++;
	}

	if (*str2)
		return false;

	return true;
}



/**
* @brief    Command parser and executer for user input commands
* @details  Gets the command string from command builder, compares with the commands of defined command tables of 8614x and bootloader
*           if found calls the function associated with the command, passsing the whole command string with arguments to the called
*           function
*
* @param[in]    cmd_str Input commnad from user.
*/




int parse_execute_command( const char *cmd_str)
{

	int found = 0;
    int tableIdx;

#if defined(SENSOR_8614x)
    if( starts_with(&cmd_str[0], setCommEchoModeCMD.cmdStr)) {
    	int status = setCommEchoModeCMD.execute(cmd_str);
        if( status != 0x00){
        	SERIALOUT("\r\n%s err=%d\r\n", "set_host_mode", COMM_INVALID_PARAM);
        	hostCommEchoMode  = HOSTCOMMMODE_ECHO_OFF;
        }
        found = 1;
    }

    if( starts_with(&cmd_str[0], setHostModeCMD.cmdStr)) {
    	int status = setHostModeCMD.execute(cmd_str);
        if( status != 0x00){
        	SERIALOUT("\r\n%s err=%d\r\n", "set_host_mode", COMM_INVALID_PARAM);
        	hostOperatingMode = HOSTMODEAPPLICATION;
        }
        found = 1;
    }

    if( starts_with(&cmd_str[0], getHostModeCMD.cmdStr)) {
    	int  status = getHostModeCMD.execute(cmd_str);
    	found = 1;
    }

    if( starts_with(&cmd_str[0], getHubFwVersionCMD.cmdStr)) {
    	int  status = getHubFwVersionCMD.execute(cmd_str);
    	found = 1;
    }

 /*   if( starts_with(&cmd_str[0], getHubModeCMD.cmdStr)) {
    	int  status = getHubModeCMD.execute(cmd_str);
    	found = 1;
    }*/
#elsif(0)

    extern cmd_interface_tp initDemoBptMeasurementCMD;
    if( starts_with(&cmd_str[0], initDemoBptMeasurementCMD.cmdStr)) {
    	int  status = initDemoBptMeasurementCMD.execute(cmd_str);
    	found = 1;
    }

    extern cmd_interface_tp stopDemoBptMeasurementCMD;
    if( starts_with(&cmd_str[0], stopDemoBptMeasurementCMD.cmdStr)) {
    	int  status = stopDemoBptMeasurementCMD.execute(cmd_str);
    	found = 1;
    }

#endif


    if( hostOperatingMode == HOSTMODEAPPLICATION) {

#if (SENSOR_8614x)
    	tableIdx = NUMCMDS8614X;
		do{
			tableIdx -= 1;
			if (starts_with(&cmd_str[0], CMDTABLE8614x[tableIdx].cmdStr)){

				CMDTABLE8614x[tableIdx].execute(cmd_str);
				/*MYG DEBUG8*/// SERIALPRINT("___SELECTED COMMAND IDX IS: %d \r\n", tableIdx);
				SERIALOUT(" \r\n"); // Here is needed due to a bug on mbed serial!
				found = 1;
			}

		}while(tableIdx && found == 0 );
#else

    	tableIdx = NUMCMDS3010xMFAST;
		do{
			tableIdx -= 1;
			if (starts_with(&cmd_str[0], CMDTABLE3010xMFAST[tableIdx].cmdStr)){

				CMDTABLE3010xMFAST[tableIdx].execute(cmd_str);
				/*MYG DEBUG8*///SERIALOUT("___SELECTED COMMAND IDX IS: %d \r\n", tableIdx);
				SERIALOUT(" \r\n"); // Here is needed due to a bug on mbed serial!
				found = 1;
			}

		}while(tableIdx && found == 0 );

#endif


    }

    if( hostOperatingMode == HOSTMODEBOOTLOADER) {


    	tableIdx = NUMCMDSBOOTLDRAPI;
		do{
			tableIdx -= 1;
			if (starts_with(&cmd_str[0], CMDTABLEBOOTLDR[tableIdx].cmdStr)){

				CMDTABLEBOOTLDR[tableIdx].execute(cmd_str);
				/*MYG DEBUG8*/// SERIALPRINT("___SELECTED COMMAND IDX IS: %d \r\n", tableIdx);
				SERIALOUT(" \r\n"); // Here is needed due to a bug on mbed serial!
				found = 1;
			}

		}while(tableIdx && found == 0 );
    }

    return found;
}


/**
* @brief    Command builder from serial i/o device.
* @details  Reads character and builds commands for application.
*
* @param[in]    ch Input character from i/o device.
*/
__attribute__((__always_inline__))
void cmdIntf_build_command(char ch)
{
	static char cmd_str[1536];
    static int cmd_idx = 0;
    int status;

    if(hostCommEchoMode == HOSTCOMMMODE_ECHO_ON)
    	SERIALOUT("%c", ch);

	if (ch == 0x00) {
		return;
	}

	if ((ch == '\n') || (ch == '\r')) {
		if (cmd_idx < 1024)
		cmd_str[cmd_idx++] = '\0';
		status = parse_execute_command(cmd_str);

		//Clear cmd_str
		while (cmd_idx > 0)
			cmd_str[--cmd_idx] = '\0';

	} else if ((ch == 0x08 || ch == 0x7F) && cmd_idx > 0) {
		//Backspace character
		if (cmd_idx > 0)
			cmd_str[--cmd_idx] = '\0';
	} else {

		if (cmd_idx < 1536)
			cmd_str[cmd_idx++] = ch;
	}

}


