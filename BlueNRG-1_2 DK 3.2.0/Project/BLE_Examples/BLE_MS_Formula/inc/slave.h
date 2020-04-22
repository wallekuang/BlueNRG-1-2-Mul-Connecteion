/*****************************************************************************
File Name:    slave.h
Description:  
Note: 
History:	
Date                Author                   Description
2019-10-28         Lucien                     Create
****************************************************************************/
#ifndef __SLAVE_H__
#define __SLAVE_H__

#include <stdint.h>


#define MAX_SUPPORTED_SLAVES  3


typedef uint8_t slave_address_type[6]; 



void set_database(void);


void device_initialization(void);


void set_device_discoverable(void);

void APP_Tick(void);





#endif


