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

#ifndef _HOST_ACCEL_HELPER_H_
#define _HOST_ACCEL_HELPER_H_

#include <stdint.h>

/* Struct defining the sample of accelerometer
 * Note: as alogorithms expect data as mg or g ; calculations needs to be done over raws accel sensordata and convert to mg or g
 *       acceld data is feed to sensor hub in mg or g format for all 3 axis,s data. Float definitions below are for mg and g
 *       calculations and can be modified to work with fixed point data type.
 *
 * */
typedef struct _accel_data_t {
	float x;
	float y;
	float z;
	int16_t x_raw;
	int16_t y_raw;
	int16_t z_raw;
} accel_data_t;

/**
 * @brief	Initialize the accelerometer on the host device
 */
void CSTMR_SH_HostAccelerometerInitialize();

/**
 * @brief	Set default parameters for the accelerometer
 */
void CSTMR_SH_HostAccelerometerSetDefaults();

/**
 * @brief	Set the sampling rate of the accelerometer
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerSetSampleRate(int sampleRate);

/**
 * @brief	Enable data ready interrupt of the accelerometer
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerEnableDataReadyInterrupt();

/**
 * @brief	Gets a sample from the accelerometer if the sample is ready
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerGet_sensor_xyz(accel_data_t *accel_data);

/**
 * @brief	Add the given sample to the accelerometer queue
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerEnqueueData(accel_data_t *accel_data);

/**
 * @brief	Get the sample count in the accelerometer queue
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerGetDataCount();

/**
 * @brief	Get a sample from the accelerometer queue
 *
 *
 * @return	0 on SUCCESS
 */
int CSTMR_SH_HostAccelerometerDequeuData(accel_data_t *accel_data);






#endif
