/*
 * main_lcd.c
 *
 *  Created on: 28.01.2021
 *      Author: Engineer_Sed
 */


#define LCD_HI2C      hi2c1
#define LCD_I2C_ADDR  0x4F /* 0x7F (0x3F) for PCF8574AT,  0x4F (0x27) for PCF8574T */
#define LCD_ROWS      2
#define LCD_COLS      16

LCD_I2C_TypeDef lcd;

char scr[LCD_ROWS][LCD_COLS];

uint8_t lcd_buf[1000];

void Init_LCD()
{
	lcd.hi2c     = &LCD_HI2C;
	lcd.addr     = LCD_I2C_ADDR;
	lcd.rows     = LCD_ROWS;
	lcd.cols     = LCD_COLS;
	lcd.charsize = 0;
	lcd.buf_ptr  = lcd_buf;
	lcd.buf_size = sizeof(lcd_buf);

//			// Auto detect I2C addr:
//	        // - set breakpoint to line: while(1)
//  	    // - run
//          // - if breakpoint reached, then see value of variable a in debugger
//			//
//			for (int a=0; a<0x7f; a++)
//			{
//				uint8_t d = 0;
//				if (HAL_I2C_Master_Transmit(lcd.hi2c, a, &d, 1, 2) == HAL_OK)
//				{
//					a = a | 1;
//					while(1);
//				}
//			}

	LCD_I2C_Init     (&lcd);
	LCD_I2C_backlight(&lcd);
}

void lcd_refresh()
{
	if (!LCD_I2C_Busy(&lcd))
	{
		lcd_to_refresh = 0;

		// prepare chars
		for (int r=0; r<LCD_ROWS; r++)
		{
			int i;

			// fill with chars from scr array
			for (i=0; i<LCD_COLS; i++)
			{
				char * c = &scr[r][i];
				if (*c == 0)   break; // the rest of the line will be empty
				if (*c < 0x20) *c = ' ';
			}

			// rest of line fill with spaces
			for (; i<LCD_COLS; i++)
			{
				scr[r][i] = ' ';
			}
		}

		LCD_I2C_startAsyncWrite(&lcd);
		{
			for (int r=0; r<LCD_ROWS; r++)
			{
				LCD_I2C_setCursor(&lcd, 0, r);
				for (int c=0; c<LCD_COLS; c++)
				{
					LCD_I2C_write(&lcd, scr[r][c]);
				}
			}
		}
		LCD_I2C_sendAsyncData(&lcd);
	}
}
