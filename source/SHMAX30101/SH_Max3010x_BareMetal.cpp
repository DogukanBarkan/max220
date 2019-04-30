#include "SH_Max3010x_BareMetal.h"
#include "SHComm.h"
#include "HostAccelHelper.h"
#include <string.h> //for memset
#include <stdint.h>

#define MIN_MACRO(a,b) ((a)<(b)?(a):(b))

// Defines Taken from API doc
#define SSMAX30101_REG_SIZE        1
#define SSMAX30101_MODE1_DATASIZE  12
#define SSWHRM_MODE1_DATASIZE      6
#define SSWHRM_MODE2_DATASIZE      12
#define SSACCEL_MODE1_DATASIZE     6
#define SSAGC_MODE1_DATASIZE       0

// sensor configuration
#define ENABLE_MAX30101
#define ENABLE_ACCEL

#define MAX_NUM_WR_ACC_SAMPLES			 5
#define BMI160_SAMPLE_RATE				25

enum{
	NOACCELUSAGE = 0,
	SENSHUBACCEL = 1,
	HOSTACCEL    = 2
};

// end of senor and algorithm configuration

// end of defines

//function pointer use to perform arithmetic operation
typedef void (*rx_data_callback)(uint8_t *);
typedef struct {
	int data_size;
	rx_data_callback rx_data_parser;
} ss_data_req;


typedef struct Max30101_SH_Status_Tracker {
	uint8_t data_type_enabled;					// what type of data is enabled
	uint8_t sample_count_enabled;				// does me11 provide sample count
	uint32_t sample_count;
	uint8_t data_buf_storage[512];				// store data read from SH
	ss_data_req algo_callbacks[SH_NUM_CURRENT_ALGOS];
	ss_data_req sensor_callbacks[SH_NUM_CURRENT_SENSORS];
	uint8_t sensor_enabled_mode[SH_NUM_CURRENT_SENSORS];
	uint8_t algo_enabled_mode[SH_NUM_CURRENT_ALGOS];
	uint8_t accel_data_from_host;
	int input_fifo_size;
} Max30101_SH_Status_Tracker_t;



typedef struct {
	uint32_t led1;
	uint32_t led2;
	uint32_t led3;
	uint32_t led4;
} max30101_mode1_data;

typedef struct {
	uint16_t hr;
	uint8_t hr_conf;
	uint16_t spo2;
	uint8_t status;
} whrm_mode1_data;

typedef struct {
	uint16_t hr;
	uint8_t hr_conf;
	uint16_t spo2;
	uint8_t status;  // state machine
	uint16_t r_value;
	int8_t status_expanded;
	uint8_t flag_motion;
	uint16_t perfusion;
} whrm_mode2_data;

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} accel_mode1_data;


/*DEMO RELATED DATA TO REPORT RESULTS FROM MAIN APPLICATION*/
uint8_t mFastMesurementHr   = 0;
uint8_t mFastMesurementSpo2 = 0;
uint8_t mFastMesurementConf = 0;

static uint8_t is_agc_usage_required = ENABLE_AGC_USAGE;


// Max30101 Default Callbacks
static void max30101_data_rx(uint8_t* data_ptr) {
	max30101_mode1_data sample;
	sample.led1 = (data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2];
	sample.led2 = (data_ptr[3] << 16) | (data_ptr[4] << 8) | data_ptr[5];
	sample.led3 = (data_ptr[6] << 16) | (data_ptr[7] << 8) | data_ptr[8];
	sample.led4 = (data_ptr[9] << 16) | (data_ptr[10] << 8) | data_ptr[11];
	//SERIALOUT("led1=%d led2=%d  led3=%d  led4=%d \r\n", (int)sample.led1, (int)sample.led2, (int)sample.led3, (int)sample.led4);
}

void whrm1_data_rx(uint8_t* data_ptr) {
	//See API doc for data format
	whrm_mode1_data sample;
	sample.hr = (data_ptr[0] << 8) | data_ptr[1];
	sample.hr_conf = data_ptr[2];
	sample.spo2 = (data_ptr[3] << 8) | data_ptr[4];
	sample.status = data_ptr[5];
	SERIALOUT("hr=%.1f conf=%d spo2=%.1f status=%d\r\n", (float)sample.hr / 10.0, sample.hr_conf, sample.spo2/ 10.0, sample.status);

    mFastMesurementHr = sample.hr;
    mFastMesurementSpo2 = sample.spo2;
    mFastMesurementConf = sample.hr_conf;
}


void whrm2_data_rx(uint8_t* data_ptr)
{
	//See API doc for data format
	whrm_mode2_data sample;
	sample.hr = (data_ptr[0] << 8) | data_ptr[1];
	sample.hr_conf = data_ptr[2];
	sample.spo2 = (data_ptr[3] << 8) | data_ptr[4];
	sample.status = data_ptr[5];
	sample.r_value = (data_ptr[6] << 8) | data_ptr[7];

	sample.status_expanded = data_ptr[8];
	sample.flag_motion = data_ptr[9];
	sample.perfusion = (data_ptr[10] << 8) | data_ptr[11];

	SERIALOUT("hr=%.1f conf=%d spo2=%d status=%d r=%.1f status_expanded=%d motion=%d perfusion=%.1f\r\n",
		(float)sample.hr / 10.0, sample.hr_conf, sample.spo2, sample.status, sample.r_value,
		sample.status_expanded, sample.flag_motion, (float)sample.perfusion / 10.0);

    mFastMesurementHr = sample.hr;
    mFastMesurementSpo2 = sample.spo2;
    mFastMesurementConf = sample.hr_conf;

}

void accel_data_rx(uint8_t* data_ptr) {
#ifdef ENABLE_ACCEL
	//See API doc for data format
	accel_mode1_data sample;
	sample.x = (data_ptr[0] << 8) | data_ptr[1];
	sample.y = (data_ptr[2] << 8) | data_ptr[3];
	sample.z = (data_ptr[4] << 8) | data_ptr[5];
	SERIALOUT("x:%d, y:%d, z:%d\r\n", sample.x, sample.y, sample.z);

#endif
}

static void agc_data_rx(uint8_t* data_ptr) {
	//NOP: AGC does not collect data
}
// end of Max30101 Default Callbacks

static Max30101_SH_Status_Tracker_t * get_config_struct() {
	static Max30101_SH_Status_Tracker_t glbl_max3010x_status_track;
	return &glbl_max3010x_status_track;
}

static void initialize_config_struct() {
	Max30101_SH_Status_Tracker_t *p_glbl_max3010x_status_track = get_config_struct();

		//set all the values to 0
	memset(p_glbl_max3010x_status_track, 0, sizeof(*p_glbl_max3010x_status_track));
	// max30101
	p_glbl_max3010x_status_track->sensor_callbacks[SH_SENSORIDX_MAX30101].data_size = SSMAX30101_MODE1_DATASIZE;
	p_glbl_max3010x_status_track->sensor_callbacks[SH_SENSORIDX_MAX30101].rx_data_parser = &max30101_data_rx;
	// accel
	p_glbl_max3010x_status_track->sensor_callbacks[SH_SENSORIDX_ACCEL].data_size = SSACCEL_MODE1_DATASIZE;
	p_glbl_max3010x_status_track->sensor_callbacks[SH_SENSORIDX_ACCEL].rx_data_parser = &accel_data_rx;
	// agc
	p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_AGC].data_size = SSAGC_MODE1_DATASIZE;
	p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_AGC].rx_data_parser = &agc_data_rx;
	// whrmMode1
	p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].data_size = SSWHRM_MODE1_DATASIZE;
	p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].rx_data_parser = &whrm1_data_rx;
	// whrmMode2
	//p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].data_size = SSWHRM_MODE2_DATASIZE;
	//p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].rx_data_parser = &whrm2_data_rx;


}



int CSTMR_SH_FeedAccDataIntoSH(Max30101_SH_Status_Tracker_t *p_glbl_max3010x_status_track  /*p_max8614x_status_track*/) {

	static accel_data_t peek_buf[MAX_NUM_WR_ACC_SAMPLES];
	static uint8_t tx_buf[MAX_NUM_WR_ACC_SAMPLES * sizeof(accel_mode1_data) + 2]; // 2 bytes for the command
	if(!p_glbl_max3010x_status_track->accel_data_from_host) {
		return -1;
	} else {
		accel_data_t accel_data = {0};
		accel_mode1_data acc_sample;
		int num_tx, num_samples, num_bytes = 0, num_wr_bytes = 0;
		int num_written_samples, nb_expected;
		int ret = 0;

		// get accelerometer data
		ret = CSTMR_SH_HostAccelerometerGet_sensor_xyz(&accel_data);
		if (ret < 0)
			return ret;

		if(CSTMR_SH_HostAccelerometerEnqueueData(&accel_data) != 0) {
			SERIALOUT("Thrown an accel sample\r\n");
		}

		if(CSTMR_SH_HostAccelerometerGetDataCount() < MAX_NUM_WR_ACC_SAMPLES) {
			return -1;
		}

		ret = sh_get_num_bytes_in_input_fifo(&num_bytes);
		if (ret != 0) {
			SERIALOUT("Unable to read num bytes in input fifo\r\n");
			return -1;
		}
		num_tx = p_glbl_max3010x_status_track->input_fifo_size - num_bytes;
		if (num_tx <= 0) {
			SERIALOUT("num_tx can't be negative\r\n");
			return -1;
		}
		num_samples = num_tx / sizeof(accel_mode1_data);
		num_samples = MIN_MACRO(num_samples, MAX_NUM_WR_ACC_SAMPLES);
		num_tx = num_samples * sizeof(accel_mode1_data);
		if (num_samples == 0) {
			SERIALOUT("Input FIFO is Full\r\n");
			return -1;
		}

		for(int i = 0; i < num_samples; ++i) {
			ret |= CSTMR_SH_HostAccelerometerDequeuData(&peek_buf[i]);
		}
		if (ret != 0) {
			SERIALOUT("CSTMR_SH_HostAccelerometerDequeuData failed\r\n");
			return -1;
		}


		for (int i = 2, j = 0; j < num_samples; i+= sizeof(accel_mode1_data), j++) {
			accel_data = peek_buf[j];
			acc_sample.x = (int16_t)(accel_data.x*1000);
			acc_sample.y = (int16_t)(accel_data.y*1000);
			acc_sample.z = (int16_t)(accel_data.z*1000);
			tx_buf[i] = acc_sample.x;
			tx_buf[i + 1] = acc_sample.x >> 8;
			tx_buf[i + 2] = acc_sample.y;
			tx_buf[i + 3] = acc_sample.y >> 8;
			tx_buf[i + 4] = acc_sample.z;
			tx_buf[i + 5] = acc_sample.z >> 8;

		}

		ret = sh_feed_to_input_fifo(tx_buf, num_tx + 2, &num_wr_bytes);
		if(ret != 0) {
			SERIALOUT("sh_feed_to_input_fifo\r\n");
			return -1;
		}
		num_written_samples = num_wr_bytes / sizeof(accel_mode1_data);
		if(num_written_samples != num_samples) {
			SERIALOUT("num_written_samples failed\r\n");
			return -1;
		}
	}
	return 0;
}

void SH_Max3010x_data_report_execute(void) {
	int num_samples, databufLen;
	uint8_t *databuf;

	Max30101_SH_Status_Tracker_t *p_glbl_max3010x_status_track = get_config_struct();

	// prepare the buffer to store the results
	databuf = p_glbl_max3010x_status_track->data_buf_storage;
	databufLen = sizeof(p_glbl_max3010x_status_track->data_buf_storage);

	// poll SH
	sh_ss_execute_once(databuf, databufLen, &num_samples);

	if(num_samples) {
		//Skip status byte
		uint8_t *data_ptr = &databuf[1];

		int i = 0;
		for (i = 0; i < num_samples; i++) {
			int sh_data_type = p_glbl_max3010x_status_track->data_type_enabled;
			if (p_glbl_max3010x_status_track->sample_count_enabled) {
				p_glbl_max3010x_status_track->sample_count = *data_ptr++;
			}
			//Chop up data and send to modules with enabled sensors
			if (sh_data_type == SS_DATATYPE_RAW || sh_data_type == SS_DATATYPE_BOTH) {
				for (int i = 0; i < SH_NUM_CURRENT_SENSORS; i++) {
					if (p_glbl_max3010x_status_track->sensor_enabled_mode[i]) {
						p_glbl_max3010x_status_track->sensor_callbacks[i].rx_data_parser(data_ptr);
						data_ptr += p_glbl_max3010x_status_track->sensor_callbacks[i].data_size;
					}
				}
			}
			if (sh_data_type == SS_DATATYPE_ALGO || sh_data_type == SS_DATATYPE_BOTH) {
				for (int i = 0; i < SH_NUM_CURRENT_ALGOS; i++) {
					if (p_glbl_max3010x_status_track->algo_enabled_mode[i]) {
						p_glbl_max3010x_status_track->algo_callbacks[i].rx_data_parser(data_ptr);
						data_ptr += p_glbl_max3010x_status_track->algo_callbacks[i].data_size;
					}
				}
			}
		}

		CSTMR_SH_FeedAccDataIntoSH(p_glbl_max3010x_status_track);
	}
}

int SH_Max3010x_Maximfast_init( const int algoExecutionMode /* 1: mfastExecMode1 2: mfastExecMode1*/,
		                        const int agcUsage ,        /* O: no 1:yes*/
							    const int accelBehavior,    /* 0:no 1:hub 2:host*/
							    const int sensHubDataType   /* 1: SS_DATATYPE_RAW 2:SS_DATATYPE_ALGO 3:SS_DATATYPE_BOTH	*/)
{

	int status;

 	// first initialize the global config struct, status tracker.
	initialize_config_struct();

	Max30101_SH_Status_Tracker_t *p_glbl_max3010x_status_track = get_config_struct();

	/* ME11 initialization based on Smart Sensor API commands*/

	/* Disable IRQ based Event reporting from Sensor Hub ME11*/
	sh_disable_irq_mfioevent();

	// get input fifo size
	if( accelBehavior == HOSTACCEL) {
		status = sh_get_input_fifo_size(&p_glbl_max3010x_status_track->input_fifo_size);
		if (status != SS_SUCCESS) {
			SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
			SERIALOUT("FAILED at line %d\n", __LINE__);
			return COMM_GENERAL_ERROR;;
		}
	}
	status = sh_set_data_type(sensHubDataType /*SS_DATATYPE_BOTH*/, false);
	if (status != 0) {
		SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
		SERIALOUT("FAILED at line %d\n", __LINE__);
		return COMM_GENERAL_ERROR;
	}else {
		p_glbl_max3010x_status_track->data_type_enabled    = sensHubDataType; /*SS_DATATYPE_BOTH;*/
		p_glbl_max3010x_status_track->sample_count_enabled = false;
	}

	status = sh_set_fifo_thresh(15);
	if (status != 0) {
		SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
		SERIALOUT("FAILED at line %d\n", __LINE__);
		return COMM_GENERAL_ERROR;
	}

	if (agcUsage) {
		status = sh_enable_algo_withmode(SH_ALGOIDX_AGC, 1, SSAGC_MODE1_DATASIZE);
		if (status != SS_SUCCESS) {
			SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
			SERIALOUT("FAILED at line %d\n", __LINE__);
			sh_enable_irq_mfioevent();
			return COMM_GENERAL_ERROR;
		}
		p_glbl_max3010x_status_track->algo_enabled_mode[SH_ALGOIDX_AGC] = 0x01;
	}

	status = sh_sensor_enable(SH_SENSORIDX_MAX30101, SSMAX30101_MODE1_DATASIZE, SH_INPUT_DATA_DIRECT_SENSOR);
	if (status != SS_SUCCESS) {
		SERIALOUT("\r\n err=%d\r\n",  COMM_GENERAL_ERROR);
		SERIALOUT("FAILED at max3010x sensor init line %d\n", __LINE__);
		sh_enable_irq_mfioevent();
		return COMM_GENERAL_ERROR;
	}
	p_glbl_max3010x_status_track->sensor_enabled_mode[SH_SENSORIDX_MAX30101] = 0x01;


   if(accelBehavior == 1){

		status = sh_sensor_enable(SH_SENSORIDX_ACCEL, SSACCEL_MODE1_DATASIZE, SH_INPUT_DATA_DIRECT_SENSOR);
		if (status != SS_SUCCESS) {
			SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
			SERIALOUT("FAILED at line %d\n", __LINE__);
		}
		p_glbl_max3010x_status_track->accel_data_from_host = false;
		p_glbl_max3010x_status_track->sensor_enabled_mode[SH_SENSORIDX_ACCEL] = 0x01;

   }else if(accelBehavior == 2){

		CSTMR_SH_HostAccelerometerInitialize();
		CSTMR_SH_HostAccelerometerSetDefaults();
		status = CSTMR_SH_HostAccelerometerSetSampleRate(BMI160_SAMPLE_RATE);
		if (status != 0) {
			return status;
		}
		status = CSTMR_SH_HostAccelerometerEnableDataReadyInterrupt();
		if(status != 0){
			return status;
		}

		status = sh_sensor_enable(SH_SENSORIDX_ACCEL, SSACCEL_MODE1_DATASIZE, SH_INPUT_DATA_FROM_HOST);
		if (status != 0) {
			SERIALOUT("\r\n err=%d\r\n", COMM_GENERAL_ERROR);
		    SERIALOUT("FAILED at line %d\r\n", __LINE__);
			return status;
		}
		p_glbl_max3010x_status_track->accel_data_from_host = true;
		p_glbl_max3010x_status_track->sensor_enabled_mode[SH_SENSORIDX_ACCEL] = 0x01;

   }

    // DEBUG: Check this function again!
	int algoSampleSize = (algoExecutionMode==1)? SSWHRM_MODE1_DATASIZE:SSWHRM_MODE2_DATASIZE;
	status = sh_enable_algo_withmode(SH_ALGOIDX_WHRM, algoExecutionMode ,algoSampleSize);
	if (status != SS_SUCCESS) {
		SERIALOUT("\r\n err=%d\r\n",  COMM_GENERAL_ERROR);
		SERIALOUT("FAILED at MaximFast algo init line %d\n", __LINE__);
		sh_enable_irq_mfioevent();
		return COMM_GENERAL_ERROR;
	}
	if(algoExecutionMode == 2){
		p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].data_size = SSWHRM_MODE2_DATASIZE;
		p_glbl_max3010x_status_track->algo_callbacks[SH_ALGOIDX_WHRM].rx_data_parser = &whrm2_data_rx;
	}
	p_glbl_max3010x_status_track->algo_enabled_mode[SH_ALGOIDX_WHRM] = 0x01;

	/* Enable IRQ based Event reporting from Sensor Hub ME11*/
	sh_enable_irq_mfioevent();

	SERIALOUT("\r\n err=%d\r\n",  status);
	return COMM_SUCCESS;

}


/* COMMAND TABLE FUNCTIONS*/

int SH_Max3010x_set_ppgreg(const char *addr_value_args){

	//CMD: set_reg ppgsensor 0xAA 0xAA
	int addr,val,status;
	if( sscanf(addr_value_args,"%*s %*s %4x %10x", &addr , &addr ) == 2 ){
        status = sh_set_reg(SH_SENSORIDX_MAX30101, (uint8_t) addr, (uint32_t) val, SSMAX30101_REG_SIZE);
        if(status == 0)
        	SERIALOUT("OK \r\n");
	}else
		    SERIALOUT("ERR \r\n");

    return status;
}

int SH_Max3010x_get_ppgreg(const char *addr_arg){

	//CMD: get_reg ppgsensor 0xAA
	int addr;
	int status = -1;
	uint32_t val;

	if( sscanf(addr_arg,"%*s %*s %4x", &addr) == 1 ){
		int status = sh_get_reg(SH_SENSORIDX_MAX30101, (uint8_t) addr, &val);
        if(status == 0)
        	SERIALOUT("reg_val=%02X \r\n",val);
        else
        	SERIALOUT("COMM ERR \r\n");
	}else
		    SERIALOUT("ERR \r\n");

    return status;
}




/*IMPORTANT: AGC usage setting should be set before MaximFast algorithm initialized */
int SH_Max3010x_set_ppg_agcusage(const char *config_arg){

	int status = -1;
	uint32_t val;
    if( sscanf(config_arg, "%*s %*s %*s %d", &val) == 1 ){
    	is_agc_usage_required = (val == 0)? DISABLE_AGC_USAGE:ENABLE_AGC_USAGE;
    	status =  SS_SUCCESS;
    }

    return status;  // if command error return -1 if operational error return >0 error

}


int SH_Max3010x_get_ppg_agcusage(const char *null_arg){

	int status = 0;
	SERIALOUT(" is agc used = %d \r\n", is_agc_usage_required );
    return status;  // if command error return -1 if operational error return >0 error

}

int SH_Max3010x_self_test(const char *null_arg){

	int status = -1;

	uint8_t test_result;
	bool test_failed = false;
	SERIALOUT("starting selftest_max30101\r\n");
	sh_mfio_selftest();
	status = sh_self_test(SH_SENSORIDX_MAX30101, &test_result, 500);
	if(status != SS_SUCCESS){
		SERIALOUT("ss_int->self_test(SS_SENSORIDX_MAX30101, &test_result) has failed err<-1>\r\n");
		test_failed = true;
	}
	// reset mfio pin to old state
	if(!sh_reset_mfio_irq()){
		SERIALOUT("smart sensor reset_mfio_irq has failed err<-1>\r\n");
		test_failed = true;
	}
	// reset the sensor to turn off the LED
	status = sh_reset_to_main_app();

	if(test_failed){
		status = -1;
	}else{
		status = SS_SUCCESS;
	}

	SERIALOUT("\r\n%s err=%d\r\n", CMDTABLE3010xMFAST[5].cmdStr , status);
    return status;

}


int SH_Max3010x_stop(const char *null_arg) {

	sh_disable_irq_mfioevent();
	Max30101_SH_Status_Tracker_t *p_glbl_max3010x_status_track = get_config_struct();

	for(int i = 0; i < SH_NUM_CURRENT_SENSORS; ++i) {
		if(p_glbl_max3010x_status_track->sensor_enabled_mode[i]) {
			p_glbl_max3010x_status_track->sensor_enabled_mode[i] = 0;
			sh_sensor_disable(i);
		}

	}

	for(int i = 0; i < SH_NUM_CURRENT_ALGOS; ++i) {
		if(p_glbl_max3010x_status_track->algo_enabled_mode[i]) {
			p_glbl_max3010x_status_track->algo_enabled_mode[i] = 0;
			sh_disable_algo(i);
		}
	}

	sh_clear_mfio_event_flag();
	sh_enable_irq_mfioevent();

	return 0x00;
}



int SH_Max3010x_measure_whrm_estimation_hubaccel  ( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastEstimationMode,
											 is_agc_usage_required,
											 SENSHUBACCEL,
											 SS_DATATYPE_BOTH);
    return status;
}

int SH_Max3010x_measure_whrm_estimation_hostaccel( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastEstimationMode,
											 is_agc_usage_required,
											 HOSTACCEL,
											 SS_DATATYPE_BOTH);
    return status;

}
int SH_Max3010x_measure_whrm_estimation_noaccel( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastEstimationMode,
											 is_agc_usage_required,
											 NOACCELUSAGE,
											 SS_DATATYPE_BOTH);
    return status;

}

int SH_Max3010x_measure_whrm_calibration_hubaccel ( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastCalibrationMode,
											 is_agc_usage_required,
											 SENSHUBACCEL,
											 SS_DATATYPE_BOTH);
    return status;

}
int SH_Max3010x_measure_whrm_calibration_hostaccel( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastCalibrationMode,
											 is_agc_usage_required,
											 HOSTACCEL,
											 SS_DATATYPE_BOTH);
    return status;

}
int SH_Max3010x_measure_whrm_calibration_noaccel  ( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastCalibrationMode,
											 is_agc_usage_required,
											 NOACCELUSAGE,
											 SS_DATATYPE_BOTH);
    return status;

}
int SH_Max3010x_measure_whrm_validation_hubaccel  ( const char *null_arg ) {

	int status = SH_Max3010x_Maximfast_init( mfastEstimationMode,
											 0 /*is_agc_usage_required*/,
											 SENSHUBACCEL,
											 SS_DATATYPE_ALGO);
    return status;

}

