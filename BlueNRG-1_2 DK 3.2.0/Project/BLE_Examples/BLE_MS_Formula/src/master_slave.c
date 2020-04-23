/*
  ******************************************************************************
  * @file    master_slave.c
  * @author  AMS - RF Application Team
  * @version V1.1.0
  * @date    28 - September - 2018
  * @brief   Application functions x Master_Slave device using multiple connection
  *          formula 
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2017 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 
  
/** \cond DOXYGEN_SHOULD_SKIP_THIS
 */ 
 
/* Includes-----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "gp_timer.h"
#include "SDK_EVAL_Config.h"
#include "OTA_btl.h"
#include "master_slave.h"
#include "osal.h"
#include <math.h>
#include "ble_utils.h"
#include "app_common.h"
#include "device_list.h"


#ifndef DEBUG
#define DEBUG 1 //TBR
#endif
   
#if DEBUG
#include <stdio.h>
#define PRINTF(...) COMPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
   
#define SLAVES_LOCAL_NAME_LEN 6 
#define MAX_SLAVES_NUMBER 8 
   
#define COPY_UUID_16(uuid_struct, uuid_1, uuid_0) \
do {\
  uuid_struct[0] = uuid_0; uuid_struct[1] = uuid_1; \
}while(0)

#define SUPERVISION_TIMEOUT 5000

/* SDK 3.0.0 or later: BlueNRG-2, BLE stack v2.1x (extended data length is supported) */
#ifdef BLUENRG2_DEVICE 

/* Charactestic length */
#define CHAR_LEN (DEFAULT_MAX_ATT_MTU - 3)

/* Extended data length parameters */
#define TX_OCTECTS  (DEFAULT_MAX_ATT_MTU + 4)
#define TX_TIME     ((TX_OCTECTS +14)*8)

#else 
/* Charactestic length */
#define CHAR_LEN (DEFAULT_ATT_MTU - 3)
#endif



struct master_slave_t{
	ENUM_TASK_STATE 						adv;										
	ENUM_TASK_STATE 						scan;  
};

/* discovery procedure mode context */
typedef struct discoveryContext_s {
  uint8_t device_found_address_type;
  uint8_t device_found_address[6];
  uint16_t connection_handle; 
} discoveryContext_t;

/* Private variables ---------------------------------------------------------*/
static uint16_t tx_handle;
static uint16_t rx_handle;
static uint16_t service_handle;

/* Slaves local names */
static uint8_t slaves_local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'s','l','a','v','e','m'}; 

/**
* @brief Input parameters x Multiple connection formula:
* Default values are used if not defined
*/
uint8_t num_masters = MASTER_SLAVE_NUM_MASTERS; 
uint8_t num_slaves = MASTER_SLAVE_NUM_SLAVES;
float scan_window = MASTER_SLAVE_SCAN_WINDOW;
float sleep_time = MASTER_SLAVE_SLEEP_TIME;


static struct master_slave_t s_state;

static void master_slave_init(void)
{
		s_state.adv = TASK_STATE_NONE;
		s_state.scan = TASK_STATE_NONE;
}


/******************************************************************************
 * Function Name  : Print_Anchor_Period.
 * Description    : Print Device Ancor Period
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/  
void Print_Anchor_Period(void)
{
  uint8_t Status; 
  uint32_t Anchor_Period;
  uint32_t Max_Free_Slot;
  
  Status =  aci_hal_get_anchor_period(&Anchor_Period,
                                      &Max_Free_Slot);
  if (Status == 0)
  {
    PRINTF("Anchor Period = %d.%d ms, Max Free Slot = %d.%d ms\r\n", PRINT_INT(Anchor_Period*0.625),PRINT_FLOAT(Anchor_Period*0.625) , PRINT_INT(Max_Free_Slot*0.625),PRINT_FLOAT(Max_Free_Slot*0.625) );
  }
}
          
/******************************************************************************
 * Function Name  : device_initialization.
 * Description    : Init a BlueNRG device.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void device_initialization(void)
{
  uint16_t gap_service_handle;
  uint16_t dev_name_char_handle;
  uint16_t appearance_char_handle;
  uint8_t status = BLE_STATUS_SUCCESS;

  uint8_t master_slave_address[] = {0xAA,0xAA,0x00,0xE1,0x80,0x02};
  
  uint8_t device_name[] = {0x61,0x64,0x76,0x73,0x63,0x61,0x6E};

  PRINTF("************** Master_Slave device \r\n");
  
  //aci_hal_write_config_data
  status = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN,master_slave_address);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_hal_write_config_data() failed:0x%02x\r\n", status);

    return;
  }else{
    PRINTF("aci_hal_write_config_data --> SUCCESS\r\n");
  }

  // aci_hal_set_tx_power_level
  status = aci_hal_set_tx_power_level(0x01,0x04);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_hal_set_tx_power_level() failed:0x%02x\r\n", status);
    return;
  }else{
    PRINTF("aci_hal_set_tx_power_level --> SUCCESS\r\n");
  }

  // aci_gatt_init
  status = aci_gatt_init();
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_init() failed:0x%02x\r\n", status);
    return;
  }else{
    PRINTF("aci_gatt_init --> SUCCESS\r\n");
  }

  // aci_gap_init
  status = aci_gap_init(GAP_PERIPHERAL_ROLE|GAP_CENTRAL_ROLE,0x00,0x07, &gap_service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gap_init() failed:0x%02x\r\n", status);
    return;
  }else{
    PRINTF("aci_gap_init --> SUCCESS\r\n");
  }

  status = aci_gatt_update_char_value_ext(0,gap_service_handle,dev_name_char_handle,0,0x07,0x00,0x07,device_name); 
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_update_char_value_ext() failed:0x%02x\r\n", status);
    return;
  }else{
    PRINTF("aci_gatt_update_char_value_ext() --> SUCCESS\r\n");
  }
  
  status = hci_le_set_scan_response_data(0x00,NULL);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("hci_le_set_scan_response_data() failed:0x%02x\r\n", status);
  }else{
    PRINTF("hci_le_set_scan_response_data --> SUCCESS\r\n");
  }
  
   /* ********************** Define Master_Slave database ************************* */
if (num_masters >0)
  {
    //Add database
    set_database(); 
  }
  
  /* Extended data length x supporting  increased ATT_MTU size on a single LL PDU packet (BlueNRG-2, BLE stack v2.1 or later) */
#ifdef BLUENRG2_DEVICE  /* NOTE: SDK 3.0.0 or later includes BLE stack v2.1x which supports BlueNRG-2, extended data length */
    status = hci_le_write_suggested_default_data_length(TX_OCTECTS, TX_TIME);
    if (status != BLE_STATUS_SUCCESS) {
      PRINTF("hci_le_write_suggested_default_data_length() failed:0x%02x\r\n", status);
    }else{
      PRINTF("hci_le_write_suggested_default_data_length() --> SUCCESS\r\n");
    }
#endif 
          
  init_multiple_connection_parameters();

	device_init(tx_handle,rx_handle,service_handle);
	
  master_slave_init();
	
#if DEBUG ==1 
  
  PRINTF("****** Input Connection Parameters *******************************\r\n");
  PRINTF("\r\n");
  PRINTF("Num of Masters: %d\r\n", num_masters);
  PRINTF("Num of Slaves: %d\r\n", num_slaves);
  PRINTF("Minimal Scan Window: %d.%d ms\r\n", PRINT_INT(scan_window), PRINT_FLOAT(scan_window));
  PRINTF("Sleep time: %d.%d ms\r\n", PRINT_INT(sleep_time), PRINT_FLOAT(sleep_time));
  PRINTF("\r\n");
  //PRINTF("Connection bandwidth: %d.%d \r\n", PRINT_INT((1000*PACKETS_PER_CI*20*8)/Connection_Interval_ms),PRINT_FLOAT((1000*PACKETS_PER_CI*20*8)/Connection_Interval_ms) );
  PRINTF("******************************************************************\r\n");
#endif 

} /* end device_initialization() */


/******************************************************************************
 * Function Name  : set_database.
 * Description    : Create a GATT DATABASE.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void set_database(void)
{
  uint8_t status = BLE_STATUS_SUCCESS;
	Service_UUID_t service_uuid;
	Char_UUID_t tx_uuid;
	Char_UUID_t rx_uuid;

  uint8_t server_uuid[16] = SPS_UUID;

	Osal_MemCpy(&service_uuid.Service_UUID_128, server_uuid, 16);
  //aci_gatt_add_service
  //status = aci_gatt_add_service(service_uuid_type,service_uuid_type,service_type,max_attribute_records, &service_handle);
  status = aci_gatt_add_service(UUID_TYPE_128,&service_uuid,PRIMARY_SERVICE,0x06, &service_handle);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_service() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_service --> SUCCESS\r\n");
  }

  uint8_t uuid1[16] = TX_UUID;
	Osal_MemCpy(&tx_uuid.Char_UUID_128, uuid1, 16);
  status = aci_gatt_add_char(service_handle,UUID_TYPE_128,&tx_uuid,APP_MAX_ATT_SIZE,CHAR_PROP_NOTIFY,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,
								0x07,0x01, &tx_handle);
	if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_char() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_char --> SUCCESS\r\n");
  }

	uint8_t uuid2[16] = RX_UUID;
	Osal_MemCpy(&rx_uuid.Char_UUID_128, uuid2, 16);
  status = aci_gatt_add_char(service_handle,UUID_TYPE_128,&rx_uuid,APP_MAX_ATT_SIZE,CHAR_PROP_WRITE_WITHOUT_RESP,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,
								0x07,0x01, &rx_handle);

  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_char() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_char --> SUCCESS\r\n");
  }
	

	
  
}


/******************************************************************************
 * Function Name  : set_device_discoverable.
 * Description    : Puts the device in connectable mode.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
uint8_t set_device_discoverable(void)
{
  uint8_t status;
  uint8_t Local_Name[] = {AD_TYPE_COMPLETE_LOCAL_NAME, 0x61, 0x64, 0x76, 0x73, 0x63, 0x61,0x6e}; //advscan {0x61,0x64,0x76,0x73,0x63,0x61,0x6E};

	const Multiple_Connection_type*  pMulCon = get_multiple_connection_parameters();
  Print_Anchor_Period(); 
	status = aci_gap_set_discoverable(ADV_IND,
                                    pMulCon->Advertising_Interval,
                                    pMulCon->Advertising_Interval,
                                    0x00,
                                    NO_WHITE_LIST_USE,
                                    sizeof(Local_Name),//0x08
                                    Local_Name,
                                    0x00,
                                    NULL,
                                    0x0000,
                                    0x0000);
	return status;
}


/******************************************************************************
 * Function Name  : device_scanning.
 * Description    : Puts the device in scannable mode.
 * Input          : None.
 * Output         : None.
 * Return         : Status code.
******************************************************************************/
uint8_t device_scanning(void)
{
  uint8_t status;

	const Multiple_Connection_type*  pMulCon = get_multiple_connection_parameters();
	status = aci_gap_start_general_discovery_proc(pMulCon->Scan_Interval,
                                                pMulCon->Scan_Window,
                                                0x00,
                                                0x00);
  return (status);
}


static void APP_master_slave_adv_scan_control(ENUM_TASK_STATE target_adv,ENUM_TASK_STATE target_scan)
{
		tBleStatus status;
		if(target_adv != s_state.adv){
				if(target_adv == TASK_STATE_NONE){
						status = aci_gap_set_non_discoverable();
						PRINTF("stop adv status:%x \n", status);
						s_state.adv = TASK_STATE_NONE;
				}
				else {
						status = set_device_discoverable();
						PRINTF("start discoverable status:%x \n", status);
						s_state.adv = ((status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE);
				}
		}

		if(target_scan != s_state.scan){
				if( (target_scan == TASK_STATE_NONE)){
						status = aci_gap_terminate_gap_proc(0x02);
						PRINTF("stop scan status:%x \n", status);
						s_state.scan = TASK_STATE_NONE;
				}
				else {
						status = device_scanning();
						PRINTF("start scan status:%x \n", status);
						s_state.scan = ((status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE);
				}
		}
}


static void APP_master_slave_auto_schedule(void)
{
		BOOL busy = !device_connected_task_all_done();
		BOOL slaves_full = device_slaves_is_full();
		BOOL masters_full = device_masters_is_full();
		
		ENUM_TASK_STATE target_adv = TASK_STATE_NONE;
		ENUM_TASK_STATE target_scan = TASK_STATE_NONE;
		
		if(!busy){
				target_adv = (masters_full? TASK_STATE_NONE:TASK_STATE_DOING);
				target_scan = (slaves_full? TASK_STATE_NONE:TASK_STATE_DOING); 
		}

		APP_master_slave_adv_scan_control(target_adv,target_scan);
}

/* this even is generated by lower layer for specific BLE state environment . 
eg. when connecting, It need to stop adv and scan 				*/
static void *app_envi_manual_schedule_even(tBleStatus sta,void* param)
{
		ENUM_TASK_STATE target_adv = TASK_STATE_NONE;
		ENUM_TASK_STATE target_scan = TASK_STATE_NONE;
		PRINTF("%s\n",__func__);
		
		APP_master_slave_adv_scan_control(target_adv,target_scan);
		return NULL;
}


/******************************************************************************
 * Function Name  : app_tick.
 * Description    : Device Demo state machine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void APP_Tick(void)
{
	app_alive_tick();
	APP_master_slave_auto_schedule();
	device_Tick(app_envi_manual_schedule_even);
	
} /* end APP_Tick() */
/* *************** BlueNRG-1 Stack Callbacks****************/


/*******************************************************************************
* Function Name  : Find_DeviceName.
* Description    : Extracts the device name.
* Input          : Data length.
*                  Data value
* Return         : TRUE if the local name found is the expected one, FALSE otherwise.
*******************************************************************************/
static uint8_t Find_DeviceName(uint8_t data_length, uint8_t *data_value)
{
  uint8_t index = 0;
  
  while (index < data_length) {
    /* Advertising data fields: len, type, values */
    /* Check if field is complete local name and the lenght is the expected one for BLE NEW Chat  */
    if (data_value[index+1] == AD_TYPE_COMPLETE_LOCAL_NAME) { 
      /* check if found device name is the expected one: local_name */ 
      if (memcmp(&data_value[index+1], &slaves_local_name[0], SLAVES_LOCAL_NAME_LEN) == 0)
        return TRUE;
      else
        return FALSE;
    } else {
      /* move to next advertising field */
      index += (data_value[index] +1); 
    }
  }
  
  return FALSE;
}


/******************************************************************************
 * Function Name  : hci_le_advertising_report_event.
 * Description    : The LE Advertising Report event indicates that a Bluetooth device or multiple
Bluetooth devices have responded to an active scan or received some information
during a passive scan. The Controller may queue these advertising reports
and send information from multiple devices in one LE Advertising Report event..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void hci_le_advertising_report_event(uint8_t num_reports,Advertising_Report_t advertising_report[])
{
  //PRINTF("hci_le_advertising_report_event --> EVENT\r\n");
  
  /* Advertising_Report contains all the expected parameters */
  uint8_t evt_type = advertising_report[0].Event_Type ;
  uint8_t data_length = advertising_report[0].Length_Data;
  uint8_t bdaddr[6];

  Osal_MemCpy(bdaddr, advertising_report[0].Address,6);
      
  /*  check current found device */
  if ((evt_type == ADV_IND) && Find_DeviceName(data_length, advertising_report[0].Data) )
  {
     device_slaves_update(advertising_report[0].Address,advertising_report[0].Address_Type);	
  }
     
}/* end hci_le_advertising_report_event() */


/******************************************************************************
 * Function Name  : aci_gap_proc_complete_event.
 * Description    : This event is sent by the GAP to the upper layers when a procedure previously started has
been terminated by the upper layer or has completed for any other reason.
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void aci_gap_proc_complete_event(uint8_t procedure_code,uint8_t status,uint8_t data_length,uint8_t data[])
{
  PRINTF("procedure_code:%x   status:%x \r\n", procedure_code, status);
  COMPrintf_hexdump(data, data_length);
	// 
	if( (procedure_code|0x02) != 0 ){
			if(s_state.scan == TASK_STATE_DOING)
					s_state.scan = TASK_STATE_DONE;
	}	
}/* end aci_gap_proc_complete_event() */


/******************************************************************************
 * Function Name  : hci_le_connection_complete_event.
 * Description    : The LE Connection Complete event indicates to both of the Hosts forming the
connection that a new connection has been created. Upon the creation of the
connection a Connection_Handle shall be assigned by the Controller, and
passed to the Host in this event. If the connection establishment fails this event
shall be provided to the Host that had issued the LE_Create_Connection command.
This event indicates to the Host which issued a LE_Create_Connection
command and received a Command Status event if the connection
establishment failed or was successful.
The Master_Clock_Accuracy parameter is only valid for a slave. On a master,
this parameter shall be set to 0x00..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void hci_le_connection_complete_event(uint8_t status,uint16_t connection_handle,uint8_t role,uint8_t peer_address_type,uint8_t peer_address[6],uint16_t conn_interval,uint16_t conn_latency,uint16_t supervision_timeout,uint8_t master_clock_accuracy)
{
 	s_state.adv = TASK_STATE_NONE;
	device_connection_complete_event(0x00,connection_handle,role,peer_address_type,
												peer_address,conn_interval,conn_latency,supervision_timeout);
             
}/* hci_le_connection_complete_event() */


/******************************************************************************
 * Function Name  : aci_gatt_notification_event.
 * Description    : This event is generated when a notification is received from the server..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void aci_gatt_notification_event(uint16_t connection_handle,uint16_t attribute_handle,uint8_t attribute_value_length,uint8_t attribute_value[])
{	
	PRINTF("connection_handle:%x attribute_handle:%x attribute_value_length:%x \n", connection_handle,attribute_handle,attribute_value_length);
	print_arr_short(attribute_value, attribute_value_length);
}

/******************************************************************************
 * Function Name  : hci_disconnection_complete_event.
 * Description    : The Disconnection Complete event occurs when a connection is terminated.
The status parameter indicates if the disconnection was successful or not. The
reason parameter indicates the reason for the disconnection if the disconnection
was successful. If the disconnection was not successful, the value of the
reason parameter can be ignored by the Host. For example, this can be the
case if the Host has issued the Disconnect command and there was a parameter
error, or the command was not presently allowed, or a Connection_Handle
that didn't correspond to a connection was given..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void hci_disconnection_complete_event(uint8_t status,uint16_t connection_handle,uint8_t reason)
{
  PRINTF("---------connection_handle:%x  hci_disconnection_complete_event --> reason 0x%02X -------\r\n", connection_handle, reason);
	device_disconnection_complete_event(connection_handle);
}

/******************************************************************************
 * Function Name  : hci_le_connection_update_complete_event.
 * Description    : The LE Connection Update Complete event is used to indicate that the Controller
process to update the connection has completed.
On a slave, if no connection parameters are updated, then this event shall not be issued.
On a master, this event shall be issued if the Connection_Update command was sent..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void hci_le_connection_update_complete_event(uint8_t Status,
                                             uint16_t Connection_Handle,
                                             uint16_t Conn_Interval,
                                             uint16_t Conn_Latency,
                                             uint16_t Supervision_Timeout)
{
  

}

/**
 * @brief  The LE Data Length Change event notifies the Host of a change to either the maximum Payload length or the maximum transmission time of Data Channel PDUs in either direction. The values reported are the maximum that will actually be used on the connection following the change. 
 * @param  param See file bluenrg1_events.h.
 * @retval See file bluenrg1_events.h.
*/
void hci_le_data_length_change_event(uint16_t connection_handle,uint16_t maxtxoctets,uint16_t maxtxtime,uint16_t maxrxoctets,uint16_t maxrxtime)
{

}

/*******************************************************************************
 * Function Name  : aci_gatt_tx_pool_available_event.
 * Description    : This event occurs when a TX pool available is received.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void aci_gatt_tx_pool_available_event(uint16_t Connection_Handle,
                                      uint16_t Available_Buffers)
{       
  /* It allows to notify when at least 2 GATT TX buffers are available */
} 


/** \endcond 
*/
