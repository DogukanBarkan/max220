/*******************************************************************************
 * Copyright (C) 2018 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *******************************************************************************
 */


/**********************************************************************************
 *
 *  Desc: Example Code to get algorithm estimation results of Heart rate and Blood Oxygen percentage from sensor hub and display on Command Console
 *        Example starts by an estimation measurement with agc enabled and using sensor hub accelerometer  and also accepts application mode commands.
 *
 *        Exampe has two modes, Application mode which is the default mode and Bootloader mode.
 *
 *        Example accepts commands described in CMDTABLE3010xMFAST while in Application mode and CMDTABLEBOOTLDR commands in bootloader mode.
 *
 *        Example,
 *
 *        1. In Deafult Application Mode
 *                   Executes CMDTABLE3010xMFAST Command Table commands from user.
 *
 *        2. Example starts by
 *                   1. Sets underlying HW for SensorHub communication
 *                   2. Starts HR+SPo2 estimation measurement
 *                   	        1. initialize algorithm config struct enabled
 *                              2. enable data type to both raw sensor and algorithm data
 *                              3. set fifo threshold for mfio event frequency to 15
 *                              5. enable ppg sensor to acquire ppg data
 *                              5. enable sensor hub accel sensor to acquire accel data
 *                              6. enables AGC algorithm
 *                              7. enable maximFasr algorithm in estimation mode
 *                  3. Example calls SH_Max3010x_data_report_execute() which
 *				                1. calls SH API's sh_ss_execute_once() function which:
 *                                 writes sensor hub's report fifo content (sensor/algorithm data samples) to a buffer(1).
 *                              2. Parses buffer(1) data to extract numeric sensor and algorithm samples according to enabled algorithms.
 * 			                      look:  whrm1/2_data_rx() , max3010x_data_rx(),  accel_data_rx() and sample structs defined within SH_Max3010x_BareMetal.cpp
 *
 *        3. In Booloader Mode
 *                 Executes Bootloader Command Table commands from user. Look at Bootloader Python Script for PC side example.
 *
 *
 ***********************************************************************************/


#include <events/mbed_events.h>
#include <mbed.h>
//#include "Adafruit_SSD1306.h"
#include "max32630fthr.h"
#include "SHComm.h"
#include "SH_Max3010x_BareMetal.h"
#include "cmdInterface.h"
#include "demoDefinitions.h"


#define WAIT_SENSORHUB_STABLE_BOOTUP_MS  ((uint32_t)2000)



int hostMode = HOSTMODEAPPLICATION;


extern uint8_t mFastMesurementHr   ;
extern uint8_t mFastMesurementSpo2 ;
extern uint8_t mFastMesurementConf ;




DigitalOut debugled(LED1, 1);
// Hardware serial port over DAPLink
Serial daplink(USBTX, USBRX, 115200);

#include "USBSerial.h"
USBSerial microUSB(0x1f00, 0x2012, 0x0001, false);


// FEATHER Board initialization
MAX32630FTHR pegasus(MAX32630FTHR::VIO_1V8);



// OLED Screen initialization
//I2C *screenI2C = get_i2c_port();

//I2C ssI2C(P3_4, P3_5);


static bool is_example_measurement_active = false;




int main() {




	int i = 0;
	uint16_t resultToDisplay;
	uint8_t  confidenceToDisplay;

	wait_ms(WAIT_SENSORHUB_STABLE_BOOTUP_MS);
	sh_init_hwcomm_interface();
	sh_disable_irq_mfioevent();
	sh_clear_mfio_event_flag();
	sh_enable_irq_mfioevent();

	/*I2C* oledI2C = get_i2c_port();
	Adafruit_SSD1306_I2c featherOLED( *oledI2C );*/

	int ret = SH_Max3010x_measure_whrm_estimation_hubaccel( NULL);
	if(ret != 0)
		SERIALOUT("MaximFast Demo Initialization Failed \r\n");

    while(1) {

    	static uint8_t dispCnt = 0;
    	debugled.write( 1- debugled.read());


		while ( SERIAL_AVAILABLE()) {

     		char ch = SERIALIN();
			cmdIntf_build_command(ch);
		}
		hostMode  = get_internal_operating_mode();

        /*featherOLED.clearDisplay();
        featherOLED.setTextCursor(0,0);
        featherOLED.printf("MAX32630FTHR OLED\n");
		featherOLED.printf("HelloWorld %d \r\n", dispCnt++);
		featherOLED.display();
		wait_ms(100);*/


		if( hostMode  == HOSTMODEAPPLICATION) {


			SH_Max3010x_data_report_execute();



		}else { /*BOOTLOADER MODE*/
            // COMMAND PUT SYSTEM IN BOOTLOADER MODE JUST RECEIVE BOOT SEQUENCE BYTES FROM USER. ALL OPERATION GOES ON COMMANDS oF BOOTLOADER COMMAND TABLE. CHECK BOOTLOADER PYTHON SCRIPT FROM PC SIDE.
		}

    }



}



