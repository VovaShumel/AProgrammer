/*
 * main_console.c
 *
 *  Created on: Jan 27, 2021
 *      Author: Engineer_Sed
 */

#define CONSOLE_HUART huart1

////////////////////////////////////////////////////////////////////////////////

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &CONSOLE_HUART)
	{
		Console_UART_TxCpltCallback(huart);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &CONSOLE_HUART)
	{
		Console_UART_RxCpltCallback(huart);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &CONSOLE_HUART)
	{
		Console_UART_ErrorCallback(huart);
	}
}
////////////////////////////////////////////////////////////////////////////////

void Init_Console()
{
	Console_Init(&CONSOLE_HUART);
}
////////////////////////////////////////////////////////////////////////////////

int ParseParam(char * s, int p, int len, int * param)
{
	  p++;
	  while ((p<len) &&
	      ((s[p]==' ') || (s[p]=='='))) p++; // dell trailing spaces...

	  int t = 0;  // convert str to int
	  int is = 0;

	  int sign = 1;

	  if ((p<len) && (s[p]=='-'))
	  {
		  sign = -1;
		  p++;
	  }

	  while ((p<len) &&
	         (s[p]>='0') && (s[p]<='9'))
	  {
	    t = t*10 + (s[p++]-'0');
	    is = 1;
	  }

	  if (is)  *param = sign*t;
	  return is;
}
////////////////////////////////////////////////////////////////////////////////

void Console_ParseCommand(char * s, int len, char * out_s, int * out_len, int out_len_max)
{
	if (len)
	{
		int p = 0;
		while ((s[p]==' ') && (p<len)) p++; // dell trailing spaces

		if (p<len)
		{
			int error_in_cmd = 1;

			switch (s[p])
			{
				case 'i': // info
				case 'h': // help
				case '?': //
				{

#define _PRINT_PARAM(S1,PARAM,S2) do { \
	my_print_str(out_s, S1,    out_len, out_len_max); \
	my_print_int_a(out_s, PARAM, 5, out_len, out_len_max); \
	my_print_str(out_s, S2,    out_len, out_len_max); \
	my_print_str(out_s, "\r\n",    out_len, out_len_max); \
				} while (0)

					my_print_str(out_s, "===============================\r\n", out_len, out_len_max);
					my_print_str(out_s, "Grease Coating Machine commands\r\n", out_len, out_len_max);
					my_print_str(out_s, "===============================\r\n", out_len, out_len_max);

					_PRINT_PARAM ("rn ", rotate_speed,     " - rotate speed (turns per minute)");
					_PRINT_PARAM ("rd ", rotate_direction, " - rotate direction (0 - forward, 1 - reverse)");
					my_print_str(out_s, "rt<N>    - start rotate for N turns (0 - endless, <empty> - stop)\r\n\r\n",    out_len, out_len_max);

					my_print_str(out_s, "Left/Right arrow - move pump while key is pressed (for minimum 300ms)\r\n",    out_len, out_len_max);
					_PRINT_PARAM ("pms ", pump_manual_speed, " - manual pump movement speed (in 0.01 mm/sec)");
					_PRINT_PARAM ("phs ", pump_home_speed,   " - home movement speed (in 0.01 mm/sec)");
					my_print_str(out_s, "ph        - move to Home\r\n\r\n",    out_len, out_len_max);

					my_print_str(out_s, "Dozing params and commands:\r\n", out_len, out_len_max);
					my_print_str(out_s, "---------------------------\r\n", out_len, out_len_max);

					_PRINT_PARAM ("rs ", rotate_step,       " - rotate step (in turns/rapes)");
					_PRINT_PARAM ("rn ", rotate_speed,      " - rotate speed (turns per minute)");
					_PRINT_PARAM ("vd ", valve_delay,       " - delay after valve on and before start rotating (in ms)");

					my_print_str(out_s, "p<i>f - pump move #i feed/step (in 0.01 mm, negative for reverse move)\r\n", out_len, out_len_max);
					my_print_str(out_s, "p<i>s - pump move #i speed (in 0.01 mm/sec)\r\n", out_len, out_len_max);
					my_print_str(out_s, "p<i>d - delay before pump move #i (in ms)\r\n", out_len, out_len_max);
					my_print_str(out_s, "-----------------------\r\n", out_len, out_len_max);
					my_print_str(out_s, "i:  p<i>f  p<i>s  p<i>d\r\n", out_len, out_len_max);

					for (int i = 0; i < MOVES_COUNT; i++)
					{
						my_print_int(out_s, i+1, out_len, out_len_max);
						my_print_str(out_s, ":", out_len, out_len_max);

						my_print_int_a(out_s, pump_feed [i], 7, out_len, out_len_max);
						my_print_int_a(out_s, pump_speed[i], 7, out_len, out_len_max);
						my_print_int_a(out_s, pump_delay[i], 7, out_len, out_len_max);
						my_print_str(out_s, "\r\n",        out_len, out_len_max);
					}
					my_print_str(out_s, "\r\n",    out_len, out_len_max);

					my_print_str(out_s, "[ or { - start dozing\r\n",    out_len, out_len_max);
					my_print_str(out_s, "\r\n",    out_len, out_len_max);

					error_in_cmd = 0;
					break;
				}

				case 'r': // rotate stepper command
				{
					p++; // see next char
					if (p == len) break;

					switch (s[p])
					{
						case 'n': // rn - rotate speed in turns per minute
						{
							int param;
							if ((ParseParam(s, p, len, &param)) &&
								((param > 0) || (param <= 1200)))
							{
								rotate_speed = param; // in turns per minute (RPM)
								error_in_cmd = 0;
							}
							break;
						}

						case 'd': // rd - rotate direction: 0 - forward, 1 - reverse
						{
							int param;
							if ((ParseParam(s, p, len, &param)) &&
								((param == 0) || (param == 1)))
							{
								rotate_direction = param;
								error_in_cmd = 0;
							}
							break;
						}

						case 't': // rt - rotate turns: 0 - infinity, starts turning
						{
							int param;
							if ((ParseParam(s, p, len, &param)) &&
								((param >= 0) || (param < 1000)))
							{
								// start turning
								rotate_turns = param;
							}
							else // no param ?
							{
								// stop turning
								rotate_turns = -1;
							}

							if (user_cmd == CMD_IDLE)  user_cmd = CMD_UPDATE_ROTATE_TURNS;
							error_in_cmd = 0;

							break;
						}

						case 's': // rs - rotate step
						{
							int param;
							if ((ParseParam(s, p, len, &param)) &&
								((param > 0) || (param <= 600)))
							{
								rotate_step = param; // in turns
								error_in_cmd = 0;
							}
							break;
						}

						default: break;
					}

					break;
				}

				case 'p': // pump stepper command
				{
					p++; // see next char
					if (p == len) break;

					switch (s[p])
					{
						case 'm': // pm
						{
							p++; // see next char
							if (p == len) break;

							if (s[p] == 's') // pms - pump speed in manual control
							{
								int param;
								if ((ParseParam(s, p, len, &param)) &&
									(param > 0))
								{
									pump_manual_speed = param; // in 0.01 mm/sec
									error_in_cmd = 0;
								}
							}
							break;
						}

						case 'h': // pump home
						{
							p++; // see next char

							if (p == len) // ph - pump home movement
							{
								if (user_cmd == CMD_IDLE)  user_cmd = CMD_GO_HOME;
								error_in_cmd = 0;
							}
							else if (s[p] == 's') // phs - pump home speed
							{
								int param;
								if ((ParseParam(s, p, len, &param)) &&
									(param > 0))
								{
									pump_home_speed = param; // in 0.01 mm/sec
									error_in_cmd = 0;
								}
							}
							break;
						}

						default:
						{
							if ((s[p] >= '1') && (s[p] <= '0'+MOVES_COUNT)) // if i-move
							{
								int i = s[p]-'0'-1; // move index (number-1)

								p++; // see next char
								if (p == len) break;

								switch (s[p])
								{
									case 'f': // p<i>f - pump move feed/step
									{
										int param;
										if (ParseParam(s, p, len, &param))
										{
											pump_feed[i] = param; // in 0.01 mm
											error_in_cmd = 0;
										}
										break;
									}

									case 's': // p<i>s - pump move step
									{
										int param;
										if ((ParseParam(s, p, len, &param)) &&
											(param > 0))
										{
											pump_speed[i] = param; // in 0.01 mm/sec
											error_in_cmd = 0;
										}
										break;
									}
									case 'd': // p<i>d - delay before pump move
									{
										int param;
										if ((ParseParam(s, p, len, &param)) &&
											(param >= 0))
										{
											pump_delay[i] = param; // in ms
											error_in_cmd = 0;
										}
										break;
									}

									default: break;
								}

							}
						}
					}

					break;
				}

				case 'v': // valve command
				{
					p++; // see next char
					if (p == len) break;

					if (s[p] == 'd') // vd - delay after valve on and before start rotating
					{
						int param;
						if ((ParseParam(s, p, len, &param)) &&
							(param >= 0))
						{
							valve_delay = param; // in 0.01 mm
							error_in_cmd = 0;
						}
					}
					break;
				}

				case '{': // pump stepper command
				case '[':
				{
					if (user_cmd == CMD_IDLE)  user_cmd = CMD_START_DOZING;
					error_in_cmd = 0;
					break;
				}


				case 27: // escape chars
				{
					p++; // see next char
					if (p == len) break;

					if ((s[p] == 'D') || (s[p] == 'C')) // Left / Right arrow
					{
						if ((user_cmd == CMD_IDLE) || (user_cmd == CMD_MOVE_WITH_ARROWS))
						{
							pump_direction = (s[p] == 'D') ? 0 : 1; // Left arrow
							pump_cmd_timeout = 30; // in 10ms, e.g. 100 is equal 100*10ms = 1000ms
							user_cmd = CMD_MOVE_WITH_ARROWS;
						}
						error_in_cmd = 0;
					}
					break;
				}

				default: break;

			} // switch (first char)

			if (error_in_cmd)
				my_print_str(out_s, "Error in command", out_len, out_len_max);
		} // if (p<len)
	} // if (len)
}


