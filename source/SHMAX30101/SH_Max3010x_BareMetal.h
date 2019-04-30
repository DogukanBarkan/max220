#ifndef SH_Max3010x_BareMetal_H_
#define SH_Max3010x_BareMetal_H_

#include <stdint.h>
#include <stdio.h>

#include <stdint.h>
#include <stdio.h>
#include "demoDefinitions.h"


#define COMM_SUCCESS        0
#define COMM_GENERAL_ERROR  -1
#define COMM_INVALID_PARAM  -254
#define COMM_NOT_RECOGNIZED -255

#define DISABLE_AGC_USAGE 0
#define ENABLE_AGC_USAGE  1

#define BPT_CALIBRATION_TIMEOUT_SECONDS ((int) 70)
#define BPT_CALIBRATION_PROCESSDONE_VAL ((int) 100)

#define CAL_DATA_SIZE_BYTES ((int) 624)
#define STATUS_OFFSET_BYTE ((int) 1)


enum mfastExecMode{

	mfastEstimationMode     = 1,
	mfastCalibrationMode    = 2
};


/**
* @brief	Initialize Max30101 with default configuration
*
* @param[in]	agc_enabled - check whether to enable agc or not
*
* @return SS_STATUS byte
*/
//int SH_Max3010x_Maximfast_init( const int algoExecutionMode, const int agc_usage , const int accelBehavior);




typedef int (*cmdExecFunc)( const char*); // typedef void (*cmdExecFunc)( const void*);

typedef struct {
	char const* cmdStr;
	cmdExecFunc execute;
	char const *help;
}cmd_interface_tp;


static int command_help(const char *str);





/**
* @brief	sets the register value of ppg sensor (8614x is connected to ME11 SENSOR HUB for this demo but can be used for all sensors physically
*           connected to ME11 SENSOR HUB).
*
* @param[in] addr_value_args : byte string including command followed by register address in hex and register value in hex
*                              "set_reg ppgsensor 0xAB 0xCD" or "set_reg ppgsensor AB CD"
*
* @return 0x00 on success and prints on command console
*/
int SH_Max3010x_set_ppgreg(const char *addr_value_args);

/**
* @brief	gets the register value of ppg sensor (8614x is connected to ME11 SENSOR HUB for this demo but can be used for all sensors physically
*           connected to ME11 SENSOR HUB).
*
* @param[in] addr_arg: byte string including command followed by register address in hex
*                      "set_reg ppgsensor 0xAB" or "set_reg ppgsensor AB"
*
* @return 0x00 on success and prints register value on command console
*/
int SH_Max3010x_get_ppgreg(const char *addr_arg);



/**
* @brief	  eanbles/disables automatic gain control usage
*
* @param[in]  config : byte string including command followed by mode value in HEX uo to 1 hexadecimal digits.
*                         "set_cfg ppg_agc 0/1"  0:off 1: On
*
*@return  status info, 0x00 on success
*
**/
int SH_Max3010x_set_ppg_agcusage(const char *config);

/**
* @brief	  returns automatic gain control usage
*
* @param[in]  null_arg , ie no params. params are discarded
*
*@return  status info, 0x00 on success ,  prints agc usage to console
*
**/
int SH_Max3010x_get_ppg_agcusage(const char *null_arg);


/**
* @brief	   runs unit test on PPG sensor MAX3010x (ie OS24)
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
              prints test result to command console
*
* @return status info, 0x00 on success.
**/
int SH_Max3010x_self_test(const char *null_arg);

/**
* @brief	  demo specific fucntion to disable algorithm and PPG sensor
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type

*
* @return status info, 0x00 on success.
**/

int SH_Max3010x_stop(const char *null_arg);

/**
* @brief	  demo specific fucntion to print description of commands to run Sensor Hub for Blood Pressure Trending capable Sensor Hub
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
              prints command descriptions to command console
*
* @return status info, 0x00 on success.
**/
static int command_help_mfast(const char *str);

/**
* @brief	  initalizes and starts maximFast measurement for estimation data acquisition from ME11 SENSOR HUB using sensor hub accelerometer.
*             it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/
int SH_Max3010x_measure_whrm_estimation_hubaccel  ( const char *null_arg );

/**
* @brief	  initalizes and starts maximFast measurement for estimation data acquisition from ME11 SENSOR HUB using host accelerometer.
*             It default initilizes ) input fifo lenght(to 5 , this is FIFO used by accel data from host). it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/
int SH_Max3010x_measure_whrm_estimation_hostaccel ( const char *null_arg );

/**
* @brief	  initalizes and starts maximFast measurement for estimation data acquisition from ME11 SENSOR HUB using no accelerometer data.
*             it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/
int SH_Max3010x_measure_whrm_estimation_noaccel   ( const char *null_arg );

/**
* @brief	  initalizes and starts maximFast measurement for calibration data acquisition from ME11 SENSOR HUB using host accelerometer.
*             It prints perfusion index during calibration mode. it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/

int SH_Max3010x_measure_whrm_calibration_hubaccel ( const char *null_arg );

/**
* @brief	  initalizes and starts maximFast measurement for estimation data acquisition from ME11 SENSOR HUB using host accelerometer.
*             It prints perfusion index during calibration mode. It default initilizes ) input fifo lenght(to 5 , this is FIFO used by accel data from host)
*             it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/

int SH_Max3010x_measure_whrm_calibration_hostaccel( const char *null_arg );
/**
* @brief	  initalizes and starts maximFast measurement for calibration data acquisition from ME11 SENSOR HUB using no accelerometer data.
*             It prints perfusion index during calibration mode. it sets sensor hub to raw data + algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/
int SH_Max3010x_measure_whrm_calibration_noaccel  ( const char *null_arg );

/**
* @brief	  initalizes and starts maximFast measurement for estimation data acquisition from ME11 SENSOR HUB using sensor hub accelerometer.
*             it sets sensor hub to only algorithm result report mode.
*
* @param[in]  null_arg : NULL string, just to match the form of command table function pointer type
*
** @return 0x00 on success and prints status on command console
*/
int SH_Max3010x_measure_whrm_validation_hubaccel  ( const char *null_arg );


void SH_Max3010x_data_report_execute(void);


#define NUMCMDS3010xMFAST (14)

const cmd_interface_tp CMDTABLE3010xMFAST[] = {

	/* 0*/	{  "enable_whrm estim_hubaccel"    , SH_Max3010x_measure_whrm_estimation_hubaccel	, "start WRHM estimation measurement using sensor hub accelerometer , reports raw PPG data "		  			},
	/* 1*/	{  "enable_whrm estim_hostaccel"   , SH_Max3010x_measure_whrm_estimation_hostaccel  , "start WRHM estimation measurement using sensor host accelerometer, reports raw PPG data "		  			},
	/* 2*/	{  "enable_whrm estim_noaccel" 	   , SH_Max3010x_measure_whrm_estimation_noaccel    , "start WRHM estimation measurement using no accelerometer data, reports raw PPG data "		  				},
    /* 3*/	{  "enable_whrm calib_hubaccel"    , SH_Max3010x_measure_whrm_calibration_hubaccel  , "start WRHM calibration measurement reporting R values, using sensor hub accelerometer, reports raw PPG data" },
	/* 4*/	{  "enable_whrm calib_hostaccel"  , SH_Max3010x_measure_whrm_calibration_hostaccel , "start WRHM calibration measurement reporting R values, using host accelerometer, reports raw PPG data"       },
	/* 5*/	{  "enable_whrm calib_noaccel" 	   , SH_Max3010x_measure_whrm_calibration_noaccel   , "start WRHM calibration measurement reporting R values, using no accelerometer data, reports raw PPG data"    },
	/* 6*/	{  "enable_whrm validation" 	   , SH_Max3010x_measure_whrm_validation_hubaccel   , "start WRHM measurement using sensor hub accelerometer data, no raw PPG data reporting"   					},
	/* 7*/	{  "get_reg ppgsensor"       	   , SH_Max3010x_get_ppgreg                		    , "get register value of 8614x sensor, usage:  get_reg ppgsensor rAddr(1byte)" 									},
	/* 8*/	{  "set_reg ppgsensor"       	   , SH_Max3010x_set_ppgreg                         , "set register value of 8614x sensor, usage :  set_reg ppgsensor rAddr(1byte) rval(1byte)" 					},
	/* 9*/	{  "set_cfg ppg agc"               , SH_Max3010x_set_ppg_agcusage                   , "on/off ppg automatic gain control for bpt algo,  usage: set_cfg ppg_agc X , X: 0 off 1 on"					},
	/* 9*/	{  "get_cfg ppg agc"               , SH_Max3010x_get_ppg_agcusage                   , "get automatic gain control usage setting 0 off 1 on"					                                        },
	/*10*/	{  "self_test ppg os24"            , SH_Max3010x_self_test 				            , " self test max30101 sensor" 																					},
	/*12*/	{  "stop"                          , SH_Max3010x_stop						        , "stops BPT measurement" 																						},
	/*13*/	{  "help"                          , command_help_mfast 						    ,  " " 																											}

};


static int command_help_mfast(const char *str){
	int cmdIdx = 0;

	SERIALOUT(" \r\n AVAILABLE COMMANDS FOR BPT ON 3010X: \r\n\r\n");
	while( cmdIdx != NUMCMDS3010xMFAST ){
		SERIALOUT(" %s : \r\n  %s \r\n\r\n", CMDTABLE3010xMFAST[cmdIdx].cmdStr , CMDTABLE3010xMFAST[cmdIdx].help );
        cmdIdx++;
	};

}


/*DEMO RELATED DATA TO REPORT RESULTS FROM MAIN APPLICATION*/
extern uint8_t mFastMesurementHr   ;
extern uint8_t mFastMesurementSpo2 ;
extern uint8_t mFastMesurementConf ;



#endif
