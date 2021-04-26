/*
 * lcd_i2c.c
 *
 *  Created on: Jan 28, 2021
 *      Author: Engineer_Sed
 */

#include "lcd_i2c.h"



inline void LCD_I2C_delay(uint32_t del) __attribute__((always_inline));
inline void LCD_I2C_delay(uint32_t del)
{
	HAL_Delay(del);
}

void LCD_I2C_delayMicroseconds(uint32_t del_us)
{
	uint32_t del = (del_us + 500) / 1000;
	if (del == 0) del = 1;
	LCD_I2C_delay(del);
}

/************ low level data pushing commands **********/

inline void LCD_I2C_expanderWrite(LCD_I2C_TypeDef * lcd, uint8_t data) __attribute__((always_inline));
inline void LCD_I2C_expanderWrite(LCD_I2C_TypeDef * lcd, uint8_t data){

	uint8_t d = data | lcd->backlightval;

	if (lcd->async)
	{
		if (lcd->buf_count < lcd->buf_size)
		{
		    lcd->buf_ptr[lcd->buf_count++]= d;
		}
	}
	else
		HAL_I2C_Master_Transmit(lcd->hi2c, lcd->addr, &d, 1, 2);
}

void LCD_I2C_expanderWriteAsync(LCD_I2C_TypeDef * lcd)
{
	if ((lcd->async) && (lcd->buf_count > 0))
	{
		HAL_I2C_Master_Transmit_IT(lcd->hi2c, lcd->addr, lcd->buf_ptr, lcd->buf_count);
	}
}

void LCD_I2C_write4bits(LCD_I2C_TypeDef * lcd, uint8_t value)
{
	LCD_I2C_expanderWrite(lcd, value);

	//_pulseEnable(value):
	LCD_I2C_expanderWrite(lcd, value | En);	// En high
	//LCD_I2C_delayMicroseconds(1);		// enable pulse must be >450ns
	LCD_I2C_expanderWrite(lcd, value & ~En);	// En low
	//LCD_I2C_delayMicroseconds(50);		// commands need > 37us to settle
}

// write either command or data
void LCD_I2C_send(LCD_I2C_TypeDef * lcd, uint8_t value, uint8_t mode) {
	uint8_t highnib =  value     & 0xf0;
	uint8_t lownib  = (value<<4) & 0xf0;
	LCD_I2C_write4bits(lcd, highnib | mode);
	LCD_I2C_write4bits(lcd, lownib  | mode);
}

//void LCD_I2C_custom_character(uint8_t char_num, uint8_t *rows){
//	createChar(char_num, rows);
//}
//
//void LCD_I2C_setBacklight(uint8_t new_val){
//	if (new_val) {
//		backlight();		// turn backlight on
//	} else {
//		noBacklight();		// turn backlight off
//	}
//}


/*********** mid level commands, for sending data/cmds */

inline void LCD_I2C_command(LCD_I2C_TypeDef * lcd, uint8_t value) __attribute__((always_inline));
inline void LCD_I2C_command(LCD_I2C_TypeDef * lcd, uint8_t value) {
	LCD_I2C_send(lcd, value, 0);
}

void LCD_I2C_write(LCD_I2C_TypeDef * lcd, uint8_t value)
{
	LCD_I2C_send(lcd, value, Rs);
}

/********** high level commands, for the user! */

void LCD_I2C_startAsyncWrite(LCD_I2C_TypeDef * lcd)
{
	if ((lcd->buf_ptr) && (lcd->buf_size > 0))
	{
		lcd->buf_count = 0;
		lcd->async = 1;
	}
}

void LCD_I2C_sendAsyncData(LCD_I2C_TypeDef * lcd)
{
	if (lcd->async)
	{
		LCD_I2C_expanderWriteAsync(lcd);
		lcd->buf_count = 0;
		lcd->async = 0; // off async mode
	}
}

int LCD_I2C_Busy(LCD_I2C_TypeDef * lcd)
{
	return (lcd->hi2c->State == HAL_I2C_STATE_READY) ? 0 : 1;
}

void LCD_I2C_clear(LCD_I2C_TypeDef * lcd)
{
	if (lcd->async) return;

	LCD_I2C_command(lcd, LCD_CLEARDISPLAY);// clear display, set cursor position to zero
	LCD_I2C_delayMicroseconds(2000);  // this command takes a long time!
}

//void LiquidCrystal_I2C::home(){
//	command(LCD_RETURNHOME);  // set cursor position to zero
//	delayMicroseconds(2000);  // this command takes a long time!
//}

void LCD_I2C_setCursor(LCD_I2C_TypeDef * lcd, uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row > lcd->rows) {
		row = lcd->rows-1;    // we count rows starting w/0
	}
	LCD_I2C_command(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void LCD_I2C_noDisplay(LCD_I2C_TypeDef * lcd) {
	lcd->displaycontrol &= ~LCD_DISPLAYON;
	LCD_I2C_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}
void LCD_I2C_display(LCD_I2C_TypeDef * lcd) {
	lcd->displaycontrol |= LCD_DISPLAYON;
	LCD_I2C_command(lcd, LCD_DISPLAYCONTROL | lcd->displaycontrol);
}

//// Turns the underline cursor on/off
//void LiquidCrystal_I2C::noCursor() {
//	_displaycontrol &= ~LCD_CURSORON;
//	command(LCD_DISPLAYCONTROL | _displaycontrol);
//}
//void LiquidCrystal_I2C::cursor() {
//	_displaycontrol |= LCD_CURSORON;
//	command(LCD_DISPLAYCONTROL | _displaycontrol);
//}
//
//// Turn on and off the blinking cursor
//void LiquidCrystal_I2C::noBlink() {
//	_displaycontrol &= ~LCD_BLINKON;
//	command(LCD_DISPLAYCONTROL | _displaycontrol);
//}
//void LiquidCrystal_I2C::blink() {
//	_displaycontrol |= LCD_BLINKON;
//	command(LCD_DISPLAYCONTROL | _displaycontrol);
//}
//
//// These commands scroll the display without changing the RAM
//void LiquidCrystal_I2C::scrollDisplayLeft(void) {
//	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
//}
//void LiquidCrystal_I2C::scrollDisplayRight(void) {
//	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
//}
//
//// This is for text that flows Left to Right
//void LiquidCrystal_I2C::leftToRight(void) {
//	_displaymode |= LCD_ENTRYLEFT;
//	command(LCD_ENTRYMODESET | _displaymode);
//}
//
//// This is for text that flows Right to Left
//void LiquidCrystal_I2C::rightToLeft(void) {
//	_displaymode &= ~LCD_ENTRYLEFT;
//	command(LCD_ENTRYMODESET | _displaymode);
//}
//
//// This will 'right justify' text from the cursor
//void LiquidCrystal_I2C::autoscroll(void) {
//	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
//	command(LCD_ENTRYMODESET | _displaymode);
//}
//
//// This will 'left justify' text from the cursor
//void LiquidCrystal_I2C::noAutoscroll(void) {
//	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
//	command(LCD_ENTRYMODESET | _displaymode);
//}
//
//// Allows us to fill the first 8 CGRAM locations
//// with custom characters
//void LiquidCrystal_I2C::createChar(uint8_t location, uint8_t charmap[]) {
//	location &= 0x7; // we only have 8 locations 0-7
//	command(LCD_SETCGRAMADDR | (location << 3));
//	for (int i=0; i<8; i++) {
//		write(charmap[i]);
//	}
//}
//
// Turn the (optional) backlight off/on
void LCD_I2C_noBacklight(LCD_I2C_TypeDef * lcd)
{
	lcd->backlightval=LCD_NOBACKLIGHT;
	LCD_I2C_expanderWrite(lcd, 0);
}

void LCD_I2C_backlight(LCD_I2C_TypeDef * lcd) {
	lcd->backlightval=LCD_BACKLIGHT;
	LCD_I2C_expanderWrite(lcd, 0);
}
//bool LiquidCrystal_I2C::getBacklight() {
//  return _backlightval == LCD_BACKLIGHT;
//}




// When the display powers up, it is configured as follows:
//
// 1. Display clear
// 2. Function set:
//    DL = 1; 8-bit interface data
//    N = 0; 1-line display
//    F = 0; 5x8 dot character font
// 3. Display on/off control:
//    D = 0; Display off
//    C = 0; Cursor off
//    B = 0; Blinking off
// 4. Entry mode set:
//    I/D = 1; Increment by 1
//    S = 0; No shift
//
// Note, however, that resetting the Arduino doesn't reset the LCD, so we
// can't assume that its in that state when a sketch starts (and the
// LiquidCrystal constructor is called).

void LCD_I2C_Init(LCD_I2C_TypeDef * lcd)
{
	lcd->async = 0;

	lcd->displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (lcd->rows > 1) {
		lcd->displayfunction |= LCD_2LINE;
	}

	// for some 1 line displays you can select a 10 pixel high font
	if ((lcd->charsize != 0) && (lcd->rows == 1)) {
		lcd->displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
	LCD_I2C_delay(50);

	// Now we pull both RS and R/W low to begin commands
	LCD_I2C_expanderWrite(lcd, lcd->backlightval);	// reset expander and turn backlight off (Bit 8 =0)
	//LCD_I2C_delay(1000);

	//put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet
	// figure 24, pg 46

	// we start in 8bit mode, try to set 4 bit mode
	LCD_I2C_write4bits(lcd, 0x03 << 4);
	LCD_I2C_delayMicroseconds(4500); // wait min 4.1ms

	// second try
	LCD_I2C_write4bits(lcd, 0x03 << 4);
	LCD_I2C_delayMicroseconds(4500); // wait min 4.1ms

	// third go!
	LCD_I2C_write4bits(lcd, 0x03 << 4);
	LCD_I2C_delayMicroseconds(150);

	// finally, set to 4-bit interface
	LCD_I2C_write4bits(lcd, 0x02 << 4);

	// set # lines, font size, etc.
	LCD_I2C_command(lcd, LCD_FUNCTIONSET | lcd->displayfunction);

	// turn the display on with no cursor or blinking default
	lcd->displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	LCD_I2C_display(lcd);

	// clear it off
	LCD_I2C_clear(lcd);

	// Initialize to default text direction (for roman languages)
	lcd->displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;

	// set the entry mode
	LCD_I2C_command(lcd, LCD_ENTRYMODESET | lcd->displaymode);

	//LCD_I2C_home(); // too long, we use setCursor
	LCD_I2C_setCursor(lcd, 0, 0);


}
