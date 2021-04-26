/*
 * my_print.h
 *
 *  Created on: Jan 22, 2021
 *      Author: Engineer_Sed
 */

#ifndef INC_MY_PRINT_H_
#define INC_MY_PRINT_H_

#include "main.h"

void my_print_str(char *dest_s, char *src_s, int *ptr, int ptr_max);
void my_print_int(char *out_s, int t, int *ptr, int ptr_max);
void my_print_int_a(char *out_s, int t, int align, int *ptr, int ptr_max);
void my_print_uint_d(char *out_s, uint32_t t, int digits, int *ptr, int ptr_max);

#endif /* INC_MY_PRINT_H_ */
