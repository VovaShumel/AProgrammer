/*
 * lcd_i2c.h
 *
 *  Created on: Jan 28, 2021
 *      Author: Engineer_Sed
 */

#ifndef INC_LCD_I2C_H_
#define INC_LCD_I2C_H_

#include "main.h"





typedef struct
{
	I2C_HandleTypeDef * hi2c;
	uint8_t   addr;
	uint8_t   cols;
	uint8_t   rows;
	uint8_t   charsize;
	uint8_t * buf_ptr;
	int       buf_size;

	// private
	uint8_t displayfunction;
	uint8_t displaycontrol;
	uint8_t displaymode;
	uint8_t backlightval;

	int      async;
	int      buf_count;

} LCD_I2C_TypeDef;


// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0b00000100  // Enable bit
#define Rw 0b00000010  // Read/Write bit
#define Rs 0b00000001  // Register select bit



void LCD_I2C_Init           (LCD_I2C_TypeDef * lcd);
void LCD_I2C_backlight      (LCD_I2C_TypeDef * lcd);
void LCD_I2C_noBacklight    (LCD_I2C_TypeDef * lcd);
void LCD_I2C_write          (LCD_I2C_TypeDef * lcd, uint8_t value);
void LCD_I2C_startAsyncWrite(LCD_I2C_TypeDef * lcd);
void LCD_I2C_sendAsyncData  (LCD_I2C_TypeDef * lcd);
int  LCD_I2C_Busy           (LCD_I2C_TypeDef * lcd);
void LCD_I2C_setCursor      (LCD_I2C_TypeDef * lcd, uint8_t col, uint8_t row);



#endif /* INC_LCD_I2C_H_ */
