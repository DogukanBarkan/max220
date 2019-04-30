/*
 * bootldrInterface.cpp
 *
 *  Created on: Feb 7, 2019
 *      Author: Yagmur.Gok
 */

#include <mbed.h>  /*for type definitions*/
#include "bootldrAPI.h"
#include "SHComm.h"
#include "demoDefinitions.h"

//extern Serial daplink;

//#define SERIALOUT printf
//#define SERIALIN  printf

#define COMM_SUCCESS        0
#define COMM_GENERAL_ERROR  -1
#define COMM_INVALID_PARAM  -254
#define COMM_NOT_RECOGNIZED -255

#define BL_FLASH_ERR_GENERAL  	-1
#define BL_FLASH_ERR_CHECKSUM 	-2
#define BL_FLASH_ERR_AUTH      	-3
#define BL_SET_NUM_PAGES_FAIL	-4
#define BL_FLASS_ERASE_FAIL		-5
#define BL_SET_IV_FAIL			-6
#define BL_FLASHING_FAIL		-7
#define BL_RAM_ALLOC_FAIL		-8

#define AES_NONCE_SIZE 11
#define AES_AUTH_SIZE  16
#define BOOTLOADER_MAX_PAGE_SIZE (8192)
#define CHECKBYTES_SIZE (16)
#define FLASHCMDBYTES (2)

#define SS_BOOTLOADER_ERASE_DELAY	700
#define AES_NONCE_SIZE				11
#define AES_AUTH_SIZE				16
#define MAX_PAGE_SIZE				8192
#define CHECKBYTES_SIZE				16

#define PAGE_WRITE_DELAY_MS			170
#define MAX_PAGE_NUMBER				31
#define PAGE_ERASE_CMD_SLEEP_MS		50
#define BL_CFG_SAVE_CMD_SLEEP_MS	50

static int parse_iv(const char* cmd, uint8_t* iv_bytes);
static int parse_auth(const char* cmd, uint8_t *auth_bytes);
static int is_hub_ready_for_flash(void);
static void clear_state_info(void);


typedef struct {
	int num_allocated_pages; /* Allocated page size */
	int num_pages;
	int num_received_pages;
	int page_size;
	uint8_t auth[AES_AUTH_SIZE];
	uint8_t nonce[AES_NONCE_SIZE];
	uint8_t pages[MAX_PAGE_NUMBER * (MAX_PAGE_SIZE + CHECKBYTES_SIZE)]; //TODO: Use dynamic allocation
} app_image_t;


static app_image_t *app_image = NULL;

static struct {

	uint32_t num_pages;
	uint32_t page_size;
	uint32_t hub_mode_bootloader;
	uint32_t is_iv_set;
	uint32_t is_auth_done;
	uint32_t is_flash_erased;
	uint32_t flag_image_on_ram;
	uint32_t bootcmds_delay_factor;
	uint32_t ebl_mode;

}bootldrState = { 0 , 0 , 0 ,0 , 0 , 0 , 0 , 1 ,1};



int BOOTLDR_get_host_bootloader_state(const char *null_arg){

	SERIALOUT(" \r\n BOOT STATE INFO: \r\n num_pages= %d \r\n page_size= %d \r\n is_iv_set= %d \r\n is_auth_done= %d \r\n is_flash_erased= %d \r\n flag_image_on_ram= %d \r\n bootcmds_delay_factor= %d \r\n ebl_mode= %d \r\n",
			                             bootldrState.num_pages,
										 bootldrState.page_size,
										 bootldrState.is_iv_set,
										 bootldrState.is_auth_done,
										 bootldrState.is_flash_erased,
										 bootldrState.flag_image_on_ram,
										 bootldrState.bootcmds_delay_factor,
										 bootldrState.ebl_mode );


}

int SH_BOOTLDR_enter_blmode(const char *null_arg){

	int status = 0x00;

	//status = sh_put_in_bootloader();
	//status = sh_set_ebl_mode((uint8_t)1); /* 1: GPIO rest 0: CMD reset*/

    if( status == 0x00) {

	    //////////status = sh_reset_to_bootloader(); - CHECKIF PROBLEM STILL PRESENT!!
    	status = sh_debug_reset_to_bootloader();
	    //status = sh_set_sensorhub_operating_mode((uint8_t)0x08);
		if (status == 0x00) {
		   SERIALOUT("\r\n%s err=%d\r\n", "bootldr", COMM_SUCCESS);
		   bootldrState.hub_mode_bootloader  = 1;
		}
		else
		   SERIALOUT("\r\n%s err=%d\r\n", "bootldr", COMM_GENERAL_ERROR);
    }

	return status;

}


int SH_BOOTLDR_exit_blmode(const char *null_arg){

	int status;
	status = sh_reset_to_main_app() ; //sh_exit_from_bootloader();
	if (status == 0x00)
		printf("\r\n exited from bootloader mode \r\n");

	if (status == 0x00) {
	   SERIALOUT("\r\n%s err=%d\r\n", "exit", COMM_SUCCESS);
       bootldrState.hub_mode_bootloader  = 0;
	}
    else
       SERIALOUT("\r\n%s err=%d\r\n", "exit", COMM_GENERAL_ERROR);

	return status;

}

int SH_BOOTLOADER_image_on_ram( const char *arg ){

      int status = COMM_SUCCESS;
	  int tmp;
      sscanf(arg, "%*s %d", &tmp );
      bootldrState.flag_image_on_ram = (tmp > 0)? 1:0 ;

      if(tmp == 1){

    	  app_image = (app_image_t*) malloc(sizeof(*app_image));
    	  if(app_image != NULL){

    		  app_image->num_allocated_pages = MAX_PAGE_NUMBER;
    		  app_image->num_pages = 0;
    		  app_image->num_received_pages = 0;
    		  app_image->page_size = MAX_PAGE_SIZE;
    		  //app_image->nonce = {0};
    		  //app_image->auth  = {0};
    		  //app_image->pages = {0};

     	  }else {
    		  SERIALOUT("Memory allocation fail for ram image \r\n");
    		  status = BL_RAM_ALLOC_FAIL;
    	  }

      }

      SERIALOUT("\r\n%s err=%d\r\n", "image_on_ram", COMM_SUCCESS);

}

int SH_BOOTLDR_get_pagesz(const char *null_arg){

	int status;
	int pageSize;

	if (bootldrState.flag_image_on_ram) {
		SERIALOUT("\r\n%s value=%d err=%d\r\n", "page_size", MAX_PAGE_SIZE, COMM_SUCCESS);
		app_image->page_size   = MAX_PAGE_SIZE;
		bootldrState.page_size = MAX_PAGE_SIZE;
		SERIALOUT("\r\n%s err=%d\r\n", "image_on_ram", 1);

	}else {

		status = sh_get_bootloader_pagesz(&pageSize);
		if (status == 0x00 ){
			  if( pageSize == -2){
				  SERIALOUT("\r\n Page size over maximum allowable \r\n");
				  status = -1;
			  }else {
				  SERIALOUT("\r\n%s value=%d err=%d\r\n", "page_size", pageSize, status);
				  bootldrState.page_size = pageSize;
			  }
		}else
			SERIALOUT("\r\n%s err=%d\r\n", "page_size", status);

	}
    return status;

}

int SH_BOOTLDR_set_pagecount(const char *arg){

	int status = -1;
	int pageCount;

    if(sscanf(arg, "%*s %d", &pageCount)){

        if(bootldrState.flag_image_on_ram){
        	app_image->num_pages =  (pageCount <= MAX_PAGE_NUMBER)?  pageCount:0;
        	app_image->num_allocated_pages = MAX_PAGE_NUMBER;
        	bootldrState.num_pages = app_image->num_pages;

        }else {

			status = sh_set_bootloader_numberofpages(pageCount);
			if (status == 0x00){
				bootldrState.num_pages = pageCount;
			}else
				bootldrState.num_pages = 0;
        }
    }
    SERIALOUT("\r\n%s err=%d\r\n", "num_pages", status);
	return status;

}

int SH_BOOTLDR_set_iv(const char *arg) {

    uint8_t iv_bytes[AES_NONCE_SIZE];
    int status =  parse_iv(arg, &iv_bytes[0]);
    if( status == 0x00){

     	if(bootldrState.flag_image_on_ram){
    		int i=0;
    		for(i = 0 ; i != AES_NONCE_SIZE ; i++)
    			app_image->nonce[i] = iv_bytes[i];
    		bootldrState.is_iv_set = 1;

    	}else {

			status = sh_set_bootloader_iv(iv_bytes);
			if( status == 0x00 ) {
				bootldrState.is_iv_set = 1;
			}
			else {
				bootldrState.is_iv_set = 0;
				status = COMM_GENERAL_ERROR;
			}
		}
	}
    SERIALOUT("\r\n%s err=%d\r\n", "set_iv", status);

    return status;

}


int SH_BOOTLDR_set_authentication(const char *arg){

	uint8_t auth_bytes[AES_AUTH_SIZE];
	int status = parse_auth(arg, &auth_bytes[0]);
    if( status == 0x00){

    	if(bootldrState.flag_image_on_ram){
    		int i=0;
    		for(i = 0 ; i != AES_AUTH_SIZE ; i++)
    			app_image->auth[i] = auth_bytes[i];
    		bootldrState.is_auth_done = 1;

    	}else {

			status = sh_set_bootloader_auth(auth_bytes);
			if( status == 0x00 ){
				bootldrState.is_auth_done = 1;
			}
			else
				status = COMM_GENERAL_ERROR;
    	}

    }
    SERIALOUT("\r\n%s err=%d\r\n", "set_auth", status);

    return status;

}

int SH_BOOTLDR_eraseflash(const char *null_arg){

	int status;
	if(bootldrState.flag_image_on_ram){
		SERIALOUT("\r\n%s err=%d\r\n", "erase", COMM_SUCCESS);
		status == 0x00 /*SS_SUCCESS*/;

	}else {

		status = sh_set_bootloader_erase();
		if(status == 0x00 /*SS_SUCCESS*/) {
			bootldrState.is_flash_erased = 1;
		}else{
			status = COMM_GENERAL_ERROR;
		}
		SERIALOUT("\r\n%s err=%d\r\n", "erase", status);

	}
    return status;

}


int SH_BOOTLDR_receive_image_to_ram(void){


	int status;
	int totalBytes = 0;
	int currentPage = 1;

	if( app_image != NULL && app_image->num_allocated_pages > 0) {

		uint8_t *page = &app_image->pages[0];
		uint32_t offset = 0;
		while (currentPage <= app_image->num_pages) {

			while (totalBytes < (MAX_PAGE_SIZE + CHECKBYTES_SIZE)) {
				page[totalBytes++] = SERIALIN(); // daplink.getc();  ; /////////////////////////////////////////////////////m_USB->_getc();
			}

			currentPage++;
			SERIALOUT("\r\npageFlashDone err=%d\r\n", COMM_SUCCESS);

			offset += MAX_PAGE_SIZE + CHECKBYTES_SIZE;
			page = &app_image->pages[offset];
			totalBytes = 0;
		}

		app_image->num_received_pages = currentPage;

		status =  COMM_SUCCESS;

	}else
	    status =  COMM_GENERAL_ERROR;

	return status;

}

int SH_BOOTLDR_flash_pages(void){


    int totalBytes = 0;
    int currentPage = 1;
	char charbuf_flash[256];
	int data_len_flash = 0;
	int status;

    static uint8_t tx_buf[ BOOTLOADER_MAX_PAGE_SIZE + CHECKBYTES_SIZE + FLASHCMDBYTES] = { SS_FAM_W_BOOTLOADER, SS_CMDIDX_SENDPAGE };
    uint8_t *data_buffer = &tx_buf[2];

    if(!is_hub_ready_for_flash()){
    	printf("missing condition for flashing:  no page size , no page number, iv notset , no outhentication , flash noterased or mode is not bootloader!\r\n");
    	clear_state_info();
    	return -1;
    }
    printf(" \r\n NOW WE ARE FLASHING THE PAGES..........\r\n");
    printf(" \r\n page_size:  %d \r\n", bootldrState.page_size);
    printf(" \r\n num_pages:  %d \r\n", bootldrState.num_pages);

    while (currentPage <= bootldrState.num_pages) {

        while (totalBytes < (bootldrState.page_size + CHECKBYTES_SIZE)) {
            data_buffer[totalBytes++] = SERIALIN(); //daplink.getc(); //m_USB->_getc();  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! MYG
            //Here we should be able to take the data over BLE
        }
        printf(" \r\n NOW WE GOT A  PAGE..........\r\n");
        //status = sh_bootloader_flashpage(&tx_buf[0] , bootldrState.page_size);

        status = sh_write_cmd(tx_buf, bootldrState.page_size + CHECKBYTES_SIZE + 2, 2000);

        if (status == 0x00)
        		printf(" \r\n NOW WE PUSHED A  PAGE..........\r\n");
/*
        if (status == 0x00){
        	currentPage++;
        	SERIALOUT("\r\npageFlashDone err=%d\r\n", COMM_SUCCESS);
        }else{

        	if (status == SS_ERR_BTLDR_CHECKSUM)
        		SERIALOUT("\r\npageFlashDone err=%d\r\n", BL_FLASH_ERR_CHECKSUM);
        	else
        		SERIALOUT("\r\npageFlashDone err=%d\r\n", BL_FLASH_ERR_GENERAL);

        }
 */

        if (status == SS_ERR_BTLDR_CHECKSUM) {

            data_len_flash = snprintf(charbuf_flash, sizeof(charbuf_flash), "\r\npageFlashDone err=%d\r\n", FLASH_ERR_CHECKSUM);
            SERIALOUT(charbuf_flash);
        } else if (status != SS_SUCCESS) {

            data_len_flash = snprintf(charbuf_flash, sizeof(charbuf_flash), "\r\npageFlashDone err=%d\r\n", FLASH_ERR_GENERAL);
            SERIALOUT(charbuf_flash);
        } else {
            currentPage++;

            data_len_flash = snprintf(charbuf_flash, sizeof(charbuf_flash), "\r\npageFlashDone err=%d\r\n", COMM_SUCCESS);
            SERIALOUT(charbuf_flash);
        }
        totalBytes = 0;

    }
    //SERIALOUT(" all pages are flashed \r\n");
    clear_state_info();

    return status;

}


int SH_BOOTLDR_flash(const char *null_arg){

	int status;
	if(bootldrState.flag_image_on_ram)
		status = SH_BOOTLDR_receive_image_to_ram();
	else
		status = SH_BOOTLDR_flash_pages();

	SERIALOUT("\r\n%s err=%d\r\n", "flash", status);

	return status;
}


int SH_BOOTLDR_flash_appimage_from_ram(const char *null_arg){


	int currentPage = 1;
	uint32_t offset = 0;
	int ret;

	if(app_image == NULL)
	    return -1;

	/* Put device to bootloader mode */
	int status = SH_BOOTLDR_enter_blmode(NULL);
	if( status != 0x00)
         return -1;

	wait_ms(10);

	status = sh_set_bootloader_numberofpages(app_image->num_pages);
	SERIALOUT("*** set_num_page... ret: %d\n", ret);
	if (status != 0x00) {
		return BL_SET_NUM_PAGES_FAIL;
	}

	status = sh_set_bootloader_iv(app_image->nonce);
	SERIALOUT("*** set_iv... ret: %d\n", ret);
	if (status != 0) {
		return BL_SET_IV_FAIL;
	}

	status = sh_set_bootloader_auth(app_image->auth);
	SERIALOUT("*** set_auth... ret: %d\n", ret);
	if (status != 0) {
		return BL_FLASH_ERR_AUTH;
	}

	status = sh_set_bootloader_erase() ;
	SERIALOUT("*** erase app memory... ret: %d\n", ret);
	if (status != 0) {
		return BL_FLASS_ERASE_FAIL;
	}

	static uint8_t tx_buf[MAX_PAGE_SIZE + CHECKBYTES_SIZE + 2];

	tx_buf[0] = SS_FAM_W_BOOTLOADER;
	tx_buf[1] = SS_CMDIDX_SENDPAGE;

	while (currentPage <= app_image->num_pages) {

		memcpy(&tx_buf[2], &app_image->pages[offset], MAX_PAGE_SIZE + CHECKBYTES_SIZE);

		status = sh_write_cmd(tx_buf, MAX_PAGE_SIZE + CHECKBYTES_SIZE + 2, bootldrState.bootcmds_delay_factor * PAGE_WRITE_DELAY_MS);

		if (status == SS_ERR_BTLDR_CHECKSUM) {

			SERIALOUT("\r\npageFlashDone err=%d\r\n", BL_FLASH_ERR_CHECKSUM);
			break;
		} else if (status != SS_SUCCESS) {

			SERIALOUT("\r\npageFlashDone err=%d\r\n", BL_FLASH_ERR_GENERAL);
			break;
		} else {

			SERIALOUT("\r\npageFlashDone err=%d\r\n", COMM_SUCCESS);
		}

		offset += MAX_PAGE_SIZE + CHECKBYTES_SIZE;
		currentPage++;
	}

	return COMM_SUCCESS;

}

int SH_BOOTLDR_set_host_ebl_mode(const char *arg) {

    int status;
    int tmp;
    sscanf(arg, "%*s %*s %*s %d", &tmp);
    status = sh_set_ebl_mode(tmp);
    if( status == 0x00) {
    	bootldrState.ebl_mode = tmp;
    }else
    	status = COMM_INVALID_PARAM;

    SERIALOUT("\r\n%s err=%d\r\n", "set_cfg host ebl",status);

    return status;

}

int SH_BOOTLDR_get_host_ebl_mode(const char *null_arg){

      int value;
      value = sh_get_ebl_mode();
      SERIALOUT("\r\n%s value=%s\r\n", "get_cfg host ebl", (value==1)? "GPIO_RST_MODE":"CMD_RST_MODE");
      return 0x00;
}

int SH_BOOTLDR_set_host_bootcmds_delay_factor( const char *arg) {

     int status;
	 int tmp;
     sscanf(arg, "%*s %*s %*s %d", &tmp);
     status =  sh_set_bootloader_delayfactor( tmp);
     if( status == 0x00) {
    	 bootldrState.bootcmds_delay_factor = tmp;
     }else
    	 status = COMM_INVALID_PARAM;

     SERIALOUT("\r\n%s err=%d\r\n", "set_cfg host cdf",status);

     return status;

}

int SH_BOOTLDR_get_host_bootcmds_delay_factor( const char *null_arg){

	int value;
	value =  sh_get_bootloader_delayfactor();
	SERIALOUT("\r\n%s value=%d \r\n", "get_cfg host cdf", value);
	return 0x00;

}

static int is_hub_ready_for_flash(void){

	int status = 0;
	if( bootldrState.hub_mode_bootloader == 1 &&
		 bootldrState.is_auth_done == 1	&&
		  bootldrState.is_iv_set == 1 &&
		   bootldrState.is_flash_erased == 1 &&
		    bootldrState.num_pages > 0 &&
			 bootldrState.page_size > 0 )
		    	status =  1;

    return status;
}

static void clear_state_info(void) {

	bootldrState.is_auth_done    = 0;
	bootldrState.is_iv_set       = 0;
	bootldrState.is_flash_erased = 0;
	bootldrState.num_pages       = 0;
	bootldrState.page_size       = 0;
	bootldrState.hub_mode_bootloader;

	return;
}

static int parse_iv(const char* cmd, uint8_t* iv_bytes) {

	int status = 0x00;
	char cmdStr[] = "set_iv ";
	int length = strlen(cmd);
	int expected_length = strlen(cmdStr) + 2*AES_NONCE_SIZE;

	if (length != expected_length) {
		SERIALOUT("Couldn't parse IV, incorrect number of characters (len:%d, expected:%d)\n",
               length, expected_length);
		status = COMM_INVALID_PARAM;
	}else{

		const char* ivPtr = cmd + strlen(cmdStr);
		int num_found;
		int byteVal;
		for (int ividx = 0; ividx < AES_NONCE_SIZE; ividx++) {
			num_found = sscanf(ivPtr, "%2X", &byteVal);

			if (num_found != 1 || byteVal > 0xFF) {
				status = COMM_INVALID_PARAM;
				//break; //
				return status;
			}
			iv_bytes[ividx] = (uint8_t)byteVal;
		   ivPtr += 2;
		}
	}
    return status;

}


static int parse_auth(const char* cmd, uint8_t *auth_bytes){

	int status = 0x00;
	char cmdStr[] = "set_auth ";
    int length = strlen(cmd);
    int expected_length = strlen(cmdStr) + 2*AES_AUTH_SIZE;

    if (length != expected_length) {
    	status = -1;
    }else{

		const char* macPtr = cmd + strlen(cmdStr);

		int num_found;
		int byteVal;
		for (int aidx = 0; aidx < AES_AUTH_SIZE; aidx++) {
			num_found = sscanf(macPtr, "%2X", &byteVal);

			if (num_found != 1 || byteVal > 0xFF) {
				status = COMM_INVALID_PARAM;;
				//break; //
				return status;
			}

			auth_bytes[aidx] = (uint8_t)byteVal;
			macPtr += 2;
		}

    }
    return status;

}

