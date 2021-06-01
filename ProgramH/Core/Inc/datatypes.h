#ifndef INC_DATATYPES_H_
#define INC_DATATYPES_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef TRUE
	#define TRUE	1
#endif

#ifndef FALSE
	#define FALSE	0
#endif

#ifndef NULL
	#define NULL	0
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef bool TBool;		// TODO глянуть в компилере есть бит?
typedef unsigned char TCounter;			// Счетчик
typedef TCounter TIndex;				// Индекс перечисления (совпадает со счетчиком)
typedef unsigned char TMask;			// Слово битовой маски
typedef unsigned char TUData;			// Универсальный беззнаковый тип для разных данных - байт
typedef unsigned short TUSData;			// Универсальный беззнаковый тип для разных данных - два байта
typedef unsigned long TULData;          // 4 байта без знака

typedef TIndex TTimerID;				// Идентификатор таймера
typedef unsigned short TTicks;			// Счетчик тиков системного прерывания
typedef unsigned short TTimeMs;			// Счетчик миллисекунд
typedef unsigned char TTimeMin;			// Счетчик минут
typedef unsigned char TButtonState;		// Состояние кнопки

typedef unsigned char TInfoColor;		// Тип одного цветового канала ИСИД
typedef unsigned char TInfoColors;		// Тип суммарного свечения для ИСИД
typedef unsigned char TEvent;			// Тип события
typedef unsigned char TStage;			// Стадия управления фонарем
typedef unsigned char TAlarmStatus;		// Тип, описывающий состояния предупредительных триггеров
typedef unsigned char TStatus;          // Статус какого-либо события

//Типы для скрипта
typedef unsigned char TLSCommand;		// Команда скрипта (может быть вместо идентификатора)
typedef unsigned char TLSParam;			// Параметр команд
typedef TLSParam TLSId;					// Идентификатор скрипта
typedef TLSParam TLSVariable;			// Внутренняя переменная
typedef TLSParam TLSLoop;				// Счетчик циклов
typedef TLSParam TLSIP;					// Счетчик команды (Instruction Pointer)
typedef TLSParam TLSTime;				// Время выполнения команды, расшифровку типа см. lightscripts.h

typedef unsigned char	TEEPROMAddr;	// Адрес памяти EEPROM
typedef unsigned char	TEEPROMData;	// Тип данных EEPROM

#endif /* INC_DATATYPES_H_ */
