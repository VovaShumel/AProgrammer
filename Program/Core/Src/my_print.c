/*
 * my_print.c
 *
 *  Created on: Jan 22, 2021
 *      Author: Engineer_Sed
 */

#include "my_print.h"

void my_print_str(char *dest_s, char *src_s, int *ptr, int ptr_max)
{
  int i=0;
  while ((*ptr<ptr_max) && (src_s[i]))
    dest_s[(*ptr)++] = src_s[i++];
}
////////////////////////////////////////////////////////////////////////////////

void my_print_int(char *out_s, int t, int *ptr, int ptr_max)
{
      uint8_t s[11];
      int l = 0;
      if (t<0) {
        t *= -1;
        if (*ptr<ptr_max) out_s[(*ptr)++] = '-';
      }
      do {
        s[l++] = (t%10)+'0';
        t /= 10;
      } while (t);
      for (int j=0; (j<l) && (*ptr<ptr_max); j++) out_s[(*ptr)++] = s[l-j-1];
}
////////////////////////////////////////////////////////////////////////////////

void my_print_int_a(char *out_s, int t, int align, int *ptr, int ptr_max)
{
      uint8_t s[11];
      int l = 0;
      int negative;

      // check for negative value
      if (t<0)
      {
    	negative = 1;
        t *= -1;
      }
      else negative = 0;

      // extract digits
      do {
        s[l++] = (t%10)+'0';
        t /= 10;
      } while (t);

      // add sign
      if ((*ptr<ptr_max) && (negative)) s[l++] = '-';

      // add align spaces
      for (int j=0; (j<align-l) && (*ptr<ptr_max); j++)
    	  out_s[(*ptr)++] = ' ';

      // out sign and digits
      for (int j=0; (j<l) && (*ptr<ptr_max); j++)
    	  out_s[(*ptr)++] = s[l-j-1];
}
////////////////////////////////////////////////////////////////////////////////

void my_print_uint_d(char *out_s, uint32_t t, int digits, int *ptr, int ptr_max)
{
      uint8_t s[11];
      int l = 0;
      do {
        s[l++] = (t%10)+'0';
        t /= 10;
      } while (t);
      for (int j=0; (j<(digits-l)) && (*ptr<ptr_max); j++) out_s[(*ptr)++] = '0';
      for (int j=0; (j<l) && (*ptr<ptr_max); j++) out_s[(*ptr)++] = s[l-j-1];
}
////////////////////////////////////////////////////////////////////////////////
