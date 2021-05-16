/*
 * main_while.c
 *
 *  Created on: 29.01.2021
 *      Author: Engineer_Sed
 */

#define _AT_HOME1 (key[DIN_HOME1].state == 1)
#define _AT_HOME2 (key[DIN_HOME2].state == 1)

#define _ON_VALVE  HAL_GPIO_WritePin(VALVE_ON_GPIO_Port, VALVE_ON_Pin, GPIO_PIN_SET)
#define _OFF_VALVE HAL_GPIO_WritePin(VALVE_ON_GPIO_Port, VALVE_ON_Pin, GPIO_PIN_RESET)


void cmd_update_rotate_turns()
{
	int m = 1; // rotate stepper (Motor)
	if (CNC_TaskFinished_i(m) == 0) CNC_Stop_i(m);

	if (rotate_turns >= 0)
	{
		  CNC_ResetCoord(m);

		  TCoord c = (rotate_turns == 0) ?  2000000000/cnc.m[m].mm :
											rotate_turns * 360;

		  if (rotate_direction == 1) c = -c;

		  CNC_SetSpeed_i  (m, rotate_speed*360/60*2);
		  CNC_Start_GoTo_i(m, &c);
		  CNC_Update_i    (m); // start
	}

	user_cmd = CMD_IDLE;
}
////////////////////////////////////////////////////////////////////////////////

void cmd_move_with_arrows()
{
  int m = 0; // pump stepper (Motor)

  if (CNC_TaskFinished_i(m) == 0) CNC_Stop_i(m);

  CNC_ResetCoord(m);

  TCoord c = 2000000000; // max
  if (pump_direction == 1) c = -c;

  if (!((pump_direction == 0) && _AT_HOME2) &&
	  !((pump_direction == 1) && _AT_HOME1))
  {
	  CNC_SetSpeed_i(m, pump_manual_speed);
	  CNC_Start_GoTo_i(m, &c);
	  CNC_Update_i (m); // start move
  }

  while ((pump_cmd_timeout > 0) &&
		 !((pump_direction == 0) && _AT_HOME2) &&
		 !((pump_direction == 1) && _AT_HOME1))
  {
	  pump_cmd_timeout--; // wait until arrow key is released
	  HAL_Delay(10);
  }

  CNC_Stop_i(m);
}
////////////////////////////////////////////////////////////////////////////////

void cmd_start_dozing()
{
	screen_to_refresh = 1;

	int p = 0;
	int r = 1;

	int dont_touch_rotate = CNC_TaskFinished_i(r) ? 0 : 1;

	if (dont_touch_rotate)
	{
		if (CNC_TaskFinished_i(p) == 0) CNC_Stop_i(p);
	}
	else // stop all motors
		if (CNC_TaskFinished() == 0) CNC_Stop();

	TCoord c;

	// On valve
	_ON_VALVE;

	// Delay
	if (valve_delay >= 0)   HAL_Delay(valve_delay);

	// Start rotating
	if (!dont_touch_rotate)
	{
		CNC_ResetCoord(r);

		c = rotate_step * 360; // turns to degrees
		if (rotate_direction == 1) c = -c;

		CNC_SetSpeed_i  (r, rotate_speed*360/60*2);
		CNC_Start_GoTo_i(r, &c);
		CNC_Update_i    (r); // start move
	}


	int count = MOVES_COUNT; //TODO: take from settings

	for (int i = 0; i < count; i++)
	{
		if (pump_feed[i] == 0) continue; // skip empty feed

		if (_AT_HOME1 || _AT_HOME2) break;

		// Delay before pump move
		if (pump_delay[i] >= 0) HAL_Delay(pump_delay[i]); // df - delay before pump forward move after starting rotating

		// i-move
		CNC_ResetCoord(p);

		c = pump_feed[i];

		CNC_SetSpeed_i  (p, pump_speed[i]*2);
		CNC_Start_GoTo_i(p, &c);
		CNC_Update_i    (p); // start move

		while ((CNC_TaskFinished_i(p) == 0) && (!_AT_HOME1) && (!_AT_HOME2)) ;
	}

	if (_AT_HOME1 || _AT_HOME2)  CNC_Stop(); //

	// 7. Wait for task completion
	if (!dont_touch_rotate)
		while (CNC_TaskFinished() == 0) ;

	// Of valve
	_OFF_VALVE;

	screen_to_refresh = 1;
}

void cmd_go_home()
{
	TCoord c;
	int p = 0;

	CNC_ResetCoord(p);

	c = -pump_max_length;

	CNC_SetSpeed_i  (p, pump_home_speed*2);
	CNC_Start_GoTo_i(p, &c);
	CNC_Update_i    (p); // start move

	while ((CNC_TaskFinished_i(p) == 0) && (!_AT_HOME1)) ;

	CNC_Stop(p);

	screen_to_refresh = 1;
}

void Loop() {
//	  if (user_cmd) {
//		  if (user_cmd == CMD_UPDATE_ROTATE_TURNS) cmd_update_rotate_turns();
//		  if (user_cmd == CMD_MOVE_WITH_ARROWS)    cmd_move_with_arrows();
//		  if (user_cmd == CMD_START_DOZING)        cmd_start_dozing();
//		  if (user_cmd == CMD_GO_HOME)             cmd_go_home();
//
//		  user_cmd = CMD_IDLE;
//	  }

// TODO по клику кнопкой загрузить прошивку
}
