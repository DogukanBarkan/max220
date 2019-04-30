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
#include "HostAccelHelper.h"
#include "mbed.h"
#include "bmi160.h"
#include "CircularBuffer.h"

#define BUF_SIZE (32)

I2C I2CM2(P5_7, P6_0); /* SDA, SCL */
InterruptIn bmi160_int_pin(P3_6);
BMI160_I2C bmi160_dev(&I2CM2, BMI160_I2C::I2C_ADRS_SDO_LO, &bmi160_int_pin);
CircularBuffer<accel_data_t, BUF_SIZE> glbl_BMI160_QUEUE;


static BMI160_I2C *pbmi160;




void CSTMR_SH_HostAccelerometerInitialize() {
	pbmi160 = &bmi160_dev;
	pbmi160->reset();
	glbl_BMI160_QUEUE.reset();
	wait_ms(20);
}


void CSTMR_SH_HostAccelerometerSetDefaults() {
	pbmi160->BMI160_DefaultInitalize();
}

int CSTMR_SH_HostAccelerometerSetSampleRate(int sampleRate) {
	return pbmi160->setSampleRate(sampleRate);
}

int CSTMR_SH_HostAccelerometerEnableDataReadyInterrupt() {
	return pbmi160->enable_data_ready_interrupt();
}

int CSTMR_SH_HostAccelerometerGet_sensor_xyz(accel_data_t *accel_data) {
	int ret = 0;
	BMI160::SensorData stacc_data = {0};

	if(pbmi160 == NULL)
		return -1;

	if (pbmi160) {
		ret = pbmi160->getSensorXYZ(stacc_data, BMI160::SENS_2G);
		if (ret < 0)
			return ret;
	}

	accel_data->x = stacc_data.xAxis.scaled;
	accel_data->y = stacc_data.yAxis.scaled;
	accel_data->z = stacc_data.zAxis.scaled;
	accel_data->x_raw = stacc_data.xAxis.raw;
	accel_data->y_raw = stacc_data.yAxis.raw;
	accel_data->z_raw = stacc_data.zAxis.raw;

	return ret;
}


int CSTMR_SH_HostAccelerometerEnqueueData(accel_data_t *accel_data) {
	int ret = 0;
	if(glbl_BMI160_QUEUE.full())
		ret = -1;
	else {
		glbl_BMI160_QUEUE.push(*accel_data);
	}
	return ret;
}

int CSTMR_SH_HostAccelerometerGetDataCount() {
	return glbl_BMI160_QUEUE.size();
}

int CSTMR_SH_HostAccelerometerDequeuData(accel_data_t *accel_data) {
	int ret = 0;

	if(glbl_BMI160_QUEUE.empty()) {
		ret = -1;
	} else {
		glbl_BMI160_QUEUE.pop(*accel_data);
	}
	return ret;
}


