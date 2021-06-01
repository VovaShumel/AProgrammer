#include "lowlevel.h"

												// Данные нижнего уровня, вызываемые из верхнего

TLowLevelExportData dataLowLevelExport;			// Экспортные данные

volatile struct {								// Оперативные данные
#ifdef HAS_COLOR_INDICATOR
	TCounter cntIndicator;						// Счетчик тиков для индикации
#endif
	TTimeMs msNewTime;							// Время в мс между опросами
	TTimeMs msDelay;							// Счетчик для организации задержек в миллисекундах
	TCounter cntSwitchAntibounce[BUTTON_COUNT];	// Счетчик расчета антидребезга кнопки
#ifdef HAS_COLOR_INDICATOR
	TInfoColor iledRed;							// Цветовые каналы индикаторного диода
	TInfoColor iledGreen;
#endif
} dataLowLevel;

void MakeDelayMS(TTimeMs ms) {
  dataLowLevel.msDelay = ms;
  while(dataLowLevel.msDelay);                  // Ожидать, пока переменная не обнулится из прерывания
}

void Pulse(TTimeMs ms) { // Выдаёт высокий уровень на PB10 заданный интервал мс
//	HW_SetOutHi();
//	MakeDelayMS(ms);
//	HW_SetOutLo();
}

void Handle_1ms_Timer(void) {
	dataLowLevel.msNewTime++;

	if (dataLowLevel.msDelay)      // Задана пауза?
		dataLowLevel.msDelay--;

	TBool bContact;

    for (TIndex button = 0; button < BUTTON_COUNT; button++) {
		//bContact = HW_IsBtnPressed();

        if (bContact != GetButtonContact(button)) {
            dataLowLevel.cntSwitchAntibounce[button]++;
            if (bContact) {
                if (dataLowLevel.cntSwitchAntibounce[button] > 30)
                    SetButtonContact(button, TRUE);
            } else
                if (dataLowLevel.cntSwitchAntibounce[button] > 15)
                    SetButtonContact(button, FALSE);
        } else
            dataLowLevel.cntSwitchAntibounce[button] = 0;
    }
}
