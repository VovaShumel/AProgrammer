/*
 * main_keys.c
 *
 *  Created on: 28.01.2021
 *      Author: Engineer_Sed
 */

typedef enum
{
	KEY_PEDAL,
	KEY_ESC,
	KEY_DOWN,
	KEY_UP,
	KEY_ENTER,
	DIN_HOME1, //TODO: move all DIN_... to separate object
	DIN_HOME2,
	DIN_VALVE_STATE,
	DIN_PUMP_ALM,
	DIN_P24V_OK,

	KEY_LAST
} KEY_ENUM_TypeDef;

typedef struct
{
	GPIO_TypeDef * Port;
	uint16_t       Pin;
	int            input;
	int            state;
	int            down_time;

} KEY_TypeDef;


KEY_TypeDef key[KEY_LAST];

void handle_key_states()
{
	for (int i=0; i<KEY_LAST; i++)
	{
		// PEDAL input
		int cur = (HAL_GPIO_ReadPin(key[i].Port, key[i].Pin) == GPIO_PIN_RESET) ? 0 : 1;

		if (cur == key[i].input) // filter 10 ms
		{
			key[i].state = cur; // update state
		}
		key[i].input = cur; // save current state

		if (key[i].state == 0) key[i].down_time++;
		else                   key[i].down_time = 0;
	}
}


void Init_Keys()
{
	int k = KEY_PEDAL;
	key[k].Port = PEDAL_GPIO_Port;
	key[k].Pin  = PEDAL_Pin;

	k = KEY_ESC;
	key[k].Port = KEY_ESC_GPIO_Port;
	key[k].Pin  = KEY_ESC_Pin;

	k = KEY_DOWN;
	key[k].Port = KEY_DOWN_GPIO_Port;
	key[k].Pin  = KEY_DOWN_Pin;

	k = KEY_UP;
	key[k].Port = KEY_UP_GPIO_Port;
	key[k].Pin  = KEY_UP_Pin;

	k = KEY_ENTER;
	key[k].Port = KEY_ENTER_GPIO_Port;
	key[k].Pin  = KEY_ENTER_Pin;

	k = DIN_HOME1;
	key[k].Port = HOME1_GPIO_Port;
	key[k].Pin  = HOME1_Pin;

	k = DIN_HOME2;
	key[k].Port = HOME2_GPIO_Port;
	key[k].Pin  = HOME2_Pin;

	k = DIN_VALVE_STATE;
	key[k].Port = VALVE_STATE_GPIO_Port;
	key[k].Pin  = VALVE_STATE_Pin;

	k = DIN_PUMP_ALM;
	key[k].Port = PUMP_ALM_GPIO_Port;
	key[k].Pin  = PUMP_ALM_Pin;

	k = DIN_P24V_OK;
	key[k].Port = P24V_OK_GPIO_Port;
	key[k].Pin  = P24V_OK_Pin;

	for (int i=0; i<KEY_LAST; i++)
	{
		key[i].input     = 1;
		key[i].state     = 1;
		key[i].down_time = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////

