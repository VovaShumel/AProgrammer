/*
 * console.h
 *
 *  Created on: Jan 22, 2021
 *      Author: Engineer_Sed
 */

#ifndef INC_CONSOLE_H_
#define INC_CONSOLE_H_

#include "main.h"

void Console_Init(UART_HandleTypeDef *huart);
void Console_ParseCommand(char * s, int len, char * out_s, int * out_len, int out_len_max); // defined by user
void Console_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void Console_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void Console_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif /* INC_CONSOLE_H_ */
