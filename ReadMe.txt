���Demo�����BlueNRG-1/2 �Ķ�����demo(BLE_Examples\BLE_MS_Formula)�����Ż��Ľ��Ĵ��롣

��Demo��ȱ��:
1. �޷�Ӧ����ʵ�ʹ��̣�just a demo
2. ���ȶ����������޷�������ֻ�趨�˷�������
3. ͨ��������


��ʹ����Demo֮ǰ���������Ķ��ٷ����ϵ�����
���������ο�:C:\Program Files (x86)\STMicroelectronics\BlueNRG-1_2 DK 3.2.0\Docs\index.html
----> Bluetooth Low Energy (BLE) Multiple Connections scenarios formula and timing chart 
----> Bluetooth Low Energy (BLE) Multiple Connections scenarios guidelines 


��Ҫ����:
BlueNRG-1/2 �Ķ�����demo �ǻ���ʱ��۵ġ�����BlueNRG-LP�Ĳ�һ������
����BLE�¼��������㲥��ɨ�����������¼���ͳһһ���̶������ڡ�
����: Copy of Multiple_Connection_Formula_Timings_Chart.xlsx ����ͼʾ.



�ļ�ble_utils.c �к���GET_Master_Slave_device_connection_parameters ����ͨ�������Ĳ�����������Ӧ�����ڡ�
����Ĭ����Ҫ���ã�Keil Ϊ���ӣ�IAR���ƣ��� 
Options for target ----->  Preprocessor Symbols:
LS_SOURCE=LS_SOURCE_EXTERNAL_32KHZ 
SMPS_INDUCTOR=SMPS_INDUCTOR_10uH 
BLE_STACK_CONFIGURATION=BLE_STACK_FULL_CONFIGURATION 
MASTER_SLAVE=1 MASTER_SLAVE_NUM_MASTERS=1 
MASTER_SLAVE_NUM_SLAVES=6 MASTER_SLAVE_SCAN_WINDOW=20.00 
MASTER_SLAVE_SLEEP_TIME=0.00 BLUENRG2_DEVICE 
HS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ USER_BUTTON=BUTTON_1 DEBUG=1

����Ĭ��BlueNRG-1 ʹ��16M����BlueNRG-2ʹ��32M����
ʹ���ڲ�DC-DC��SMPS_INDUCTOR=SMPS_INDUCTOR_10uH����
���Ľڵ���Ա�һ���������ӣ�MASTER_SLAVE_NUM_MASTERS=1 ��
���Ľڵ��������6���ӽڵ㣨MASTER_SLAVE_NUM_SLAVES=6��
ɨ�贰��20ms��MASTER_SLAVE_SCAN_WINDOW=20.00��

������Ӧ������ʹ˲�ͬ���������и�����֤��



Ĭ������ʱ�䣨����ʱ��������ͨ���鿴����֪:Copy of Multiple_Connection_Formula_Timings_Chart.xlsx ��
#define GUARD_TIME 1.6   //ms
#define GUARD_TIME_END 2.5 //ms

Ĭ��������������������ʵ�ʵ����������ӡ������ʱ��ۣ�Print_Anchor_Period��
����ʱ������ˣ�����Ҫ�����˴���
























