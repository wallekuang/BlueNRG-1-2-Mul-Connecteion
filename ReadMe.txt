这个Demo是针对BlueNRG-1/2 的多连接demo(BLE_Examples\BLE_MS_Formula)进行优化改进的代码。

老Demo的缺陷:
1. 无法应用于实际工程，just a demo
2. 不稳定，断连后无法重连，只设定了发送数据
3. 通信速率慢


在使用新Demo之前，建议先阅读官方的老的资料
资料索引参考:C:\Program Files (x86)\STMicroelectronics\BlueNRG-1_2 DK 3.2.0\Docs\index.html
----> Bluetooth Low Energy (BLE) Multiple Connections scenarios formula and timing chart 
----> Bluetooth Low Energy (BLE) Multiple Connections scenarios guidelines 


重要概念:
BlueNRG-1/2 的多连接demo 是基于时间槽的【这点和BlueNRG-LP的不一样】。
所有BLE事件（包括广播，扫描间隔，连接事件）统一一个固定的周期。
如表格: Copy of Multiple_Connection_Formula_Timings_Chart.xlsx 里面图示.



文件ble_utils.c 中函数GET_Master_Slave_device_connection_parameters 就是通过给定的参数，计算相应的周期。
工程默认重要配置（Keil 为例子，IAR类似）： 
Options for target ----->  Preprocessor Symbols:
LS_SOURCE=LS_SOURCE_EXTERNAL_32KHZ 
SMPS_INDUCTOR=SMPS_INDUCTOR_10uH 
BLE_STACK_CONFIGURATION=BLE_STACK_FULL_CONFIGURATION 
MASTER_SLAVE=1 MASTER_SLAVE_NUM_MASTERS=1 
MASTER_SLAVE_NUM_SLAVES=6 MASTER_SLAVE_SCAN_WINDOW=20.00 
MASTER_SLAVE_SLEEP_TIME=0.00 BLUENRG2_DEVICE 
HS_SPEED_XTAL=HS_SPEED_XTAL_32MHZ USER_BUTTON=BUTTON_1 DEBUG=1

工程默认BlueNRG-1 使用16M晶振，BlueNRG-2使用32M晶振，
使用内部DC-DC（SMPS_INDUCTOR=SMPS_INDUCTOR_10uH），
中心节点可以被一个主机连接（MASTER_SLAVE_NUM_MASTERS=1 ）
中心节点可以连接6个子节点（MASTER_SLAVE_NUM_SLAVES=6）
扫描窗口20ms（MASTER_SLAVE_SCAN_WINDOW=20.00）

如果你的应用需求和此不同，可以自行更改验证。



默认守卫时间（守卫时间概念可以通过查看表格得知:Copy of Multiple_Connection_Formula_Timings_Chart.xlsx ）
#define GUARD_TIME 1.6   //ms
#define GUARD_TIME_END 2.5 //ms

默认无需调整，但是如果你实际调试中如果打印最大空余时间槽（Print_Anchor_Period）
发现时间溢出了，则需要调整此处。
























