/*****************************************************************************
File Name:    app_common.h
Description:  
Note: 
History:	
Date                Author                   Description
2019-10-24         Lucien                     Create
****************************************************************************/
#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

#include "bluenrg_x_device.h"
#include "cassert.h"


/* */
typedef enum
{
	MASTER_STATE_INIT = 0x00,
	MASTER_STATE_CONNECTING,
	MASTER_STATE_CONNECTED,
}ENUM_MASTER_STATE;

typedef enum{
	SLAVE_DISCONNECTED,
	SLAVE_CONNECTED,
}ENUM_SLAVE_STATE;

typedef enum
{
		TASK_STATE_NONE = 0x00,
		TASK_STATE_DOING,
		TASK_STATE_DONE,
}ENUM_TASK_STATE;


typedef struct _task_tag
{
		ENUM_TASK_STATE sta;
		uint16_t retry_times;
}task_t;


typedef struct _con_param_tag
{
	 uint8_t identifier;
   uint16_t L2CAP_Length;
   uint16_t interval_Min;
   uint16_t interval_Max;
   uint16_t slave_Latency;
   uint16_t timeout_Multiplier;
}con_param_t;


/* local role for master */
struct master_t{
	task_t ccc;									// client characteristic configuration
	task_t ds_TX;   						// discover  tx handle
	task_t ds_RX;   						// discover  rx handle
	task_t pair;								// pair
	task_t do_exchange_cfg;			// MTU exchange
	
	task_t con_req;							// connection param update request
	
	ENUM_MASTER_STATE state;		
	uint32_t con_timestamp;			// start connect timestamp
	uint16_t service_handle;
	uint16_t TX_handle;
	uint16_t RX_handle;
	uint8_t  gatt_proc_flag;		// 0x00: free   0x01:busy
	con_param_t con;				
	
};




/* local role for slave */
struct slave_t{
	task_t pair;								// pair
	task_t con_para_update;			// connection_parameter_update_req
	BOOL ccc;										// client characteristic configuration
	uint32_t con_timestamp;			// start connect timestamp
};




//server UUID
#define SPS_UUID   { 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E }
//server RX UUID
#define RX_UUID 		{ 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E }
//server TX UUID
#define TX_UUID 		{ 0x79, 0x41, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E }


#define CE_LENGTH_MS               (10)


/* ifdef MASK_SECURITY  , The security will be mask */
#define MASK_SECURITY	

// 如果使能这个宏 则比较慢连接  ，如果不使能 则可以同时连接多个
//#define STATBLE_CONNECT_PRO_CFG

#define COPY_UUID_16(uuid_struct, uuid_1, uuid_0) \
do {\
  uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; \
}while(0)


#define MAC_ADDR(x)  x[0],x[1],x[2],x[3],x[4],x[5]


#define PRINT_INT(x)    ((int)(x))
#define PRINT_FLOAT(x)  (x>0)? ((int) (((x) - PRINT_INT(x)) * 1000)) : (-1*(((int) (((x) - PRINT_INT(x)) * 1000))))


//#define PRINT_ADDDRESS(a)   PRINTF("0x%02X%02X%02X%02X%02X%02X \n", a[5], a[4], a[3], a[2], a[1], a[0])


void app_alive_tick(void);

void test_write_data_tick(uint16_t connection_handle,uint16_t handle);
void test_notify_tick(uint16_t connection_handle,uint16_t service_handle,uint16_t char_handle);
void print_arr(uint8_t *pdata,uint16_t length);
void print_arr_short(uint8_t *pdata,uint16_t length);





#endif
