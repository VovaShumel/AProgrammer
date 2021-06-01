#ifndef INC_LOWLEVEL_H_
#define INC_LOWLEVEL_H_

#include "inc.h"

typedef struct {				// Интерфейсные данные, получаемые из нижнего уровня, но необходимые верхнему
	TBool button[BUTTON_COUNT];	// Состояние кнопки (нажата/отжата)
} TLowLevelExportData;

extern TLowLevelExportData dataLowLevelExport;

#define GetButtonContact(B)		dataLowLevelExport.button[B]
#define SetButtonContact(B, C)	dataLowLevelExport.button[B] = (C)

void MakeDelayMS(TTimeMs ms);
void Pulse(TTimeMs ms);

void Handle_1ms_Timer(void);

#endif /* INC_LOWLEVEL_H_ */
