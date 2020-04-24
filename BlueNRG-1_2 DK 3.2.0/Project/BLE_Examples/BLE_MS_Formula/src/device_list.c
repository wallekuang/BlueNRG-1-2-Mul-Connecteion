/*****************************************************************************
File Name:    device_list.c
Description:  
Note: 
History:	
Date                Author                   Description
2019-10-21         Lucien                     Create
****************************************************************************/
#include "device_list.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "bluenrg1_api.h"
#include "bluenrg1_stack.h"
#include "ble_utils.h"

#include <stdlib.h>

#ifndef DEBUG
#define DEBUG 1
#endif


#if DEBUG
#define PRINTF(...) COMPrintf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#define DEFAULT_TASK_RETRY_TIMES		(400)


struct device_connected_t {
  struct dl_list node;
	uint8_t mac[6];
	uint8_t llID;
	uint8_t address_type;
	uint8_t role_type;
	uint16_t connection_handle;
	uint16_t update_len;
	union{
		struct master_t master;				/* local role for master */
		struct slave_t slave;					/* local role for slave */
	}role;
};


/* The number of master-slave master number + master-slave slave numbler */
#define NODES_NUM (MASTER_SLAVE_NUM_MASTERS+MASTER_SLAVE_NUM_SLAVES)
/* The memmory for */ 
static struct device_connected_t __device_memory[NODES_NUM];

/* this slave mean that the peer divice is slave */
static struct dl_list s_salve_list;
/* this master mean that the peer divice is master */
static struct dl_list s_master_list;
static struct dl_list s_device_unuse;

/* for local as server */
static uint16_t s_tx_handle;
static uint16_t s_rx_handle;
//static uint16_t s_svc_handle;

/**
 * @brief Multiple connection parameters variable
 */
const static Multiple_Connection_type *pMS_ConPara; 


BOOL device_slaves_is_full(void)
{
		return dl_list_len(&s_salve_list) == MASTER_SLAVE_NUM_SLAVES;
}

BOOL device_masters_is_full(void)
{
		return dl_list_len(&s_master_list) == MASTER_SLAVE_NUM_MASTERS;
}

BOOL device_is_full(void)
{
		return dl_list_len(&s_device_unuse) == 0;
}

BOOL device_is_empty(void)
{
		return (dl_list_len(&s_device_unuse) == NODES_NUM);
}


void device_connection_complete_event(uint8_t llID, uint16_t connection_handle,
																uint8_t role,uint8_t peer_address_type,uint8_t peer_address[6],
											uint16_t conn_interval,uint16_t conn_latency,uint16_t supervision_timeout)
{
		// master  Role of the local device in the connection
		if(role == 0x00){
				struct device_connected_t *item;
		    dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
		    		//PRINTF("item = %x \n",item);
		    		if(memcmp(item->mac,peer_address,sizeof(item->mac)) == 0){
								item->address_type = peer_address_type;
								item->llID = llID;
								item->update_len = DEFAULT_ATT_MTU -3;	
								item->role_type = role;
								item->role.master.state = MASTER_STATE_CONNECTED;
								item->connection_handle = connection_handle;
								item->role.master.con_timestamp = HAL_VTimerGetCurrentTime_sysT32();
								PRINTF("item->role.master.state = MASTER_STATE_CONNECTED\n");
								PRINTF("item: %x ",item);
								return;
						}
		    }
		}

		// slave  Role of the local device in the connection
		if(role == 0x01){
			struct device_connected_t *item;
			item = dl_list_last(&s_device_unuse, struct device_connected_t, node);
			if(item != NULL){
					dl_list_del(&item->node);
					memcpy(item->mac,peer_address,sizeof(item->mac));
				  item->address_type = peer_address_type;
					item->llID = llID;
					item->update_len = DEFAULT_ATT_MTU -3;	
					item->connection_handle = connection_handle;
					item->role.slave.con_timestamp = HAL_VTimerGetCurrentTime_sysT32();
	        dl_list_add(&s_master_list, &item->node);
			}
		}
}


/* remove the node form device list when disconnected */
void device_disconnection_complete_event(uint16_t connection_handle)
{
		struct device_connected_t *item = NULL;
		uint16_t clean_count = 0;
		uint8_t * clean_addr = NULL;
		
    dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
				if(item->connection_handle == connection_handle){
							/* remove bonded information */
							aci_gap_remove_bonded_device(item->address_type,item->mac);
							dl_list_del(&item->node);
							clean_count = sizeof(struct device_connected_t) - sizeof(struct dl_list);
							clean_addr = (uint8_t *)item;
							memset(clean_addr, 0, clean_count);
							PRINTF("remove node\n");
							dl_list_add(&s_device_unuse, &item->node);
							return;
				}
    }

		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(item->connection_handle == connection_handle){
							/* remove bonded information */
							aci_gap_remove_bonded_device(item->address_type,item->mac);
							dl_list_del(&item->node);
							//memset(&item->role,0,sizeof(item->role));
							clean_count = sizeof(struct device_connected_t) - sizeof(struct dl_list);
							clean_addr = (uint8_t *)item;
							memset(clean_addr, 0, clean_count);
							PRINTF("remove node\n");
							dl_list_add(&s_device_unuse, &item->node);
							
							return;
				}
    }
}

/* update the slaves list */
void device_slaves_update(uint8_t peer_address[6],uint8_t Address_Type)
{
		/* check slaves list is full */
		if(device_slaves_is_full())
					return;

		/* check the device if It is  in slaves list ? */
		struct device_connected_t *item;
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(memcmp(item->mac,peer_address,sizeof(item->mac)) == 0){
						return;
				}
		}

		PRINTF("add new device: Address_Type:%x \n",Address_Type);
		print_arr(peer_address, 6);

		
		/* new device , apply a node to add to list */
		item = dl_list_last(&s_device_unuse, struct device_connected_t, node);
		if(item != NULL){
				dl_list_del(&item->node);
				memcpy(item->mac,peer_address,sizeof(item->mac));
			  item->address_type = Address_Type;
				item->role.master.state = MASTER_STATE_INIT;
        dl_list_add_tail(&s_salve_list, &item->node);
		}

}

void device_init(uint16_t tx_handle,uint16_t rx_handle,uint16_t svc_handle)
{
		s_tx_handle = tx_handle;
		s_rx_handle = rx_handle;
//		s_svc_handle = svc_handle;
		
		dl_list_init(&s_salve_list);
		dl_list_init(&s_master_list);
		dl_list_init(&s_device_unuse);

		for (int i = 0; i < ARRAY_LEN(__device_memory); ++i) {
        dl_list_add(&s_device_unuse, &__device_memory[i].node);
    }


		pMS_ConPara = get_multiple_connection_parameters();
}

void aci_gatt_disc_read_char_by_uuid_resp_event(uint16_t Connection_Handle,
                                                uint16_t Attribute_Handle,
                                                uint8_t Attribute_Value_Length,
                                                uint8_t Attribute_Value[])
{
		struct device_connected_t *item = NULL;
		struct master_t *pm;
		uint8_t tx_uuid[] = RX_UUID;
		uint8_t rx_uuid[] = TX_UUID;
		PRINTF("Connection_Handle:%x   %s \n",Connection_Handle, __func__);
		for(int i=0;i<Attribute_Value_Length;i++){
				PRINTF("%x ",Attribute_Value[i]);
		}
		PRINTF("\n");

		// check the length 
		if(Attribute_Value_Length != sizeof(tx_uuid) + 3)
			return;
			
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(Connection_Handle == item->connection_handle){
						pm = &item->role.master;
						if(memcmp(tx_uuid,Attribute_Value+3,sizeof(tx_uuid)) == 0){
								pm->ds_TX.sta = TASK_STATE_DONE;
								pm->TX_handle = Attribute_Handle;
								PRINTF("pm->ds_TX.sta = TASK_STATE_DONE\n");
						}

						if(memcmp(rx_uuid,Attribute_Value+3,sizeof(tx_uuid)) == 0){
								pm->ds_RX.sta = TASK_STATE_DONE;
								pm->RX_handle = Attribute_Handle;
								PRINTF("pm->ds_RX.sta = TASK_STATE_DONE\n");
						}
				}
		}

}

void aci_gatt_error_resp_event(uint16_t Connection_Handle,
															 uint8_t Req_Opcode,
															 uint16_t Attribute_Handle,
															 uint8_t Error_Code)
{
		struct device_connected_t *item = NULL;

		PRINTF("%s  Attribute_Handle:%x  Error_Code:%x \n", __func__, Attribute_Handle, Error_Code);
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(Connection_Handle == item->connection_handle){
						//aci_gap_terminate(item->connection_handle, 0x13);
				}
		}
}
															 
void aci_att_read_by_type_resp_event(uint16_t Connection_Handle,
																		uint8_t Handle_Value_Pair_Length,
																		uint8_t Data_Length,
																		uint8_t Handle_Value_Pair_Data[])
{
		PRINTF("%s \n", __func__);
		for(int i=0;i<Data_Length;i++){
				PRINTF("%x ",Handle_Value_Pair_Data[i]);
		}
		PRINTF("\n");

		struct device_connected_t *item = NULL;
		struct master_t *pm;
		uint8_t tx_uuid[] = RX_UUID;
		uint8_t rx_uuid[] = TX_UUID;
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(Connection_Handle == item->connection_handle && 
					 Data_Length == 21){
						pm = &item->role.master;
						if(memcmp(tx_uuid,&Handle_Value_Pair_Data[5],sizeof(tx_uuid)) == 0){
								pm->ds_TX.sta = TASK_STATE_DONE;
								pm->TX_handle = Handle_Value_Pair_Data[0] + (Handle_Value_Pair_Data[1]<<8);
								PRINTF("pm->TX_handle:%x\n",pm->TX_handle);
						}

						if(memcmp(rx_uuid,&Handle_Value_Pair_Data[5],sizeof(rx_uuid)) == 0){
								pm->ds_RX.sta = TASK_STATE_DONE;
								pm->RX_handle = Handle_Value_Pair_Data[0] + (Handle_Value_Pair_Data[1]<<8);
								PRINTF("pm->RX_handle:%x\n",pm->RX_handle);
						}
				}
		}
}

																		
void aci_gap_pairing_complete_event(uint16_t Connection_Handle,
																		uint8_t Status,
																		uint8_t Reason)
{
		PRINTF("%s Connection_Handle:%x Status:%x Reason:%x \n", __func__ ,Connection_Handle, Status,Reason);
		
		struct device_connected_t *item = NULL;
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				struct master_t *pm;
				pm = &item->role.master;
				if(Connection_Handle == item->connection_handle){
						if(Status == BLE_STATUS_SUCCESS)
								pm->pair.sta = TASK_STATE_DONE;
						else
								pm->pair.sta = TASK_STATE_NONE;
				}
		}


		dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
				struct slave_t *ps;
				ps = &item->role.slave;
				if(Connection_Handle == item->connection_handle){
						if(Status == BLE_STATUS_SUCCESS)
								ps->pair.sta = TASK_STATE_DONE;
						else
								ps->pair.sta = TASK_STATE_NONE;
				}
		}

}

void aci_l2cap_connection_update_resp_event(uint16_t connection_handle,uint16_t result)
{
	PRINTF("aci_l2cap_connection_update_resp_event --> EVENT result:%d \r\n", result);
		struct device_connected_t *item = NULL;
		dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
				struct slave_t *ps;
				if(connection_handle == item->connection_handle){
						ps = &item->role.slave;
						if(result == SUCCESS){
								ps->con_para_update.sta = TASK_STATE_DONE;
						}
						else{
								ps->con_para_update.sta = TASK_STATE_DOING;
						}
				}
		}
}


void aci_l2cap_connection_update_req_event(uint16_t Connection_Handle,
                                           uint8_t Identifier,
                                           uint16_t L2CAP_Length,
                                           uint16_t Interval_Min,
                                           uint16_t Interval_Max,
                                           uint16_t Slave_Latency,
                                           uint16_t Timeout_Multiplier)
{
	PRINTF("%s \n", __func__);
  struct device_connected_t *item = NULL;
		
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				if(Connection_Handle == item->connection_handle){
						struct master_t *pm = &item->role.master;
						pm->con_req.sta = TASK_STATE_DOING;
						pm->con.identifier = Identifier;
						pm->con.L2CAP_Length = L2CAP_Length;
						pm->con.interval_Min = Interval_Min;
						pm->con.interval_Max = Interval_Max;
						pm->con.slave_Latency = Slave_Latency;
						pm->con.timeout_Multiplier = Timeout_Multiplier;
				}
		}
}

void aci_att_exchange_mtu_resp_event(uint16_t Connection_Handle,
                                     uint16_t Att_MTU)
{
	PRINTF("ATT mtu exchanged with value = %d, \n", Att_MTU);
	struct device_connected_t *item = NULL;
	dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
		
		if(Connection_Handle == item->connection_handle){
				item->update_len = Att_MTU-3;
		}
	}

	dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
		struct master_t *pm;
		pm = &item->role.master;
		if(Connection_Handle == item->connection_handle){
				item->update_len = Att_MTU-3;
				pm->do_exchange_cfg.sta = TASK_STATE_DONE;
				PRINTF("exchange_mtu TASK_STATE_DONE \n ");
		}
	}
}

void aci_gatt_attribute_modified_event(uint16_t connection_handle,uint16_t attr_handle,uint16_t offset,uint16_t attr_data_length,uint8_t attr_data[])
{
  //PRINTF("aci_gatt_attribute_modified_event --> EVENT\r\n");
	if(attr_data_length < 2)
			return;

	struct device_connected_t *item = NULL;
	dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
		struct slave_t *ps = &item->role.slave;
		if(connection_handle == item->connection_handle){
				if(attr_handle == s_tx_handle+2){
						ps->ccc = (attr_data[0]==0x01)? TRUE:FALSE;
						PRINTF("ccc:%d \n",ps->ccc);
				}

				if(attr_handle == s_rx_handle+1){
						uint16_t count = (attr_data[0]<<8) + (attr_data[1]<<0);
						PRINTF("connection_handle:%x  attr_data_length:%x  count:%x \n",connection_handle, attr_data_length, count);
						print_arr_short(attr_data, attr_data_length);
				}
		}
	}
	
}


void hci_encryption_change_event(uint8_t Status,
                                 uint16_t Connection_Handle,
                                 uint8_t Encryption_Enabled)
{
		PRINTF("%s \n", __func__);

}

static void device_default_retry_time_proc(struct device_connected_t *item, uint32_t retry_times)					
{
		PRINTF("%s \n", __func__);
		if(retry_times > DEFAULT_TASK_RETRY_TIMES){
				aci_gap_terminate(item->connection_handle, 0x13);
		}
}


/******************************************************************************
 * Function Name  : aci_gatt_proc_complete_event.
 * Description    : This event is generated when a GATT client procedure completes either with error or
successfully..
 * Input          : See file bluenrg1_events.h.
 * Output         : See file bluenrg1_events.h.
 * Return         : See file bluenrg1_events.h.
******************************************************************************/
void aci_gatt_proc_complete_event(uint16_t connection_handle,uint8_t error_code)
{
  PRINTF("aci_gatt_proc_complete_event --> EVENT error_code:%x \r\n", error_code);
	
	struct device_connected_t *item = NULL;
	dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
		struct master_t *pm;
		pm = &item->role.master;
		if(connection_handle == item->connection_handle){
				pm->gatt_proc_flag = FALSE;
		}
		if((connection_handle == item->connection_handle) &&
			 (pm->ccc.sta == TASK_STATE_DOING)){
				pm->ccc.sta = TASK_STATE_DONE;
				PRINTF("connection_handle:%x ccc is enable\n", connection_handle);
		}
	}
}

static void device_master_gatt_proc(void)
{
		struct device_connected_t *item = NULL;
		struct master_t *pm;
		tBleStatus status;
		/*if It have GATT proc even have not done , wait.... */
		
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				pm = &item->role.master;
				if(pm->state != MASTER_STATE_CONNECTED)
					continue;
				if(pm->gatt_proc_flag == TRUE)
					continue;
				
				if(pm->ds_TX.sta == TASK_STATE_NONE){
						uint8_t uuid_tx[] = RX_UUID;
						status = aci_gatt_disc_char_by_uuid(item->connection_handle,0x0001,0xffff,0x02,(UUID_t*)uuid_tx);
						pm->ds_TX.sta = (status == BLE_STATUS_SUCCESS)?TASK_STATE_DOING:TASK_STATE_NONE;
						pm->gatt_proc_flag = (status == BLE_STATUS_SUCCESS)? TRUE:FALSE;
						PRINTF("connection_handle:%x status:%x pm->ds_TX.sta:%d \n",item->connection_handle, status, pm->ds_TX.sta);
						pm->ds_TX.retry_times++;
						device_default_retry_time_proc(item,pm->ds_TX.retry_times);
				}
				
				if(pm->gatt_proc_flag)
					continue;

				if(pm->ds_RX.sta == TASK_STATE_NONE){
						uint8_t uuid_rx[] = TX_UUID;
						status = aci_gatt_disc_char_by_uuid(item->connection_handle,0x0001,0xffff,0x02,(UUID_t*)uuid_rx);
						pm->ds_RX.sta = (status == BLE_STATUS_SUCCESS)?TASK_STATE_DOING:TASK_STATE_NONE;
						pm->gatt_proc_flag = (status == BLE_STATUS_SUCCESS)? TRUE:FALSE;
						PRINTF("connection_handle:%x status:%x pm->ds_RX.sta:%d \n",item->connection_handle,status, pm->ds_RX.sta);
						pm->ds_RX.retry_times++;
						device_default_retry_time_proc(item,pm->ds_RX.retry_times);
				}

				if(pm->gatt_proc_flag)
					continue;
				
				if(pm->ccc.sta == TASK_STATE_NONE){
						uint8_t ccc[2] = {0x01, 0x00};
						status = aci_gatt_write_char_desc(item->connection_handle,pm->RX_handle+2,sizeof(ccc),ccc);
						pm->ccc.sta = (status == BLE_STATUS_SUCCESS)?TASK_STATE_DOING:TASK_STATE_NONE;
						pm->gatt_proc_flag = (status == BLE_STATUS_SUCCESS)? TRUE:FALSE;
						pm->ccc.retry_times++;
						PRINTF("connection_handle:%x status:%x pm->CCC.sta:%d \n",item->connection_handle,status, pm->ccc.sta);
						device_default_retry_time_proc(item,pm->ccc.retry_times);
				}

				if(pm->gatt_proc_flag)
					continue;

				if(pm->do_exchange_cfg.sta == TASK_STATE_NONE){
						tBleStatus status = aci_gatt_exchange_config(item->connection_handle);
						pm->do_exchange_cfg.sta = (status == BLE_STATUS_SUCCESS)?TASK_STATE_DOING:TASK_STATE_NONE;
						pm->gatt_proc_flag = (status == BLE_STATUS_SUCCESS)? TRUE:FALSE;
						PRINTF("aci_gatt_exchange_config  status:%x  \n", status);
				}

				if(pm->gatt_proc_flag)
					continue;
		}
}



static void device_master_Tick(APP_ENVI_CLEAR_CALLBACK app_envi_cb)
{

		/* if the list have the node are not connected */
		struct device_connected_t *item = NULL;
		struct master_t *pm;
		tBleStatus status;

		
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				pm = &item->role.master;
				/* if some device is connecting , wait..... */
				if(pm->state == MASTER_STATE_CONNECTING)
					break;

#ifdef STATBLE_CONNECT_PRO_CFG
				if( (pm->state == MASTER_STATE_CONNECTED) && ((pm->do_exchange_cfg.sta != TASK_STATE_DONE)))
					break;
#endif
				
				if(pm->state == MASTER_STATE_INIT){
						// clear the environment incase BLE EVEN conflict when connecting
						app_envi_cb(BLE_STATUS_SUCCESS,NULL);

						status = aci_gap_create_connection(pMS_ConPara->Scan_Interval,
                                     pMS_ConPara->Scan_Window,
                                     item->address_type,
                                     item->mac,
                                     0x00,
                                     pMS_ConPara->Connection_Interval,
                                     pMS_ConPara->Connection_Interval,
                                     0x0000,
                                     400,//(int) (pMS_ConPara->Connection_Interval * 1.25*100), //SUPERVISION_TIMEOUT,
                                     pMS_ConPara->CE_Length,
                                     pMS_ConPara->CE_Length);
				
						PRINTF("aci_gap_create_connection %x  type:%x  \n", status, item->address_type);
						pm->state = (status == BLE_STATUS_SUCCESS)?MASTER_STATE_CONNECTING:MASTER_STATE_INIT;
						pm->con_timestamp = HAL_VTimerGetCurrentTime_sysT32();
						return;
				}
		}	

		device_master_gatt_proc();
		
		dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				pm = &item->role.master;
				if(!(pm->state == MASTER_STATE_CONNECTED))
					return;
				con_param_t *pcon = &pm->con;

				//if(pm->do_exchange_cfg.sta != TASK_STATE_DONE)
				//	continue;
				
				if(pm->con_req.sta == TASK_STATE_DOING){
						PRINTF("interval_Min:%x \n",pcon->interval_Min);
						PRINTF("interval_Max:%x \n",pcon->interval_Max);
						PRINTF("slave_Latency:%x \n",pcon->slave_Latency);
						PRINTF("timeout_Multiplier:%x \n",pcon->timeout_Multiplier);
						PRINTF("identifier:%x \n",pcon->identifier);
						tBleStatus status = aci_l2cap_connection_parameter_update_resp(item->connection_handle, pcon->interval_Min,pcon->interval_Max,
													pcon->slave_Latency,pcon->timeout_Multiplier,(CE_LENGTH_MS/0.625),(CE_LENGTH_MS/0.625),pcon->identifier,0x01);
					  PRINTF("aci_l2cap_connection_parameter_update_resp	status:%x \r\n", status);
						pm->con_req.sta = (status == BLE_STATUS_SUCCESS)?TASK_STATE_DONE:TASK_STATE_DOING;
				}
		}

		
		
}


uint16_t device_get_handle(uint8_t llID)
{
	uint16_t ret = 0xffff;
	struct device_connected_t *item = NULL;
	dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
		if(llID == item->llID){
				return item->connection_handle;
		}
	}

	dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
		if(llID == item->llID){
				return item->connection_handle;
		}
	}

	return ret;
}

static void device_master_test_write(void)
{
	struct device_connected_t *item = NULL;
	static struct device_connected_t *next_item = NULL;
	
	static uint32_t last_time;
	static uint16_t count = 0;
	uint32_t cur_time = HAL_VTimerGetCurrentTime_sysT32();
	uint32_t ms = (uint32_t)abs(cur_time-last_time)*2.4414/1000;
	if(ms > 1000){
			last_time = cur_time;

			if( dl_list_len(&s_salve_list) == 0)
					return;
			
			uint8_t temp[APP_MAX_ATT_SIZE];
			memset(temp,0,sizeof(temp));
			
			count++;
			temp[0] = (count>>8)&0xff;
			temp[1] = (count>>0)&0xff;

			struct device_connected_t *first = dl_list_first(&s_salve_list, struct device_connected_t, node);

			
			uint16_t len = 0;
			dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				uint16_t handle = item->role.master.TX_handle;
				task_t ds_TX = item->role.master.ds_TX;
				if(next_item == NULL )
					next_item = item;
				
			  if(item == next_item){
					if( (item != NULL) && (handle != 0) && (ds_TX.sta == TASK_STATE_DONE) ){
						tBleStatus status = aci_gatt_write_without_resp(item->connection_handle, handle+1, 20, temp);
						PRINTF("aci_gatt_write_char_value: status:%x	connection_handle:%X \n", status, item->connection_handle);
					}
					next_item = dl_list_next(&s_salve_list, next_item, struct device_connected_t, node);
					break;
				}
				
				len++;
				if(len >= dl_list_len(&s_salve_list))
					item = NULL;
			}
	}
}



static void device_slave_Tick(APP_ENVI_CLEAR_CALLBACK app_envi_cb)
{
		struct device_connected_t *item = NULL;
		struct slave_t *ps;
		
		dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
				ps = &item->role.slave;
#ifndef MASK_SECURITY				
				if(ps->pair.sta == TASK_STATE_NONE){
						uint8_t status = aci_gap_send_pairing_req(item->connection_handle,FALSE);
						PRINTF("aci_gap_send_pairing_req  status:%x\r\n", status);
						ps->pair.sta = (status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE;
						ps->pair.retry_times++;
						return;
				}
#endif
				
				if(ps->con_para_update.sta == TASK_STATE_NONE){
					 // this is for test. application shoule be adjust param
					 	PRINTF("aci_l2cap_connection_parameter_update_req:\r\n");
						uint8_t status = aci_l2cap_connection_parameter_update_req(item->connection_handle,50,50,0,500);
						ps->con_para_update.sta = (status == BLE_STATUS_SUCCESS)? TASK_STATE_DOING:TASK_STATE_NONE;
						ps->con_para_update.retry_times++;
						PRINTF("status %x \n", status);
				}

		}
}


/* */
void device_Tick(APP_ENVI_CLEAR_CALLBACK app_envi_cb)
{
		device_master_Tick(app_envi_cb);
		device_slave_Tick(app_envi_cb);
		device_master_test_write();

		
//		device_slave_test_notify();
		

}


/* return if It have any task need to upper layer mutex BLE even  */
BOOL device_connected_task_all_done(void)
{
		if(device_is_empty())
				return TRUE;
		
		struct device_connected_t *item;
		struct master_t *pm;
    dl_list_for_each(item, &s_salve_list, struct device_connected_t, node) {
				pm = &item->role.master;
				if(pm->state == MASTER_STATE_CONNECTING
#ifdef STATBLE_CONNECT_PRO_CFG
				|| pm->ccc.sta != TASK_STATE_DONE || 
#ifndef MASK_SECURITY
					 pm->pair.sta != TASK_STATE_DONE || 
#endif
					 pm->ds_TX.sta != TASK_STATE_DONE || \
					 pm->ds_RX.sta != TASK_STATE_DONE || \
					 pm->do_exchange_cfg.sta != TASK_STATE_DONE
#endif
					 )
					return FALSE;
    }
#ifndef MASK_SECURITY
		struct slave_t *ps;
		item = NULL;
		dl_list_for_each(item, &s_master_list, struct device_connected_t, node) {
				ps = &item->role.slave;
				if(ps->pair.sta != TASK_STATE_DONE)
					return FALSE;
    }
#endif
		return TRUE;
}








