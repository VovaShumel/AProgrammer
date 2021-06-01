#ifndef __CONTROLS_H_INCLUDED__
#define __CONTROLS_H_INCLUDED__

#include "inc.h"
											// Данные
typedef struct {
                                            // OPTIMIZE_POSSIBLE как вариант проверить, мож будет быстрее доступ, если эти переменные будут структурой, и доступ к массиву таких структур?
	TButtonState stateButton[BUTTON_COUNT]; // Состояние кнопки
	TBool bControlsFree[BUTTON_COUNT];      // Признак, что подсистема управления свободна (можно засыпать)
                                            // Т.е., что кнопка отпущена уже относительно долгое время (около секунды)
	TCounter cntRepeatEvents[BUTTON_COUNT]; // Счетчик повторных событий (нажатий кнопки или поворотов кольца)
} TControlsData;


extern TControlsData dataControls;			// Данные подсистемы управления

#define IsButtonPressed(N)         (dataControls.stateButton[N] & 0x01)

#define GetRepeatEvents(N)			dataControls.cntRepeatEvents[N]         // Доступ к счетчику событий
#define ResetRepeatEvents(N)		dataControls.cntRepeatEvents[N] = 0
#define FirstRepeatEvents(N)		dataControls.cntRepeatEvents[N] = 1
#define IncRepeatEvents(N)			dataControls.cntRepeatEvents[N]++

#define ButtonFree(N)				dataControls.bControlsFree[N] = TRUE    // Разрешение/запрет спячки
#define ButtonBusy(N)				dataControls.bControlsFree[N] = FALSE
#if BUTTON_COUNT == 2
    #define IsControlsFree()		dataControls.bControlsFree[0] && dataControls.bControlsFree[1]
#else
    #define IsControlsFree()		dataControls.bControlsFree[0]
#endif

#define GetSlowTurboRaiseDiscret()  dataControls.slowTurboRaiseDiscret
#define SetSlowTurboRaiseDiscret(D) dataControls.slowTurboRaiseDiscret = (D)

																				// Функции
void ControlsInit(void);
void ControlsAwake(void);														// Перенастройка параметров после пробуждения

TEvent ControlsCheck(void);														// Проверка управляющих элементов, возвращает возникшее событие

#endif /*__CONTROLS_H_INCLUDED__*/
