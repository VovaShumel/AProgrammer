/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "inc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef enum {
	CMD_IDLE,
	CMD_GO_HOME,
	CMD_UPDATE_ROTATE_TURNS,
	CMD_MOVE_WITH_ARROWS,
	CMD_START_DOZING,
} USER_CMD_TypeDef;

volatile USER_CMD_TypeDef user_cmd;

#define MOVES_COUNT 5 /* should not be greater than 9 (one digit only) */

int pump_feed [MOVES_COUNT] = { 175,  100, -250 };
int pump_speed[MOVES_COUNT] = { 100,   28,  200 };
int pump_delay[MOVES_COUNT] = {   0,    0,    0 };

int pump_max_length = 30000;
__IO int pump_direction  = 0;
__IO int pump_cmd_timeout;
int pump_manual_speed; // in 0,01mm/sec
int pump_home_speed = 200;
int valve_delay = 1000;

#define PMS_SET_COUNT 2
int pump_manual_speed_set[PMS_SET_COUNT] = { 100, 600 };
int cur_pump_manual_speed = 0;

int rotate_step      =  6; // in turns
int rotate_speed     = 60; // in turns/minute (RPM)
int rotate_turns     =  -1; // in turns
int rotate_direction =  0; // 0 - forward, 1 - reverse

int screen_to_refresh = 0;
int lcd_to_refresh = 0;

int syringe_diam = 20; // in mm

#define SETTINGS_ITEMS_COUNT (sizeof(sett_item) / sizeof(sett_item[0]))

int settings_level   = 0;
int settings_item    = 0;
int settings_changed = 0;

typedef struct {
	char * caption;
	char * unit;
	int *  value;
	int    min;
	int    max;
	int    step;

} SETTINGS_ITEM_Typedef;

const SETTINGS_ITEM_Typedef sett_item[] = {
		// caption        unit       pointer            min  max step
		{ "Syr.diameter", "mm",       &syringe_diam,       1,   100,   1 },
		{ "P.man.speed",  ".01mm/s",  &pump_manual_speed, 10,  2000,  10 },
		{ "P.home speed", ".01mm/s",  &pump_home_speed,   10,  2000,  10 },
};

typedef enum {
	VW_DOZING = 0,
	VW_MANUAL,
	VW_CHANGE,
	VW_SETTINGS,
	VW_LAST
} SCREEN_VIEW_TypeDef;

__IO SCREEN_VIEW_TypeDef screen_view = VW_DOZING;

int tick_10ms = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#include "main_keys.c"
#include "main_console.c"
#include "main_cnc.c"
#include "main_lcd.c"
#include "main_loop.c"

void Setup() {
	Init_CNC();
	Init_Console();
	Init_Keys();
	Init_LCD();

	pump_manual_speed = pump_manual_speed_set[cur_pump_manual_speed];

	screen_to_refresh = 1;
}

#define P2F_CHANGE_STEP 5
#define P2F_CHANGE_MAX  5000
#define P2F_INDEX 2
#define P2F_NAME "p2f "

#define _ADD_CHAR(L,C) do {\
	if (p<LCD_COLS) scr[L][p++] = C;\
	} while(0)

void screen_refresh() {
	int r = 0;
	int p = 0;

	switch (screen_view)
	{
		case VW_DOZING:
		{
			// First row
			my_print_str(scr[r], "DOZING: ", &p, LCD_COLS);

			if (user_cmd == CMD_START_DOZING)
			{
				my_print_str(scr[r], "Dozing..", &p, LCD_COLS);
			}
			else
			{
				my_print_str(scr[r], "Ready", &p, LCD_COLS);
			}
			_ADD_CHAR(r, 0);

			// Second row
			r++;
			p = 0;
			my_print_str(scr[r], P2F_NAME, &p, LCD_COLS);
			my_print_int(scr[r], pump_feed[P2F_INDEX-1], &p, LCD_COLS);
			_ADD_CHAR(r, 0);
			break;
		}

		case VW_MANUAL:
		{
			my_print_str(scr[r], "MANUAL:", &p, LCD_COLS);
			_ADD_CHAR(r, 0);

			r++;
			p = 0;
			my_print_str(scr[r], "Speed ", &p, LCD_COLS);
			my_print_int(scr[r], pump_manual_speed, &p, LCD_COLS);
			_ADD_CHAR(r, 0);
			break;
		}

		case VW_CHANGE:
		{
			my_print_str(scr[r], "CHANGE:", &p, LCD_COLS);

			if (user_cmd == CMD_GO_HOME)
			{
				my_print_str(scr[r], " Homing..", &p, LCD_COLS);
			}
			_ADD_CHAR(r, 0);

			r++;
			p = 0;
			_ADD_CHAR(r, 0);
			break;
		}

		case VW_SETTINGS:
		{
			if (settings_level == 0)
			{
				my_print_int(scr[r], settings_item+1, &p, LCD_COLS);
				_ADD_CHAR(r, ' ');
				my_print_str(scr[r], sett_item[settings_item].caption, &p, LCD_COLS);
				_ADD_CHAR(r, 0);

				r++;
				p = 0;
				my_print_int(scr[r], *(sett_item[settings_item].value), &p, LCD_COLS);
				_ADD_CHAR(r, 0);
			}
			else if (settings_level == 1)
			{

			}
			else while (1);

			break;
		}

		default: while(1); // each view should be implemented
	} // switch (screen_view)

	screen_to_refresh = 0;
	lcd_to_refresh = 1; // refresh display
}

#define KEY_REPEAT_DELAY  100 /* in 10ms, e.g. for 100 delay is 100*10ms = 1000ms */
#define KEY_REPEAT_PERIOD 20  /* in 10ms, e.g. for 100 delay is 100*10ms = 1000ms */

#define _KEY_PRESSED_(KEY)  (key[KEY].down_time == 1)
#define _KEY_DOWN_(KEY)     (key[KEY].state     == 0)
#define _KEY_LONG_DOWN(KEY) (key[KEY].down_time == KEY_REPEAT_DELAY)
#define _KEY_PRESSED_WITH_REPEAT_(KEY) (_KEY_PRESSED_(KEY) || \
             ((key[KEY].down_time > KEY_REPEAT_DELAY) && ((key[KEY].down_time - KEY_REPEAT_DELAY) % KEY_REPEAT_PERIOD == 0)))

int handle_esc_to_change_view()
{
	// Esc key
	if ((_KEY_PRESSED_(KEY_ESC)) && (user_cmd == CMD_IDLE))
	{
		// OnExit Menu event
		if (screen_view == VW_MANUAL)
		{
			// reset manual speed
			cur_pump_manual_speed = 0;
			pump_manual_speed     = pump_manual_speed_set[cur_pump_manual_speed];
		}
		else if (screen_view == VW_SETTINGS)
		{
			if (settings_changed)
			{
				//TODO: save settings

				settings_changed = 0;
			}
		}

		if (screen_view+1 == VW_LAST) screen_view = 0;
		else                          screen_view++;

		// OnEnter Menu event
		if (screen_view == VW_SETTINGS)
		{
			settings_level = 0;
		}

		screen_to_refresh = 1;
		return 1;
	}
	return 0;
}

void handle_key_events()
{
	do
	{
		if (screen_view == VW_DOZING)
		{
			if (handle_esc_to_change_view()) break;

			// PEDAL key
			if (_KEY_PRESSED_(KEY_PEDAL) || _KEY_PRESSED_(KEY_ENTER))
			{
				if (user_cmd == CMD_IDLE)  user_cmd = CMD_START_DOZING;
			}

			else if (_KEY_PRESSED_WITH_REPEAT_(KEY_DOWN))
			{
				pump_feed[P2F_INDEX-1] = (pump_feed[P2F_INDEX-1] > P2F_CHANGE_STEP)  ? pump_feed[P2F_INDEX-1] - P2F_CHANGE_STEP : 0;
				screen_to_refresh = 1;
			}

			else if (_KEY_PRESSED_WITH_REPEAT_(KEY_UP))
			{
				pump_feed[2-1] = (pump_feed[P2F_INDEX-1] + P2F_CHANGE_STEP < P2F_CHANGE_MAX)  ? pump_feed[P2F_INDEX-1] + P2F_CHANGE_STEP : P2F_CHANGE_MAX;
				screen_to_refresh = 1;
			}
		}

		else if (screen_view == VW_MANUAL)
		{
			if (handle_esc_to_change_view()) break;

			// Down/Up (Left/Right) key
			if (_KEY_DOWN_(KEY_DOWN) || _KEY_DOWN_(KEY_UP))
			{
				int desired_direction = _KEY_DOWN_(KEY_DOWN) ? 0 : 1;

				if (user_cmd == CMD_IDLE)
				{
					pump_direction = desired_direction;
					pump_cmd_timeout = 3; // in 10ms, e.g. 3 is equal 3*10ms = 30ms
					user_cmd = CMD_MOVE_WITH_ARROWS;
				}
				else if ((user_cmd == CMD_MOVE_WITH_ARROWS) && (pump_direction == desired_direction)) // if is already moving in desired direction
				{
					// continue moving
					pump_cmd_timeout = 3; // in 10ms, e.g. 3 is equal 3*10ms = 30ms
				}
			}

			else if (_KEY_PRESSED_(KEY_PEDAL) || _KEY_PRESSED_(KEY_ENTER))
			{
				if (user_cmd == CMD_IDLE)
				{
					cur_pump_manual_speed = (cur_pump_manual_speed+1) % PMS_SET_COUNT;
					pump_manual_speed     = pump_manual_speed_set[cur_pump_manual_speed];
					screen_to_refresh = 1;
				}
			}
		}

		else if (screen_view == VW_CHANGE)
		{
			if (handle_esc_to_change_view()) break;

			// Down/Up (Left/Right) key
			if (_KEY_DOWN_(KEY_DOWN) || _KEY_DOWN_(KEY_UP))
			{
				int desired_direction = _KEY_DOWN_(KEY_DOWN) ? 0 : 1;

				if (user_cmd == CMD_IDLE)
				{
					pump_direction = desired_direction;
					pump_cmd_timeout = 3; // in 10ms, e.g. 3 is equal 3*10ms = 30ms
					user_cmd = CMD_MOVE_WITH_ARROWS;
				}
				else if ((user_cmd == CMD_MOVE_WITH_ARROWS) && (pump_direction == desired_direction)) // if is already moving in desired direction
				{
					// continue moving
					pump_cmd_timeout = 3; // in 10ms, e.g. 3 is equal 3*10ms = 30ms
				}
			}

			else if (_KEY_LONG_DOWN(KEY_ENTER))
			{
				if (user_cmd == CMD_IDLE)
				{
					user_cmd = CMD_GO_HOME;
					screen_to_refresh = 1;
				}
			}
		}

		else if (screen_view == VW_SETTINGS)
		{
			if (settings_level == 0) // Settings list
			{
				if (handle_esc_to_change_view()) break;

				if (_KEY_PRESSED_(KEY_ENTER)) // Enter to parameter changing
				{
//					parameter_value = settings.;
					settings_level++;
					screen_to_refresh = 1;
				}
				else if (_KEY_PRESSED_WITH_REPEAT_(KEY_DOWN)) // previous item
				{
					settings_item = (settings_item-1 + SETTINGS_ITEMS_COUNT) % SETTINGS_ITEMS_COUNT;
					screen_to_refresh = 1;
				}

				else if (_KEY_PRESSED_WITH_REPEAT_(KEY_UP)) // next item
				{
					settings_item = (settings_item+1) % SETTINGS_ITEMS_COUNT;
					screen_to_refresh = 1;
				}
			}
			else if (settings_level == 1) // Parameter changing
			{
				if (_KEY_PRESSED_(KEY_ENTER)) // apply parameter changes and exit
				{
					// save parameter
	//				if (parameter_value != settings_item)
					{
						settings_changed = 1;
					}

					settings_level--;
					screen_to_refresh = 1;
				}
			}
		}

	} while(0);
}

void Handle_1ms_Timer() {
	if (++tick_10ms == 10) { // each 10 ms
		tick_10ms = 0;

		handle_key_states();
		handle_key_events();
	}

	if (screen_to_refresh)
		screen_refresh();

	if (lcd_to_refresh)
		lcd_refresh();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  Setup();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
	  Loop();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
