/*****************************************************************************
File Name:    device_list.h
Description: 
Note: 
History:	
Date                Author                   Description
2019-10-21         Lucien                     Create
****************************************************************************/
#ifndef __NODE_LIST_H__
#define __NODE_LIST_H__
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "double_list.h"
//#include "system_util.h"
#include "app_common.h"
#include "ble_status.h"


/* incase BLE even mutex that It is need to clean environment */
typedef void *(*APP_ENVI_CLEAR_CALLBACK)(tBleStatus sta,void* param);



void device_connection_complete_event(uint8_t llid, uint16_t connection_handle,uint8_t role,uint8_t peer_address_type,uint8_t peer_address[6],uint16_t conn_interval,uint16_t conn_latency,uint16_t supervision_timeout);


void device_slaves_update(uint8_t peer_address[6],uint8_t Address_Type);


void device_init(uint16_t tx_handle,uint16_t rx_handle,uint16_t svc_handle);


uint16_t device_get_handle(uint8_t llID);

void device_Tick(APP_ENVI_CLEAR_CALLBACK app_envi_cb);

void device_disconnection_complete_event(uint16_t connection_handle);


BOOL device_connected_task_all_done(void);


BOOL device_slaves_is_full(void);

BOOL device_masters_is_full(void);



#endif
