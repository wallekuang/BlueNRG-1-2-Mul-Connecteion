/*****************************************************************************
File Name:    app_common.c
Description:  
Note: 
History:	
Date                Author                   Description
2019-10-28         Lucien                     Create
****************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_common.h"
#include "bluenrg1_stack.h"



#ifndef DEBUG
#define DEBUG 1 //TBR
#endif


#if DEBUG
#define PRINTF(...) COMPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


/**
	* @brief This function use to detecte the application if it is alive. printf "*" character per 1000ms
	* @param Peer_Identity_Address_Type Identity address type.
	* Values:
	- 0x00: Public Identity Address
	- 0x01: Random (static) Identity Address
	* @param Peer_Identity_Address Public or Random (static) Identity address of the peer device
	* @retval Value indicating success or error code.
*/
void app_alive_tick(void)
{
	static uint32_t last_time;
	uint32_t cur_time = HAL_VTimerGetCurrentTime_sysT32();
	uint32_t ms = (uint32_t)(abs(cur_time-last_time)*2.4414/1000);
	if(ms > 1000){
			last_time = cur_time;
			PRINTF("*");
	}
}


/**
	* @brief This function use to test the write  characteristic for clien role to call
	* @param[in] connection_handle: peer device connection handle
		@param[in] handle: the  handle of characteristic want to write. 
	* @retval Value none.
*/
void test_write_data_tick(uint16_t connection_handle,uint16_t handle)
{
	static uint32_t last_time;
	static uint16_t count = 0;
	uint32_t cur_time = HAL_VTimerGetCurrentTime_sysT32();
	uint32_t ms = (uint32_t)(abs(cur_time-last_time)*2.4414/1000);
	if(ms > 1000){
			last_time = cur_time;

			uint8_t temp[APP_MAX_ATT_SIZE];
			memset(temp,0,sizeof(temp));
			
			count++;
			temp[0] = (count>>8)&0xff;
			temp[1] = (count>>0)&0xff;
			
			tBleStatus status = aci_gatt_write_without_resp(connection_handle,handle,APP_MAX_ATT_SIZE,temp);
	
			PRINTF("aci_gatt_write_char_value: status:%x  \n", status);
	}
}


/**
* @brief This function use to test the notify  characteristic for server role to call
* @param[in] service_handle: the server handle 
* @param[in] connection_handle: peer device connection handle
*	@param[in] handle: the  handle of characteristic want to write. 
* @retval Value none.
*/
void test_notify_tick(uint16_t connection_handle,uint16_t service_handle,uint16_t char_handle)
{
	static uint32_t last_time;
	static uint16_t count = 0;
	uint32_t cur_time = HAL_VTimerGetCurrentTime_sysT32();
	uint32_t ms = (uint32_t)(abs(cur_time-last_time)*2.4414/1000);
	if(ms > 2000){
			last_time = cur_time;
			uint16_t updata_len = (DEFAULT_MAX_ATT_MTU - 3);
			uint8_t temp[(DEFAULT_MAX_ATT_MTU - 3)];
			memset(temp,0,sizeof(temp));
			
			count++;
			temp[0] = (count>>8)&0xff;
			temp[1] = (count>>0)&0xff;
			
			//PRINTF("connection_handle:%x service_handle:%x char_handle:%x ",connection_handle, service_handle, char_handle);
			tBleStatus status  = aci_gatt_update_char_value_ext(connection_handle,service_handle,char_handle,0x01,(DEFAULT_MAX_ATT_MTU - 3) ,0x0000,updata_len,temp);
			PRINTF("notify status:%x  count:%x \n", status, count);
	}
		
}


void print_arr(uint8_t *pdata,uint16_t length)
{
		for(int i=0;i<length;i++){
				PRINTF("%x ", pdata[i]);
		}
		PRINTF("\n");
}

void print_arr_short(uint8_t *pdata,uint16_t length)
{
	if(length < 8){
			print_arr(pdata,length);
	}
		
	for(int i=0;i<length;i++){
			if(i > 4 && i< length-4){
					continue;
			}
			if(i==4){
					PRINTF("...");
			}
				
			PRINTF("%x ", pdata[i]);
	}
	
	PRINTF("\n");
}






