#include "controls.h"

#define STATE_BUTTON_OPENED				0
#define STATE_BUTTON_CLOSED				1
#define STATE_BUTTON_WAITING_MCLICK		2
#define STATE_BUTTON_HOLD				3
#define BUTTON_GET_CONTACT(S)			(TBool)((S) & 0x01)

#define STATE_RING_OPENED				0
#define STATE_RING_CLOSED				1

TControlsData dataControls;

void ControlsInit(void) {
	if (GetButtonContact(0))
		dataControls.stateButton[0] = STATE_BUTTON_CLOSED; // Если во время подачи питания на драйвер кнопка нажата,
	else
		dataControls.stateButton[0] = STATE_BUTTON_OPENED;

    ControlsAwake();
}

void ControlsAwake(void) {
    for (TIndex button = 0; button < BUTTON_COUNT; button++) {
        ResetRepeatEvents(button);
        ButtonBusy(button);	
        StartTimerSW(TIMER_STATE_BUTTON_1);   // Запустить таймер для отработки разрешения спящего режима
    #if BUTTON_COUNT > 1
        StartTimerSW(TIMER_STATE_BUTTON_2);
    #endif
    }
}

TEvent ControlsCheck(void) {		// Проверка управляющих элементов, возвращает возникшее событие
	TEvent idEvent = EVENT_NONE;
    TBool contact;
    
    for (TIndex button = 0; button < BUTTON_COUNT; button++) {                              // Обработка программной кнопки
        contact = GetButtonContact(button);                                     

        if (contact == BUTTON_GET_CONTACT(dataControls.stateButton[button])) {              // Состояние контакта не изменилось	
            if (IsTimerWork(TIMER_STATE_BUTTON_1 + button)) {                               // Если таймер запущен, значит могут быть какие-то изменения		

                if (dataControls.stateButton[button] == STATE_BUTTON_OPENED)                // Проверка текущих состояний
                    if (CheckTimerSW(TIMER_STATE_BUTTON_1 + button, BREAK_LINK_TIME)) {
                        idEvent = EVENT_BUTTON_COMBS_OVER;
                        StopTimer(TIMER_STATE_BUTTON_1 + button);
                        ButtonFree(button);                                                 // Можно переходить в спящий режим
                    }

                if (dataControls.stateButton[button] == STATE_BUTTON_WAITING_MCLICK)
                    if (CheckTimerSW(TIMER_STATE_BUTTON_1 + button, REPEAT_CLICK_TIME)) {
                        idEvent = EVENT_BUTTON_MCLICK_OVER;
                        dataControls.stateButton[button] = STATE_BUTTON_OPENED;
                        ResetRepeatEvents(button);
                    }

                if (dataControls.stateButton[button] == STATE_BUTTON_CLOSED)
                    if (CheckTimerSW(TIMER_STATE_BUTTON_1 + button, HOLD_TIME)) {
                        dataControls.stateButton[button] = STATE_BUTTON_HOLD;
                        idEvent = EVENT_BUTTON_HOLD;
                        FirstRepeatEvents(button);
                        StartTimerSW(TIMER_STATE_BUTTON_1 + button);
                    }

                if (dataControls.stateButton[button] == STATE_BUTTON_HOLD)
                    if (CheckTimerSW(TIMER_STATE_BUTTON_1 + button, REPEAT_HOLD_TIME)) {
                        idEvent = EVENT_BUTTON_HOLD;
                        IncRepeatEvents(button);
                        StartTimerSW(TIMER_STATE_BUTTON_1 + button);
                    }
            }
        } else {                                                        // Состояние контакта изменилось
            if (contact) {                                              // Замкнули контакт?
                if (GetRepeatEvents(button) == 2)                       // Включение строба нужно реализовать не по клику, а именно по нажатию быстрому трёхкратному.
                    idEvent = EVENT_BUTTON_PRESS_AFTER_DOUBLE_CLICK;    // Не нужно ловить каждое нажатие, достаточно словить 2 клика, и нажатие после них
                else
                    idEvent = EVENT_BUTTON_PRESS;
                dataControls.stateButton[button] = STATE_BUTTON_CLOSED;
            } else {                                                    // Разомкнули контакт
                idEvent = EVENT_BUTTON_RELEASE;

                if (dataControls.stateButton[button] == STATE_BUTTON_CLOSED) {  // Проверить на клик

                    // Если от прошлого замыкания прошло немного времени
                    // Либо первые два клика могут быть медленнее
                    if (IsTimerInProgress(TIMER_STATE_BUTTON_1 + button, CLICK_TIME) || ((GetRepeatEvents(button) < 2) && IsTimerInProgress(TIMER_STATE_BUTTON_1 + button, HOLD_TIME))) {
                        idEvent = EVENT_BUTTON_CLICK;
                        dataControls.stateButton[button] = STATE_BUTTON_WAITING_MCLICK;
                        IncRepeatEvents(button);
                    } else
                        ResetRepeatEvents(button);
                }

                if (dataControls.stateButton[button] == STATE_BUTTON_HOLD)          // Если после удержания, то сбросить счетчик
                    ResetRepeatEvents(button);

                if (idEvent == EVENT_BUTTON_CLICK)                                  // Если был клик, ожидать повторного
                    dataControls.stateButton[button] = STATE_BUTTON_WAITING_MCLICK;
                else
                    dataControls.stateButton[button] = STATE_BUTTON_OPENED;
            }

            StartTimerSW(TIMER_STATE_BUTTON_1 + button);
            ButtonBusy(button);                                                     // Запретить спать
        }
    }

	return idEvent;
}

#if BUTTON_COUNT == 2
bit IsBothBtnsPressed(void) {
    return BUTTON_GET_CONTACT(dataControls.stateButton[0]) && BUTTON_GET_CONTACT(dataControls.stateButton[1]);
}

bit IsBothBtnsReleased(void) {
    return !(BUTTON_GET_CONTACT(dataControls.stateButton[0]) || BUTTON_GET_CONTACT(dataControls.stateButton[1]));
}
#endif
