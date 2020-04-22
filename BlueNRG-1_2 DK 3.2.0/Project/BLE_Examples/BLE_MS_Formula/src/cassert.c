/**
  ******************************************************************************
  * @file    cassert.c
  * @author  AMG GCSA Lucien
  * @version V1.0.0
  * @date    10/23/2018
  * @brief   
  ******************************************************************************
  * @attention
  *
  *
  * <h2><center>&copy; COPYRIGHT 2018 STMicroelectronics</center></h2>
  ******************************************************************************
  */
#include "cassert.h"
#include "BlueNRG1_uart.h"
#include "cassert.h"

void uart_write_str(uint8_t *bufptr)
{
	if(bufptr != NULL)
	{
			while(*bufptr)
			{
				if(UART_GetFlagStatus(UART_FLAG_BUSY) == RESET)
					UART_SendData(*(bufptr++));
			}
	}
}

void assert_error(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number*/
  COMPrintf("Wrong parameters value: file %s on line %d\r\n", file, line);
  /* Infinite loop */
  while (1)
  {
  }
}




#define PRINT_BUF (256)
// serial printf   uint64_t %llu
static uint8_t printf_buffer[PRINT_BUF] = {0};
int COMPrintf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt );
	memset(printf_buffer,'\0',sizeof(printf_buffer));
	uint16_t len = vsprintf((char *)printf_buffer,fmt,ap);
	va_end( ap );

	uart_write_str(printf_buffer);
	return len;   
}

int COMPrintf_hexdump(uint8_t *parr,uint8_t len)
{
	char my_buf[PRINT_BUF];
	if(len < ((PRINT_BUF-1) /5))
	{
			memset(my_buf,0,sizeof(my_buf));
			for(int i=0;i<len;i++)
			{
					if((i+1)!=len)
						sprintf(my_buf+5*i,"0x%02x ",parr[i]);
					else
						sprintf(my_buf+5*i,"0x%02x \n",parr[i]);
			}

			COMPrintf(my_buf);
	}  
	return len;
}

