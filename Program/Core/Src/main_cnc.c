/*
 * main_cnc.c
 *
 *  Created on: Jan 27, 2021
 *      Author: Engineer_Sed
 */

#define TIM_PUMP    TIM1
#define TIM_ROTATE  TIM2
#define TIM_CNC_DEL TIM3

#define CNC_DEL (20-5) /* 20*0.1us = 2us pulse on CNCx_STEP signal */

#define TIM_PUMP_CLK    (2*HAL_RCC_GetPCLK1Freq()/1000000)  /* 1 us */
#define TIM_ROTATE_CLK  (2*HAL_RCC_GetPCLK2Freq()/1000000)  /* 1 us */
#define TIM_CNC_DEL_CLK (2*HAL_RCC_GetPCLK2Freq()/10000000) /* 0.1 us */

#define TIM_PUMP_IRQn   TIM1_UP_IRQn
#define TIM_ROTATE_IRQn TIM2_IRQn

#define TIM_PUMP_DBG_REG CR
#define TIM_PUMP_DBG_BIT DBGMCU_CR_DBG_TIM1_STOP

#define TIM_ROTATE_DBG_REG CR
#define TIM_ROTATE_DBG_BIT DBGMCU_CR_DBG_TIM2_STOP

#define TIM_CNC_DEL_DBG_REG CR
#define TIM_CNC_DEL_DBG_BIT DBGMCU_CR_DBG_TIM3_STOP

#define TIM_PUMP_CLOCK_ON     __HAL_RCC_TIM1_CLK_ENABLE()
#define TIM_ROTATE_CLOCK_ON   __HAL_RCC_TIM2_CLK_ENABLE()
#define TIM_CNC_DEL_CLOCK_ON  __HAL_RCC_TIM3_CLK_ENABLE()

void TIM1_UP_IRQHandler()
{
    CNC_ISR(0);
}

void TIM2_IRQHandler()
{
	CNC_ISR(1);
}


void Init_CNC()
{
	// To init cnc.c do next:
	// Check defines above.
	// From TIM ISR call CNC_ISR(m).
	// Correct cnc.pulse_width for CNC_Del

    // Настройка таймеров для CNC
	TIM_PUMP_CLOCK_ON;   // on TIM clock
	TIM_ROTATE_CLOCK_ON;   // on TIM clock

    TIM_PUMP->CR1   = 0;
    TIM_ROTATE->CR1 = 0;
    TIM_PUMP->PSC   = TIM_PUMP_CLK-1;
    TIM_ROTATE->PSC = TIM_ROTATE_CLK-1;
    NVIC_SetPriority(TIM_PUMP_IRQn,   0);
    NVIC_SetPriority(TIM_ROTATE_IRQn, 0);
    NVIC_EnableIRQ  (TIM_PUMP_IRQn);
    NVIC_EnableIRQ  (TIM_ROTATE_IRQn);

    DBGMCU->TIM_PUMP_DBG_REG   |= TIM_PUMP_DBG_BIT;
    DBGMCU->TIM_ROTATE_DBG_REG |= TIM_ROTATE_DBG_BIT;

	// CNC delay timer
	TIM_CNC_DEL_CLOCK_ON;
	TIM_CNC_DEL->CR1 = 0 | TIM_CR1_OPM; // one pulse mode
	TIM_CNC_DEL->PSC = TIM_CNC_DEL_CLK-1;
	TIM_CNC_DEL->ARR = CNC_DEL; /* 0.1us * 20 = 2us */
	DBGMCU->TIM_CNC_DEL_DBG_REG |= TIM_CNC_DEL_DBG_BIT;

    cnc.enable_port = 0;
    cnc.del_tim = TIM_CNC_DEL;

    // PUMP
    cnc.m[0].dir_port  = PUMP_DIR_GPIO_Port;
    cnc.m[0].dir_pin   = PUMP_DIR_Pin;
    cnc.m[0].dir_inv   = 1;
    cnc.m[0].dir_delay = 2; // in CNC_Del() count
    cnc.m[0].step_port = PUMP_STEP_GPIO_Port;
    cnc.m[0].step_pin  = PUMP_STEP_Pin;
//    cnc.m[0].limit     = HOME_X;
//    cnc.m[0].limit_inv = 0;
    cnc.m[0].tim       = TIM_PUMP;
    cnc.m[0].mm        = 400;
					// 400 - 3200 pulses/rev - 2,5 um/pulse
					// 200 - 1600 pulses/rev - 5   um/pulse
					// 100 -  800 pulses/rev - 10  um/pulse
    cnc.m[0].home_ofs  = 200;
    cnc.m[0].x_max     = 23400;

    cnc.m[0].Anom      = 100000; // 0.01 mm/sec^2
    cnc.m[0].Vnom/*[CNC_SPEED_NORMAL]*/ = 1000; // 0.01 mm/sec
    //cnc.m[0].Vnom[CNC_SPEED_SLOW  ] = 1000; // 0.01 mm/sec

    // ROTATE
    cnc.m[1].dir_port  = ROTATE_DIR_GPIO_Port;
    cnc.m[1].dir_pin   = ROTATE_DIR_Pin;
    cnc.m[1].dir_inv   = 1;
    cnc.m[1].dir_delay = 2; // in CNC_Del() count
    cnc.m[1].step_port = ROTATE_STEP_GPIO_Port;
    cnc.m[1].step_pin  = ROTATE_STEP_Pin;
//    cnc.m[1].limit     = HOME_Y;
//    cnc.m[1].limit_inv = 0;
    cnc.m[1].tim       = TIM_ROTATE;
    cnc.m[1].mm        = 444;  // pulses / degree
    cnc.m[1].home_ofs  = 200;
    cnc.m[1].x_max     = 200000;//25000; // 0.01 mm/sec^2
    cnc.m[1].Anom      = 50000; //100000; // 0.01 mm/sec^2
    cnc.m[1].Vnom/*[CNC_SPEED_NORMAL]*/ = 2000; //28000; // 0.01 mm/sec
    //cnc.m[1].Vnom[CNC_SPEED_SLOW  ] = 1000; // 0.01 mm/sec



    CNC_Init();
}
////////////////////////////////////////////////////////////////////////////////

