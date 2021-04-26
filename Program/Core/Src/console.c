/*
 * console.c
 *
 *  Created on: Jan 22, 2021
 *      Author: Engineer_Sed
 */

#include "console.h"
#include "my_print.h"

#define RX_STR_SIZE 20
char    rx_char;
char    rx_str[RX_STR_SIZE];
int     rx_size;
int     rx_esc;
int p;
#define TX_BUF_SIZE 2000
char tx_buf[TX_BUF_SIZE];

int tx_size;
uint8_t tx_empty = 1;
uint8_t rx_not_empty = 0;


////////////////////////////////////////////////////////////////////////////////

void Console_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  tx_empty = 1;
}
////////////////////////////////////////////////////////////////////////////////

void Console_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	uint8_t c = rx_char;

    tx_size = 0;
    if (!rx_esc)
    {
		if (c==0x0D) // Enter
		{
		  tx_buf[tx_size++] = c;
		  tx_buf[tx_size++] = 0x0A;
		  int tmp = tx_size; // save
		  Console_ParseCommand(rx_str, rx_size, tx_buf, &tx_size, TX_BUF_SIZE);

		  if (tx_size != tmp) my_print_str(tx_buf, "\r\n", &tx_size, TX_BUF_SIZE);
		  rx_size = 0;
		  rx_esc  = 0;
		}
		else if ((c==0x08) || (c==0x7F)) // Backspace
		{
		  if (rx_size)
		  {
			rx_size--;
			tx_buf[tx_size++] = c;
			tx_buf[tx_size++] = ' ';
			tx_buf[tx_size++] = c;
		  }
		}
		else if ((rx_size < RX_STR_SIZE) &&
				 (((c>='0') && (c<='9')) ||
				  ((c>='a') && (c<='z')) ||
				  ((c>='A') && (c<='Z')) ||
				   (c=='=') || (c==' ') || (c=='-')))
		{
		  tx_buf[tx_size++] = c;
		  rx_str[rx_size++] = c;
		}
		else if ((rx_size == 0) && (c==27)) // first esc char
		{
			rx_esc = 1;
		}
		else if ((rx_size == 0) && ((c=='[') || (c=='{')))
		{
			  rx_str[rx_size++] = c;
			  Console_ParseCommand(rx_str, rx_size, tx_buf, &tx_size, TX_BUF_SIZE);
			  rx_size = 0;
		}
    }
    else // Esc chars
    {
        rx_str[rx_size++] = c;

    	if ((rx_size == 2) && (rx_str[0] == '[') && ((rx_str[1] == 'D') || (rx_str[1] == 'C')))
		{
    		rx_str[0] = 27;
   	        Console_ParseCommand(rx_str, rx_size, tx_buf, &tx_size, TX_BUF_SIZE);

   	        if (tx_size) my_print_str(tx_buf, "\r\n", &tx_size, TX_BUF_SIZE);
			rx_size = 0;
			rx_esc  = 0;
		}
	}

    if (tx_size)
    {
		tx_empty = 0;
		HAL_UART_Transmit_IT(huart, (uint8_t*)tx_buf, tx_size);
    }

    HAL_UART_Receive_IT(huart, (uint8_t*)&rx_char, 1);
}
////////////////////////////////////////////////////////////////////////////////

void Console_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  tx_empty = 1;
}
////////////////////////////////////////////////////////////////////////////////

void Console_Init(UART_HandleTypeDef *huart)
{
	  rx_size = 0;
	  rx_esc  = 0;
	  HAL_UART_Receive_IT(huart, (uint8_t*)&rx_char, 1); // start recieving chain
}
////////////////////////////////////////////////////////////////////////////////

