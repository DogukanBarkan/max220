/*
 * bootldrInterface.h
 *
 *  Created on: Feb 7, 2019
 *      Author: Yagmur.Gok
 */

#ifndef SOURCE_CMDUI_BOOTLDRINTERFACE_H_
#define SOURCE_CMDUI_BOOTLDRINTERFACE_H_

typedef int (*cmdExecFunc)( const char*); // typedef void (*cmdExecFunc)( const void*);

typedef struct {
	char const* cmdStr;
	cmdExecFunc execute;
	char const *help;
}cmd_interface_tb;

#define FLASH_ERR_GENERAL   -1
#define FLASH_ERR_CHECKSUM  -2
#define FLASH_ERR_AUTH      -3

/**
* @brief	  sets the SENSOR HUB to Bootloader Mode for Firmware update. Prints status info on command console
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_enter_blmode(const char *null_arg);

/**
* @brief	  exits the SENSOR HUB from Bootloader Mode to Application Mode. Prints status info on command console
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_exit_blmode(const char *null_arg);

/**
* @brief	  gets the page size of bootloader within SENSOR HUB. Prints status info and page size value on command console
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_get_pagesz(const char *null_arg);

/**
* @brief	  sets the totatl page count for MSBL file to be uploaded to SENSOR HUB. Prints status info on command console
*
* @param[in]  arg : byte string including command followed by page size value extracted from header of MSBL file in DECIMAL format!
*                   "num_pages 24"
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_set_pagecount(const char *arg);

/**
* @brief	  sets the IV vector of  MSBL file to be uploaded to SENSOR HUB. Prints status info on command console
*             IV vector is 22 bytes of data extracted from header of MSBL file.
*
* @param[in]  arg : byte string including command followed by 22 byte IV value in HEXADECIMAL format! do no preceed IV bytes wirh 0x !!!!
                    "set_iv 1234567891234567891234"
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_set_iv(const char *arg);

/**
* @brief	  sets the Authentication vector of  MSBL file to be uploaded to SENSOR HUB. Prints status info on command console
*             Authentication vector is 36 bytes of data extracted from header of MSBL file.
*
* @param[in]  arg : byte string including command followed by 22 byte IV value in HEXADECIMAL format! do no preceed IV bytes wirh 0x !!!!
                    "set_auth 12345678912345678912345678912345"
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_set_authentication(const char *arg);

/**
* @brief	  erases application code of SENSOR HUB. Prints status info on command console
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_eraseflash(const char *null_arg);

/**
* @brief	  puts the SENSOR HUB to the state of waiting for MSBL application code pages from serial command interface.
*             Prints status info on command console upon flashing of every page.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_flash(const char *null_arg);

/**
* @brief	  sets the SENSOR HUB to the bootloader state where application image pages are first stored into HOST ram and will be flashed
*             at once. Prints status info on command console.
*
* @param[in]  arg : byte string including command followed by 1 byte omage on ram flag in DECIMAL format
*                   "image_on_ram 0/1"  0: for classic mode where pages are downloaded form PC over serial command console and
*                    flashed to SENSOR HUB 1 by 1. 1: for image on ram mode.
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLOADER_image_on_ram( const char *arg );

/**
* @brief	  flashes pages in HOST Ram to Sensor Hub.
*             USE ONLY IN IMAGE_ON_RAM MODE !!!!
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_flash_appimage_from_ram(const char *null_arg);

/**
* @brief	  set the delay factor multipler for Bootloader wait durations in commands and between flashing of pages
*
* @param[in]  arg : byte string including command followed by delay factor in DECIMAL format
*                   "set_cfg host cdf 1" to "set_cfg host cdf 4" practical range. 1 is default.
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_set_host_bootcmds_delay_factor( const char *arg);

/**
* @brief	  get the delay factor multipler for Bootloader wait durations in commands and between flashing of pages.
*             Prints delay factor multipler value on command console.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_get_host_bootcmds_delay_factor( const char *null_arg);

/**
* @brief	  sets the resetting method of SENSOR HUB between command based and GPIO based resets. Default is GPIO based reset.ie 1.
*
* @param[in]  arg : byte string including command followed by ebl mode in DECIMAL format
*                   "set_cfg host ebl 1/0" . 0 for command based reset; 1 for GPIO based reset which is default and preferred option.
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_set_host_ebl_mode(const char *arg);

/**
* @brief	  gets the resetting method of SENSOR HUB between command based and GPIO based resets.
*             Prints delay factor multipler value on command console.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int SH_BOOTLDR_get_host_ebl_mode(const char *null_arg);

/**
* @brief	  gets the struct keeping state information about bootloading steps required at HOST side.
*             Prints struct data fields and state flags; ie. is_iv_set? page_size acquired from hub etc.
*             If all steps are not done, flashing operation do not take place and informs user on command
*             console
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
* @return status info, 0x00 on success.
**/
int BOOTLDR_get_host_bootloader_state(const char *null_arg);


#define NUMCMDSBOOTLDRAPI (15)

const cmd_interface_tb CMDTABLEBOOTLDR[] = {

		{  "bootldr"     	  , SH_BOOTLDR_enter_blmode	      			  , "resets and puts sensor hub to bootloader mode "	   			 												},
		{  "exit"             , SH_BOOTLDR_exit_blmode        			  , "exits sensor hub from bootloader mode to app mode" 			  							  					},
		{  "page_size"        , SH_BOOTLDR_get_pagesz         			  , "returns sensor hub bootloader page size for app data pages"   							      					},
		{  "num_pages"        , SH_BOOTLDR_set_pagecount      			  , "sets sensor hub bootloader app image pages, uasge: num_pages PAGES" 				    						},
		{  "set_iv"           , SH_BOOTLDR_set_iv             			  , "sets sensor hub bootloader initial vector bytes, usage: set_iv XXXXXXXXXXX (11 hex chrs)"      				},
		{  "set_auth"         , SH_BOOTLDR_set_authentication 			  , "sets sensor hub bootloader authentication bytes, usage: set_iv XXXXXXXXXXXXXXXX (16 hex chrs)" 				},
		{  "erase"            , SH_BOOTLDR_eraseflash         			  , "erases sesn hub application flash memory" 							        				  					},
		{  "image_on_ram"     , SH_BOOTLOADER_image_on_ram    			  , "selects pagBypage download-flash / block download-flash options"           									},
		{  "flash"            , SH_BOOTLDR_flash              	    	  ,  "flash image to hub/dowload pages from PC based on image_on_ram selection" 									},
		{  "image_flash"      , SH_BOOTLDR_flash_appimage_from_ram        , "flashes app image in ram to sensor hub, call after flash cmd in image_on_ram mode"								},
		{  "set_cfg host cdf" , SH_BOOTLDR_set_host_bootcmds_delay_factor , "sets delay factor for bootoader cmd waits default 1, usage: set_cfg host cdf FACTOR"       					},
		{  "set_cfg host ebl" , SH_BOOTLDR_set_host_ebl_mode              , "sets GPIO/CMD reset for reset hub to bootoader mode. default GPIO, usage: set_cfg host ebl 1/0,  1 for GPIO"  	},
		{  "get_cfg host cdf" , SH_BOOTLDR_get_host_bootcmds_delay_factor , "sets delay factor for bootoader cmd waits default 1, usage: set_cfg host cdf FACTOR"       					},
		{  "get_cfg host ebl" , SH_BOOTLDR_get_host_ebl_mode              , "sets GPIO/CMD reset for reset hub to bootoader mode. default GPIO, usage: set_cfg host ebl 1/0,  1 for GPIO"  	},
		{  "get_host_boot_state_info" , BOOTLDR_get_host_bootloader_state , "gets boot state keeping struct of host"  	                                                                    },


};

#endif /* SOURCE_CMDUI_BOOTLDRINTERFACE_H_ */
