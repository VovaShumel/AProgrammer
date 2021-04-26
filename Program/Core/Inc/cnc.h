#ifndef CNC_H
#define CNC_H

#include "main.h"

#define MC 2               /* motor/axis count */
#define STEP_PULSE_WIDTH 3 /* microseconds */

typedef int32_t TCoord;

typedef TCoord TCoords[MC];

typedef enum
{
  CNC_SPEED_NORMAL,
  CNC_SPEED_SLOW,
  CNC_SPEED_VERY_SLOW,
  
  CNC_SPEED_LAST
} CNC_SPEED_TypeDef;

typedef struct
{
    GPIO_TypeDef * dir_port;
	uint16_t       dir_pin;
    GPIO_TypeDef * step_port;
	uint16_t       step_pin;
//    DIN_TypeDef  limit;
    TIM_TypeDef  * tim;
    int          dir_inv;
    int          dir_delay; // delay in CNC_Del() count after DIR changing and pulses starting
    int          limit_inv;

    TCoord home_ofs;
    TCoord x_max;
    int    mm; // pulses per mm
    int    Anom; // acceleration in pulses / sec^2
    int    Vnom/*[CNC_SPEED_LAST]*/; // speeds for different speed modes
} MOTOR_TypeDef;

typedef struct
{
    MOTOR_TypeDef      m[MC]; // 0 - x, 1 - y, 2 - z
    GPIO_TypeDef * enable_port;
	uint16_t       enable_pin;

    TIM_TypeDef  * del_tim;
} CNC_TypeDef;

extern CNC_TypeDef cnc;

// инициализация:
// - задаются:
//     - DOUT-сигналы управления
//     - макс. скорости/ускорения, габариты стола, макс. z для запрета перемещения по x,y
//     - таймер/прерывания
//     - правила: RULE_FAST, RULE_SLOW и т.д.
//           - 
//           - скорость
void CNC_Init();

// определяет нули на малых скоростях: 
// сначала Z, а потом одновременно x и y 
// при обнаружении нуля останавливается с замедлением
int CNC_FindHomes();

// перемещает в точку x,y,z с/без проверки концевиков; 
// функция завершается после остановки двигателей
int CNC_GoTo(TCoords c, int check_homes, int * to_stop);

// запускает задачу на перемещение в точку x,y,z и сразу же возвращается;
// для выполнения задачи пользователь должен вызывать CNC_Update() до значения CNC_TaskFinished() != 0
void CNC_Start_GoTo(TCoords c);
void CNC_Start_GoTo_i(int i, TCoord * c);

// устанавливает предопределенную скорость движения всех двигателей
//void CNC_SetSpeedSet(CNC_SPEED_TypeDef cnc_speed);

void CNC_SetSpeed_i(int i, int speed);


// останавливает двигатели с замедлением без возврата в точку останова
void CNC_Stop();
void CNC_Stop_i(int i);

// останавливает двигатели с замедлением и возвращает в точку останова
void CNC_StopHere();

// резко останавливает двигатели без замедления
void CNC_Halt();

// генерирует импульсы на двигатели
void CNC_Update();
void CNC_Update_i(int i);

// возвращает текущую координату
TCoord CNC_GetCoord(int i);

void CNC_ResetCoord(int i);

// возвращает 1, если поставленная задача выполнена, иначе 0
int CNC_TaskFinished();
int CNC_TaskFinished_i(int i);

// 1, если достиг концевика home
int CNC_AtHome(int i);

void CNC_ISR(int i);


#endif
