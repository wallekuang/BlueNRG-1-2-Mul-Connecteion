/*****************************************************************************
File Name:    slave.c
Description:  
Note: 
History:	
Date                Author                   Description
2020-04-21         Lucien                     Create
****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bluenrg1_stack.h"
#include "slave.h"
#include "osal.h"
#include "app_common.h"
#include "bluenrg1_gap.h"
#include "bluenrg1_gatt_server.h"
#include "bluenrg1_hal.h"


#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG
#include <stdio.h>
#define PRINTF(...) COMPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


struct device_connected_t {
		struct slave_t slave;
		uint8_t mac[6];
		uint16_t connection_handle;
		uint16_t svc_handle;
		uint16_t tx_handle;
		uint16_t rx_handle;
		uint16_t update_len;
};

/**************************************************************************** */


#define  TX_OCTECTS  ((DEFAULT_MAX_ATT_MTU - 3) + 4)  //+4  for L2CAP header 
#define  TX_TIME    ((TX_OCTECTS +14)*8) //Don't modify it `


#ifndef SLAVE_INDEX
#define SLAVE_INDEX 1
#endif 


static uint8_t adv_data[] = {0x02,AD_TYPE_FLAGS, FLAG_BIT_LE_GENERAL_DISCOVERABLE_MODE|FLAG_BIT_BR_EDR_NOT_SUPPORTED,
                             7, AD_TYPE_COMPLETE_LOCAL_NAME,0x73,0x6C,0x61,0x76,0x65,0x6D}; //slavem 

static struct device_connected_t s_ins;
static ENUM_SLAVE_STATE s_ble_state = SLAVE_DISCONNECTED;


/**
 * @brief  Create a GATT DATABASE
 * @param  None.
 * @retval None.
*/
void set_database(void)
{
  uint8_t status;

	Service_UUID_t service_uuid;
	Char_UUID_t tx_uuid;
	Char_UUID_t rx_uuid;
	
	uint8_t server_uuid[16] = SPS_UUID;

  //aci_gatt_add_service
  //status = aci_gatt_add_service(service_uuid_type,service_uuid_16,service_type,max_attribute_records, &service_handle);
  // IMPORTANT: Please make sure the Max_Attribute_Records parameter is configured exactly with the required value.
	Osal_MemCpy(&service_uuid.Service_UUID_128, server_uuid, 16);
  status = aci_gatt_add_service(UUID_TYPE_128,&service_uuid,PRIMARY_SERVICE,0x06, &s_ins.svc_handle);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_service() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_service --> SUCCESS\r\n");
  }

	uint8_t uuid1[16] = TX_UUID;
	Osal_MemCpy(&tx_uuid.Char_UUID_128, uuid1, 16);

  status = aci_gatt_add_char(s_ins.svc_handle,UUID_TYPE_128,&tx_uuid,(DEFAULT_MAX_ATT_MTU - 3),CHAR_PROP_NOTIFY,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,
								0x07,0x01, &s_ins.tx_handle);
	
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_char() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_char --> SUCCESS\r\n");
  }
 #ifdef BLUENRG2_DEVICE
    status = hci_le_write_suggested_default_data_length(TX_OCTECTS,  TX_TIME);
    if (status != BLE_STATUS_SUCCESS) 
    {
      PRINTF("hci_le_write_suggested_default_data_length() failed:0x%02x\r\n", status);
    }
    else
    {
      PRINTF("hci_le_write_suggested_default_data_length --> SUCCESS\r\n");
    }
#endif

	uint8_t uuid2[16] = RX_UUID;
	Osal_MemCpy(&rx_uuid.Char_UUID_128, uuid2, 16);
  status = aci_gatt_add_char(s_ins.svc_handle,UUID_TYPE_128,&rx_uuid,(DEFAULT_MAX_ATT_MTU - 3),CHAR_PROP_WRITE_WITHOUT_RESP,ATTR_PERMISSION_NONE,GATT_NOTIFY_ATTRIBUTE_WRITE,
								0x07,0x01, &s_ins.rx_handle);

  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_add_char() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_add_char --> SUCCESS\r\n");
  }
}

#ifndef MASK_SECURITY
static void Clear_Security_Database(void)
{
  uint8_t ret; 
  
  /* ACI_GAP_CLEAR_SECURITY_DB*/
  ret = aci_gap_clear_security_db();
  if (ret != BLE_STATUS_SUCCESS) 
  {
    PRINTF("aci_gap_clear_security_db() failed:0x%02x\r\n", ret);
  }
  else
  {
    PRINTF("aci_gap_clear_security_db() --> SUCCESS\r\n");
  }
}

static void security_init(void)
{
		Clear_Security_Database();
		uint8_t ret = aci_gap_set_io_capability(IO_CAP_NO_INPUT_NO_OUTPUT);
		if (ret != BLE_STATUS_SUCCESS) {
    	PRINTF("aci_gap_set_io_capability(%d) failed:0x%02x\r\n", IO_CAP_NO_INPUT_NO_OUTPUT, ret);
	  }
	  else
	  {
	    PRINTF("aci_gap_set_io_capability(%d) --> SUCCESS\r\n",IO_CAP_NO_INPUT_NO_OUTPUT);
	  }
	 
	  /* BLE Security v4.2 is supported: BLE stack FW version >= 2.x */  
	  ret = aci_gap_set_authentication_requirement(NO_BONDING,
	                                               MITM_PROTECTION_NOT_REQUIRED,
	                                               SC_IS_SUPPORTED,
	                                               KEYPRESS_IS_SUPPORTED,
	                                               0x07, 
	                                               0x10,
	                                               DONOT_USE_FIXED_PIN_FOR_PAIRING,
	                                               123456);
	  
	  if(ret != BLE_STATUS_SUCCESS) {
	    PRINTF("aci_gap_set_authentication_requirement() failed: 0x%02x\r\n", ret);
	  }  
	  else
	  {
	    PRINTF("aci_gap_set_authentication_requirement() --> SUCCESS\r\n");
	  }
	
}
#endif

/**
 * @brief  Puts the device in connectable mode
 * @param  None.
 * @retval None.
*/
static void set_device_discoverable(void)
{
  uint8_t status;
	status = hci_le_set_advertise_enable(TRUE);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gap_set_advertising_enable() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gap_set_advertising_enable() --> SUCCESS\r\n");
  }
}

/**
 * @brief  Init a BlueNRG device
 * @param  None.
 * @retval None.
*/
void device_initialization(void)
{
  uint16_t service_handle;
  uint16_t dev_name_char_handle;
  uint16_t appearance_char_handle;
  uint8_t status;
  
  uint8_t device_name[] = {0x73,0x6C,0x61,0x76,0x65,0x6D}; //slavem
  
   PRINTF("************** Slave: %d\r\n", SLAVE_INDEX);


	// read Unique device serial number
	uint8_t chip_id[8];
	uint32_t* pChip_id = (uint32_t*)0x100007F4;
	memcpy(chip_id,pChip_id,sizeof(uint32_t));
	pChip_id = (uint32_t*)0x100007F8;
	memcpy(chip_id+sizeof(uint32_t),pChip_id,sizeof(uint32_t));



	// check If it is all 0xff
	uint8_t none[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if(memcmp(none,chip_id,sizeof(none)) == 0){
			hci_le_rand(chip_id);
	}

	for(int i=0;i<sizeof(chip_id);i++)
	{
			PRINTF(" %x ",chip_id[i]);
	}
	PRINTF("\n");
	

  //status = aci_hal_write_config_data(0x2E,0x06,slave_address[SLAVE_INDEX-1]);
  status = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET,0x06,chip_id);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_hal_write_config_data() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_hal_write_config_data --> SUCCESS\r\n");
  }

  //aci_hal_set_tx_power_level
  //status = aci_hal_set_tx_power_level(en_high_power,pa_level);
  status = aci_hal_set_tx_power_level(0,25);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_hal_set_tx_power_level() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_hal_set_tx_power_level --> SUCCESS\r\n");
  }

  //aci_gatt_init
  //status = aci_gatt_init();
  status = aci_gatt_init();
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_init() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_init --> SUCCESS\r\n");
  }

  //aci_gap_init
  status = aci_gap_init(GAP_PERIPHERAL_ROLE,0x00,sizeof(device_name), &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gap_init() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gap_init --> SUCCESS\r\n");
  }
  
  //aci_gatt_update_char_value_ext
  //status = aci_gatt_update_char_value_ext(conn_handle_to_notify,service_handle,char_handle,update_type,char_length,value_offset,value_length,value);
  status = aci_gatt_update_char_value_ext(0x0000,0x0005,0x0006,0x00,sizeof(device_name),0x0000,sizeof(device_name),device_name);
  if (status != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_update_char_value_ext() failed:0x%02x\r\n", status);
  }else{
    PRINTF("aci_gatt_update_char_value_ext --> SUCCESS\r\n");
  }
  
  //Add database
  set_database(); 
	status = aci_gap_set_discoverable(ADV_IND,0x0100,0x0100,0x00,NO_WHITE_LIST_USE,0x00,NULL,0x00,NULL,0x0006,0x0008);
  

  PRINTF("Advertising configuration %02X\n", status);
  
  status = hci_le_set_advertising_data(sizeof(adv_data), adv_data);
  
  PRINTF("Set advertising data %02X\n", status);  
#ifndef MASK_SECURITY
	security_init();
#endif
	//set_device_discoverable();
}



/**
 * @brief  Device Demo state machine
 * @param  None.
 * @retval None.
*/
void APP_Tick(void)
{
  uint8_t status;
	
	app_alive_tick();
	
	if(s_ble_state != SLAVE_CONNECTED){
			return;
	}

#if 0
	if( (s_ins.slave.con_para_update.sta == TASK_STATE_NONE)){
		  //  
		  PRINTF("aci_l2cap_connection_parameter_update_req \n");
			status = aci_l2cap_connection_parameter_update_req(s_ins.connection_handle,24,24,0,500);
			s_ins.slave.con_para_update.sta = (status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE;
			s_ins.slave.con_para_update.retry_times++;
			PRINTF("status:%x \n", status);
	}
#endif

	if( s_ins.slave.ccc){
			test_notify_tick(s_ins.connection_handle, s_ins.svc_handle , s_ins.tx_handle);
	}

#ifndef MASK_SECURITY
	if(s_ins.slave.pair.sta == TASK_STATE_NONE && (s_ins.slave.con_para_update.sta == TASK_STATE_DONE)){
				status = aci_gap_send_pairing_req(s_ins.connection_handle,FALSE);
				PRINTF("aci_gap_slave_security_req status:%x \n", status);
				s_ins.slave.pair.sta = (status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE;
				s_ins.slave.pair.retry_times++;
	}
#endif
    
}

/* *************** BlueNRG-1 Stack Callbacks****************/

/**
 * @brief  The LE Connection Complete event indicates to both of the Hosts forming the
connection that a new connection has been created. Upon the creation of the
connection a Connection_Handle shall be assigned by the Controller, and
passed to the Host in this event. If the connection establishment fails this event
shall be provided to the Host that had issued the LE_Create_Connection command.
This event indicates to the Host which issued a LE_Create_Connection
command and received a Command Status event if the connection
establishment failed or was successful.
The Master_Clock_Accuracy parameter is only valid for a slave. On a master,
this parameter shall be set to 0x00.
 * @param  param See file bluenrg1_events.h.
 * @retval See file bluenrg1_events.h.
*/
void hci_le_connection_complete_event(uint8_t status,uint16_t connection_handle,uint8_t role,uint8_t peer_address_type,uint8_t peer_address[6],uint16_t conn_interval,uint16_t conn_latency,uint16_t supervision_timeout,uint8_t master_clock_accuracy)
{
  //USER ACTION IS NEEDED
  PRINTF("hci_le_connection_complete_event --> EVENT: 0x%04x\r\n", connection_handle);
  s_ins.connection_handle = connection_handle;
	s_ble_state = SLAVE_CONNECTED;
}

/*******************************************************************************
 * Function Name  : hci_le_enhanced_connection_complete_event.
 * Description    : This event indicates that a new connection has been created
 * Input          : See file bluenrg_lp_events.h
 * Output         : See file bluenrg_lp_events.h
 * Return         : See file bluenrg_lp_events.h
 *******************************************************************************/
void hci_le_enhanced_connection_complete_event(uint8_t Status,
                                               uint16_t Connection_Handle,
                                               uint8_t Role,
                                               uint8_t Peer_Address_Type,
                                               uint8_t Peer_Address[6],
                                               uint8_t Local_Resolvable_Private_Address[6],
                                               uint8_t Peer_Resolvable_Private_Address[6],
                                               uint16_t Conn_Interval,
                                               uint16_t Conn_Latency,
                                               uint16_t Supervision_Timeout,
                                               uint8_t Master_Clock_Accuracy)
{
  
  hci_le_connection_complete_event(Status,
                                   Connection_Handle,
                                   Role,
                                   Peer_Address_Type,
                                   Peer_Address,
                                   Conn_Interval,
                                   Conn_Latency,
                                   Supervision_Timeout,
                                   Master_Clock_Accuracy);
}


void aci_gap_pairing_complete_event(uint16_t Connection_Handle,
																	 uint8_t Status,
																	 uint8_t Reason)
{
		if(Status == BLE_STATUS_SUCCESS)
				s_ins.slave.pair.sta = TASK_STATE_DONE;
		else
				s_ins.slave.pair.sta = TASK_STATE_NONE;

		PRINTF("%s  status:%x reason:%x \n", __func__,Status,Reason);
}

/**
 * @brief  The Disconnection Complete event occurs when a connection is terminated.
The status parameter indicates if the disconnection was successful or not. The
reason parameter indicates the reason for the disconnection if the disconnection
was successful. If the disconnection was not successful, the value of the
reason parameter can be ignored by the Host. For example, this can be the
case if the Host has issued the Disconnect command and there was a parameter
error, or the command was not presently allowed, or a Connection_Handle
that didn't correspond to a connection was given.
 * @param  param See file bluenrg1_events.h.
 * @retval See file bluenrg1_events.h.
*/
void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason)
{
	// clear all s_slave state
	memset(&s_ins.slave,0,sizeof(s_ins.slave));
	s_ins.connection_handle = 0;
	s_ins.update_len = DEFAULT_ATT_MTU-3;

	
	s_ble_state = SLAVE_DISCONNECTED;
	
 	set_device_discoverable();
#ifndef MASK_SECURITY	
	Clear_Security_Database();
#endif	
  PRINTF("hci_disconnection_complete_event --> reason 0x%02X\r\n", Reason);
}
  
/**
 * @brief  This event is generated to the application by the GATT server when a client modifies any
attribute on the server, as consequence of one of the following GATT procedures:
- write without response
- signed write without response
- write characteristic value
- write long characteristic value
- reliable write.
 * @param  param See file bluenrg1_events.h.
 * @retval retVal See file bluenrg1_events.h.
*/
void aci_gatt_attribute_modified_event(uint16_t connection_handle,uint16_t attr_handle,uint16_t offset,uint16_t attr_data_length,uint8_t attr_data[])
{
  if(attr_handle == s_ins.tx_handle + 2)
  {        
    if(attr_data[0] == 0x01){
				s_ins.slave.ccc = TRUE;
      	PRINTF("aci_gatt_attribute_modified_event(): enabled characteristic notification\r\n");
    }
		else{
				s_ins.slave.ccc = FALSE;
				PRINTF("aci_gatt_attribute_modified_event(): disabled characteristic notification\r\n");
		}
  }

	if(attr_handle == s_ins.rx_handle + 1){
			PRINTF("attr_data_length:%x \n",attr_data_length);
			print_arr_short(attr_data, attr_data_length);
	}
}


/**
 * @brief  This event is generated in response to an Exchange MTU request. See
@ref ACI_GATT_EXCHANGE_CONFIG.
 * @param  param See file bluenrg1_events.h.
 * @retval retVal See file bluenrg1_events.h.
*/
void aci_att_exchange_mtu_resp_event(uint16_t connection_handle,uint16_t server_rx_mtu)
{
  //USER ACTION IS NEEDED
  s_ins.update_len = server_rx_mtu-3;
  PRINTF("aci_att_exchange_mtu_resp_event(%d) --> EVENT\r\n",server_rx_mtu);
}


/**
 * @brief  This event is generated when a GATT client procedure completes either with error or
successfully.
 * @param  param See file bluenrg1_events.h.
 * @retval retVal See file bluenrg1_events.h.
*/
void aci_gatt_proc_complete_event(uint16_t connection_handle,uint8_t error_code)
{
  //USER ACTION IS NEEDED
  PRINTF("aci_gatt_proc_complete_event --> EVENT\r\n");
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

void aci_l2cap_connection_update_resp_event(uint16_t Connection_Handle,
                                            uint16_t Result)
{
		PRINTF("%s Result:%d \n", __func__, Result);
		if(Result == BLE_STATUS_SUCCESS){
				s_ins.slave.con_para_update.sta = TASK_STATE_DONE;
		}
		else{
				s_ins.slave.con_para_update.sta = TASK_STATE_NONE;
		}
}
/** \endcond 
*/
