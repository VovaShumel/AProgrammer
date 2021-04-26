#include "pic_prog.h"
#include "string.h"

#define     T_DLY                   1
#define     T_PINT_PROG             2500
#define     T_PINT_CONF             5000
#define     T_PINT_EEPROM           5000
#define     T_ENTH                  250
#define     T_ERAR                  2500
#define     T_ERAB                  5000
#define     T_PPDP                  5
#define     T_HLD0                  5
#define     T_DLY2                  1
#define     T_ERA                   10000
#define     T_PROG                  2000
#define     T_DIS                   100
#define     T_RESET                 10000

											//  PIC12F
#define     PIC12_PULSE_WIDTH       	5 	// Вес единицы младшего разряда — 100 нс

											// PIC10F22x
#define     PIC10_MAX_ADDR           0x1FF
#define     PIC10_USER_ID_ADDR       0x100
#define     PIC10_LOAD_DATA_CMD      0x02
#define     PIC10_READ_DATA_CMD      0x04
#define     PIC10_INC_ADDR_CMD       0x06
#define     PIC10_BEGIN_PROG_CMD     0x08
#define     PIC10_END_PROG_CMD       0x0E
#define     PIC10_BULK_ERASE_CMD     0x09
#define     PIC10_DATA_MASK         0xFFF
#define     PIC10_CONF_ADDR_IN_HEX  0xFFF
#define     PIC10_PULSE_WIDTH          10 	// Вес единицы младшего разряда — 100 нс

											// PIC16F153XX
#define PIC16_T_ENTS            	   1
#define PIC16_T_DIS             	 300
#define PIC16_T_ERAB            	8400
#define PIC16_T_ERAR            	2800
#define PIC16_T_PINT_PFM        	2800
#define PIC16_T_PINT_CONF       	5600

#define PIC16_PULSE_WIDTH         	   1 	// Вес единицы младшего разряда — 100 нс
#define PIC16_USER_ID_ADDR        0x8000
#define PIC16_DEVICE_ID_ADDR      0x8006
#define PIC16_CONF_WORDS_ADDR     0x8007

#define PIC16_CONF_WORDS_HEX_ADDR 0x0007 /* in HEX-file at segment section 0001 (0x10000): 0x8007*2= 1 000E */
#define PIC16_USED_ID_HEX_ADDR    0x0000 /* in HEX-file at segment section 0001 (0x10000): 0x8000*2= 1 0000 */


#define     MaxLatchNumber            32 // Максимальное количество защёлок, которое может быть занято до записи данных

#define     ADRES                   0x01 // позиция адреса в строке прошивки
#define     AMOUNT_OF_BYTES         0x00 // позиция количества байт в строке прошивки
#define     FIRST_BYTE_PLACE        0x03 // позиция первого байта данных в строке прошивки
#define     EEPROM_OFS            0xE000
#define     CONFIG_WORDS_OFS      0x000E

#define     LoadConfiguration       0x00
#define     LoadDataForProgMem      0x02
#define     LoadDataForDataMem      0x03
#define     ReadDataFromProgMem     0x04
#define     ReadDataFromDataMem     0x05
#define     IncrementAdres          0x06
#define     ResetAdres              0x16
#define     BeginIntTimedProg       0x08
#define     BeginExtTimedProg       0x18
#define     EndExternTimedProg      0x0A
#define     BulkEraseProgMem        0x09
#define     BulkEraseDataMem        0x0B
#define     RowEraseProgMem         0x11

#ifdef PIC_USE_SPI
	SPI_TypeDef * spi;
#endif

#define _PIC_SPI_ENABLE  spi->CR1 |= SPI_CR1_SPE
#define _PIC_SPI_DISABLE spi->CR1 &= ~SPI_CR1_SPE
#define _PIC_SPI_MODE_TX spi->CR1 |= SPI_CR1_BIDIOE
#define _PIC_SPI_MODE_RX spi->CR1 &= ~SPI_CR1_BIDIOE

#define _PIC_SPI_DataSize6  spi->CR2  = ((spi->CR2) & ~SPI_CR2_DS) | SPI_CR2_DS_2 | SPI_CR2_DS_0   
#define _PIC_SPI_DataSize16 spi->CR2  = ((spi->CR2) & ~SPI_CR2_DS) | SPI_CR2_DS_3 | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0   

extern HEX_Line_TypeDef * FW_CACHE_Get_Flash_Line (int lineNumber);
extern HEX_Line_TypeDef * FW_CACHE_Get_EEPROM_Line(int lineNumber);
extern void PIC_VCC_Off();
extern void PIC_VCC_On();

uint16_t picAdres;
uint8_t cntLatch  = 1;
int pulse_width;

#ifdef PIC_USE_SPI
    void PIC_WriteSPI_8(uint8_t data){
        while((spi->SR & SPI_SR_TXE) != SPI_SR_TXE){}; // проверка что регистр пуст
        *(uint8_t *)&(spi->DR) = data;                 // записываем даные в регистр
        while((spi->SR & SPI_SR_BSY) == SPI_SR_BSY){};   
    }

    void PIC_WriteSPI_16(uint16_t data) {
        while((spi->SR & SPI_SR_TXE) != SPI_SR_TXE){}; // проверка что регистр пуст
        spi->DR = data;
        while((spi->SR & SPI_SR_BSY) == SPI_SR_BSY){};   
    }

    uint16_t PIC_ReadSPI (void) {
       __disable_irq ();
       uint16_t temp = 0;
       spi->CR2 |=  SPI_CR2_RXNEIE;  
       while((spi->SR & SPI_SR_RXNE) != SPI_SR_RXNE);  // ожидаем приема данных
       _PIC_SPI_MODE_TX;
       temp = (uint16_t)spi->DR;                       // читаем данные и сбрасываем флаг приема
       __enable_irq ();
       return temp;
    }
#else
	#define _PULSE_WIDTH_DELAY Delay_01us(pulse_width) /* 5 = 0.5 us */

    void PIC_Write(uint16_t b, int bits) {
      for (int i=0; i<bits; i++) {
        _SET_GPIO_OUTPUT(PIC_DAT_PORT, PIC_DAT_PIN, b & 1);
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 1);
        _PULSE_WIDTH_DELAY;
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 0);
        _PULSE_WIDTH_DELAY;
        b >>= 1;
      }
    }

    void PIC_WriteMSB(uint32_t b, int bits) {
      for (int i=bits-1; i>=0; i--) {
        _SET_GPIO_OUTPUT(PIC_DAT_PORT, PIC_DAT_PIN, b & (1<<i));
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 1);
        _PULSE_WIDTH_DELAY;
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 0);
        _PULSE_WIDTH_DELAY;
      }
    }

    inline uint32_t PIC_Read(int bits) {
      _SET_GPIO_MODE_INPUT(PIC_DAT_PORT, PIC_DAT_PIN);
      RESET_DOUT(PIC_DAT_DIR);

      uint32_t res = 0;

      for (int i=0; i<bits; i++) {
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 1);
        _PULSE_WIDTH_DELAY;
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 0);
        res |= _GET_GPIO_INPUT(PIC_DAT_PORT, PIC_DAT_PIN) << i;
        _PULSE_WIDTH_DELAY;
      }

      SET_DOUT(PIC_DAT_DIR);
      _SET_GPIO_MODE_OUTPUT(PIC_DAT_PORT, PIC_DAT_PIN);
      return res;
    }

    inline uint32_t PIC_ReadMSB(int bits) {
      _SET_GPIO_MODE_INPUT(PIC_DAT_PORT, PIC_DAT_PIN);
      RESET_DOUT(PIC_DAT_DIR);

      uint32_t res = 0;

      for (int i=0; i<bits; i++) {
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 1);
        _PULSE_WIDTH_DELAY;
        _SET_GPIO_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN, 0);
        res = (res << 1) | _GET_GPIO_INPUT(PIC_DAT_PORT, PIC_DAT_PIN);
        _PULSE_WIDTH_DELAY;
      }

      SET_DOUT(PIC_DAT_DIR);
      _SET_GPIO_MODE_OUTPUT(PIC_DAT_PORT, PIC_DAT_PIN);
      return res;
    }

    uint16_t PIC_Read16() {
    	return PIC_Read(16);
    }
#endif

void PicSendCommand (uint8_t comm) {	// Отправка команды пику
	#ifdef PIC_USE_SPI
		_PIC_SPI_MODE_TX;
		_PIC_SPI_DataSize6;      		// устанавливаем длинну слова 6 бит
		PIC_WriteSPI_8(comm);
	#else
	  PIC_Write(comm, 6);
	#endif
}

void PicSendData (uint16_t data) {	// Отправка данных
    data <<= 1;           			// Освобождаем младший разряд, потому что первый бит стартовый и равен нулю
    data &= 0xFFFE;       			// обнуляем младший бит (TODO может не нужно?)
#ifdef PIC_USE_SPI
    _PIC_SPI_DataSize16;  			// устанавливаем длинну слова 16 бит
    Delay_us(T_DLY);   				// ждем паузу Tdly между командой и данными
    PIC_WriteSPI_16(data);
#else
    Delay_us(T_DLY);   				// ждем паузу Tdly между командой и данными
    PIC_Write(data, 16);
#endif
}

uint16_t PicReadData (void)	{	// Чтение данных
    uint16_t temp = 0;

	#ifdef PIC_USE_SPI
		_PIC_SPI_DataSize16; 	// устанавливаем длинну слова 16 бит
		_PIC_SPI_MODE_RX;

		temp = PIC_ReadSPI(); 	// получаем данные                      TODO добавить контроль времени ожидания
	#else
		Delay_us(T_DLY);   		// ждем паузу Tdly между командой и данными
		temp = PIC_Read16();
	#endif
    
    return temp;
}

// Загрузка конфигурации по слову в защелку данных и устанавливает адрес 8000. Далее нужно использовать команду инкрементации адреса для достижения нужного значения.
// Затем применить команду BeginIntTimedProg или BeginExtTimedProg для записи слова из защелки в память. После этого единственный способ вернуться к нулевому? адресу
// это либо выйти из режима программирования и снова зайти, или использовать команду сброса адреса
void PicLoadConfig (uint16_t data) {
    PicSendCommand(LoadConfiguration);
    PicSendData(data);
}

volatile int lock = 1;

void PicLoadDataForProgMem (uint16_t data) { // Загрузка данных в защелки памяти программ (14 бит)
    if (data > 0x3FFF) while (lock); 		 // значение не может быть > 0x3FFF
    PicSendCommand(LoadDataForProgMem);
    PicSendData(data);
}

void PicLoadDataForDataMem (uint8_t data) { // Загрузка данных в защелки памяти данных (8бит)
    PicSendCommand(LoadDataForDataMem);
    PicSendData(data);
}

uint16_t PicReadDataFromProgMem (void) { // Чтение данных из памяти программ (14 бит). Если включен бит защиты памяти, то "прочтутся" одни нули
    uint16_t temp = 0;
   
    PicSendCommand(ReadDataFromProgMem);
    //Delay_us(5);
    temp = PicReadData();
    
    temp >>=1;  			// Удаляем стартовый бит. Стоповый бит 0 и не изменит полученное значение
    temp &= 0x3FFF;
    
    return temp;
}

uint8_t PicReadDataFromDataMem (void) { // Чтение данных из памяти данных (8 бит). Если включен бит защиты памяти, то "прочтутся" одни нули
    uint16_t temp = 0;
   
    PicSendCommand(ReadDataFromDataMem);
    //Delay_us(5);
    temp = PicReadData();
    
    temp >>=1;  			// Удаляем стартовый бит. Стоповый бит 0 и не изменит полученное значение
    temp &= 0x3FFF;
    
    return temp;
}

// Увеличение текущего адреса
// Если применить при текущем адресе 7FFF -> счетчик дареса обнулится.
// Если применить при текущем адресе FFFF -> счетчик дареса установится на 8000
void PicIncrementAdres (void) {
    PicSendCommand(IncrementAdres);
    Delay_us(T_DLY);
}

//// сброс счетчика адреса. значение адреса обнулится не зависимо от того какое значение было установлено ранееы
//void PicResetAdres (void)
//{
//    PicSendCommand(ResetAdres);
//}

void PicBeginInternTimedProg (int delay) {	// Запись данных, сохранённых в защёлках, внутреннее тактирование
    PicSendCommand(BeginIntTimedProg);
    Delay_us(delay); // после применения команды программирования нужно выждать паузу Tpint = 2.5/5 милиСек, для того чтоб процесс програмирования успел завершииться
}

//// функция записи данных сохраненных в защлках, внешнее тактирование
//void PicBeginExternTimedProg (void)
//{
//    PicSendCommand(BeginExtTimedProg);
//}

//// функция завершающая запись данных сохраненных в защелках при внешнем тактировании
//void PicEndExternTimedProg (void)
//{
//    PicSendCommand(EndExternTimedProg);
//}

// функция стирания данных.
// если текущий адрес 0000-7FFF, то стирается :  память программ, слово конфигурации и если CPD = 0 то стирается и память данных
// если текущий адрес 8000-8008, то стирается :  память программ, слово конфигурации, ID пользователя и если CPD = 0 то стирается и память данных
// если текущий адрес выше 8008, то стирание нельзя применять
// после применения команды должно пройти время Terab для полного завершения процесса стирания
//void PicBulkEraseProgMem (void)
//{
//    PicSendCommand(BulkEraseProgMem);
//}

// функция стирания памяти данных. если CPD = 0 (защита кода активна), то для стирания нужно использовать BulkEraseProgMem()
//void PicBulkEraseDataMem (void)
//{
//    PicSendCommand(BulkEraseProgMem);   
//}

// функция стирания блока памяти. размер блока 32 защелки. если текущий адрес 8000-8008, то  сотрется только ID
//void PicRowEraseProgMem (void)
//{
//    PicSendCommand(RowEraseProgMem);
//}

void Reset_PIC() { 	// Правильная последовательность перехода в режим программирования для стенда драйверов
  PIC_VCC_Off();
  _SET_MCLR_0V; 	// прижимаем MCLR к земле чтоб установился ноль

  Delay_us(100);
  

  _SET_MCLR_9V;		// Подать питание Vihh
  Delay_us(200); 	// 10
  PIC_VCC_On();
  Delay_us(T_ENTH); 

  picAdres = 0;
}

void Reset_PIC10() {
	PIC_VCC_Off();
	_SET_MCLR_0V;

	Delay_us(T_RESET);

	PIC_VCC_On();
	Delay_us(T_PPDP);
	_SET_MCLR_9V;
	Delay_us(T_HLD0);

	picAdres = PIC10_MAX_ADDR;
}

void Reset_PIC16() {
	PIC_VCC_Off();
	_SET_MCLR_0V;

	Delay_us(T_RESET);

	PIC_VCC_On();
	Delay_us(PIC16_T_ENTS);
	_SET_MCLR_9V;
	Delay_us(T_ENTH);
}

volatile int pic_id_ = -1;

int PicCheckIdSeq (uint16_t pic_id) { // Проверка ID камня
  uint16_t id = 0;
  
  Reset_PIC(); 			 // VppFirst();
  
  //Delay(100); // задержка 100 милиСек
  
  	  	  	  	  	  	  	  	 // Получение ID из регистра 0x8006
  PicLoadConfig(0x00);           // Текущий адрес устанавливается 0x8000 ..
  PicIncrementAdres();   		 // 0x8001
  PicIncrementAdres();   		 // 0x8002
  PicIncrementAdres();   		 // 0x8003
  PicIncrementAdres();   		 // 0x8004
  PicIncrementAdres();   		 // 0x8005
  PicIncrementAdres();   		 // 0x8006
  id = PicReadDataFromProgMem();
  id >>=5;
  
  pic_id_ = id;
  
//  Delay(6);
 
  if(id == pic_id) return TRUE;  // ID по даташиту
  return FALSE;
}

void PicRowEraseSeq (void) { // Последовательность стирания данных RowErase перед началом программирования
  Reset_PIC(); 
  
  //Delay(100); // задержка 100 милиСек
  
  PicLoadConfig(0x00);              // TODO не совсем ясно зачем, но PicKit2 так делает
  PicSendCommand(RowEraseProgMem);  // стираем память программ
  Delay_us(T_ERAR);
}

void PicBulkEraseSeq (void) {	// последовательность стирания BulkErase
  Reset_PIC();
  
  //Delay(100); // задержка 100 милиСек
  
  PicLoadConfig(0x00);              // не совсем ясно зачем, но PicKit2 так делает
  
  PicSendCommand(BulkEraseProgMem); // стираем память программ
  Delay_us(T_ERAB);
  
  PicSendCommand(BulkEraseDataMem); // стираем память данных
  Delay_us(T_ERAB);
  
}

#define _LINE_ADDR ((((uint16_t)((*HEX_Line)[ADRES])) << 8) + ((uint16_t)((*HEX_Line)[ADRES+1])))

void PicAdresIncrement_WithProg(int delay) { // увеличивает текущий адрес в пике и контролирует количество задействованых защелок
    if(cntLatch == MaxLatchNumber) {
        cntLatch = 1;
        PicBeginInternTimedProg(delay);
    } else
        cntLatch++;

    PicIncrementAdres();
    picAdres++;
}

volatile int pic_written = 0;
int pic_res = 0;
int pic_res2 = 0;

void PicWriteFirmware (uint16_t lines) { // Заливка прошивки в пик. В качестве аргумента массив прошивки и количество строк в ней
  uint16_t   lineNumber      = 0;        // Номер строки, данные которой обрабатываются
  uint16_t   progAdres       = 0;        // Переменная для хранения адреса в коде
  uint8_t    dataBytesAmount = 0;        // Количество байт данных в текущей строке
  uint16_t   byteToWrite     = 0;
  uint8_t    byteCnt;                    // счетчик байт в цикле

  HEX_Line_TypeDef * HEX_Line;
  
  Reset_PIC();

  //Delay(100);
  
  while( lineNumber < lines )
  {
    HEX_Line = FW_CACHE_Get_Flash_Line(lineNumber);

    progAdres  = _LINE_ADDR; // берем значение адерса указанное в строке хекс файла
    progAdres /= 2;     	 // делим на два, потому что в хекс файле номер адреса увеличивается для каждого байта. но
                        	 // в пике одна ячейка памяти содержит два байта ( максимальное значение 3FFF), поэтому реальный адрес вдвое меньше
                        	 // TODO добавить проверку чтоб адрес делился на два ?
    
    											 // Значение реального адреса в начале строки не совпадает со значением адреса установленным в пике -> увеличиваем значение, пока не получим нужное
    while (picAdres != progAdres)
    	// PicIncrementAdres();                  // инкрементируем адрес в камне
    	PicAdresIncrement_WithProg(T_PINT_PROG);
    
    												// Инициализируем переменные перед циклом
    dataBytesAmount = (*HEX_Line)[AMOUNT_OF_BYTES]; // количество байт данных в текущей строке хекс файла
    uint16_t byteIndex = FIRST_BYTE_PLACE; 			// индекс текущего байта в текущей строке
    
    												// В одну ячейку памяти записывается пара байт.
    												// Байт с меньшим адресом, согласно хекс файлу, пишется в младшую половину ячейки.
    byteCnt = 0; 
    while( byteCnt < dataBytesAmount )
    {
        byteToWrite   = (*HEX_Line)[byteIndex + byteCnt + 1];	// выбираем байт котрый будем записывать в младшую половину
        byteToWrite <<= 8;
        byteToWrite  |= (*HEX_Line)[byteIndex + byteCnt];       // выбираем байт для старшего разряда
        byteCnt++;
        byteCnt++;
        PicLoadDataForProgMem(byteToWrite);                     // записываем байт
        PicAdresIncrement_WithProg(T_PINT_PROG);                // увеличиваем адрес по которому будет записан следующий байт в камне
        pic_written++;
    }
    lineNumber++;
    
  }

  if (cntLatch != 0)
	  PicBeginInternTimedProg(T_PINT_PROG); // если в защелка остались не записанные данные, то запускаем запись
}

void PicWrireEEPROM (uint16_t lines) {
    uint8_t      lineNumber      = 0;
    uint16_t     byteToWrite     = 0;
    uint8_t      dataBytesAmount = 0;
    uint8_t      byteCnt         = 0;

    HEX_Line_TypeDef * HEX_Line;
  

    Reset_PIC();						// подать питание Vihh

    // задержка 100 милиСек
    //Delay(100);
    
    HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    

    while ((lineNumber < lines) && (_LINE_ADDR < EEPROM_OFS)) { // find data with addr 0xE000
      lineNumber++;
      HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    }
    
    while ((lineNumber < lines) && (_LINE_ADDR >= EEPROM_OFS)) {
        HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
   
        														// инициализируем переменные перед циклом
        dataBytesAmount = (*HEX_Line)[AMOUNT_OF_BYTES];        // количество байт данных в текущей строке хекс файла
        uint8_t byteIndex = FIRST_BYTE_PLACE;
        
        byteCnt = 0; 
        while( byteCnt < dataBytesAmount )
        {
            byteToWrite = (*HEX_Line)[byteIndex + byteCnt];
            PicLoadDataForDataMem(byteToWrite);
            //Delay_us(10);
            PicBeginInternTimedProg(T_PINT_EEPROM);
            //Delay_us(3000); 						// Увеличиваем время, потому что для записи в EEPROM нужно 5 мс
            PicIncrementAdres();
            //Delay_us(10);
            byteCnt++;
            byteCnt++; 								// Второй байт в строке хекс файла 0х00, он незначащий, его пропускаем
        }
        lineNumber++;    
    }
}

void PicWrireUserID (uint16_t lines) { // запись в память даных
  uint8_t      lineNumber      = 0;
  uint16_t     byteToWrite     = 0;
  uint8_t      dataBytesAmount = 0;
  uint8_t      byteCnt         = 0;
 
  HEX_Line_TypeDef * HEX_Line;
  
  Reset_PIC();
  //Delay(100);

  	  	  	  	  	  	  	  	  	  // эта команда приводит к установке адреса 0х8000
  PicLoadConfig(0x00);                // текущий адрес устанавливается 0x8000 ..

  HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    
  while ((lineNumber < lines) && (_LINE_ADDR != 0x0000)) { // find USER ID data
    lineNumber++;
    HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
  }
  
  if (_LINE_ADDR == 0x0000) {
	  	  	  	  	  	  	  	  	  	  	  	  	  // инициализируем переменные перед циклом
    dataBytesAmount  = (*HEX_Line)[AMOUNT_OF_BYTES];  // количество байт данных в текущей строке хекс файла
    uint8_t byteIndex = FIRST_BYTE_PLACE;
    
    byteCnt = 0; 
    while (byteCnt < dataBytesAmount) {
        byteToWrite = (*HEX_Line)[byteIndex + byteCnt];

        PicLoadDataForProgMem(byteToWrite); // передаем данные в защелку
        //Delay_us(10);
        PicBeginInternTimedProg(T_PINT_CONF); // инициируем запись данных в память
        PicIncrementAdres();

        byteCnt++;
        byteCnt++; // второй байт в строке хекс файла 0х00, он не значащий и его пропускаем
    }
  }
}

uint16_t test1;
uint16_t test2;

int PicVerifyProgramMemory (uint16_t lines) { // Верификация данных памяти программ пика. Читает и сравнивает с массивом в прошивке
    uint16_t   lineNumber      = 0;           // номер строки данные которой обрабатываются
    uint16_t   progAdres       = 0;           // переменная для хранения адреса в коде
    uint16_t   picDataAdres    = 0;           // адрес регистра с которого считываются данные
    uint8_t    dataBytesAmount = 0;           // количество байт данных в текущей строке
    uint16_t   byteToCheck     = 0;
    uint16_t   byteRceived     = 0;
    uint8_t    byteCnt;                       // счетчик байт в цикле
   
    HEX_Line_TypeDef * HEX_Line;
    
    Reset_PIC();

    // задержка 100 милиСек
    //Delay(100);
    
    while(lineNumber < lines) {
        HEX_Line = FW_CACHE_Get_Flash_Line(lineNumber);
        
        									// получаем значение рельного адреса указанного в хекс файле
        progAdres  = _LINE_ADDR; 			// берем значение адерса указанное в строке хекс файла
        if(progAdres != 0) progAdres /= 2;  // делим на два, потому что в хекс файле номер адреса увеличивается для каждого байта. но
                                            // в пике одна ячейка памяти содержит два байта ( максимальное значение 3FFF), поэтому реальный адрес вдвое меньше
                                            // TODO добавить проверку чтоб адрес делился на два ?

        while (picDataAdres < progAdres) {  // Значение реального адреса в начале строки не совпадает со значением адреса установленным в пике -> то увеличиваем значение пока не получим нужное
             PicIncrementAdres();           // инкрементируем адрес в камне
             picDataAdres++;
        }
        												// инициализируем переменные перед циклом
        dataBytesAmount = (*HEX_Line)[AMOUNT_OF_BYTES]; // количество байт данных в текущей строке хекс файла
        uint16_t byteIndex = FIRST_BYTE_PLACE; 			// индекс текущего байта в текущей строке
        
        byteCnt = 0; 
        while (byteCnt < dataBytesAmount) {
            byteToCheck   = (*HEX_Line)[byteIndex + byteCnt + 1]; // выбираем байт котрый будем записывать в младшую половину
            byteToCheck <<= 8;
            byteToCheck  |= (*HEX_Line)[byteIndex + byteCnt];     // выбираем байт для старшего разряда
            byteCnt++;
            byteCnt++;
   
            byteRceived = PicReadDataFromProgMem();
            
            if (byteToCheck != byteRceived) {
                pic_res2 = (lineNumber<<4) + (byteCnt-2);
                return FALSE;
            }
            //Delay_us(10);
            PicIncrementAdres();
            picDataAdres++;
        }
        lineNumber++;    
    }
    
    //PicVhighOff();
  
    return TRUE;
}

int PicVerifyEEPROM (uint16_t lines) { // верификация памяти данных
    uint8_t  lineNumber      = 0;
    uint16_t byteToCheck     = 0;
    uint16_t byteRceived     = 0;
    uint8_t  dataBytesAmount = 0;
    uint8_t  byteCnt         = 0;
    uint16_t progAdres       = 0;      // переменная для хранения адреса в коде
    uint16_t picDataAdres = 0xE000; // адрес регистра с которого считываются данные
    
    HEX_Line_TypeDef * HEX_Line;
  
    Reset_PIC();

    // задержка 100 милиСек
    //Delay(100);

    HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    
    while ((lineNumber < lines) && (_LINE_ADDR < EEPROM_OFS)) { // find data with addr 0xE000
      lineNumber++;
      HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    }

    while ((lineNumber < lines) && (_LINE_ADDR >= EEPROM_OFS)) {
        HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
     
        								// первый адрес должен быть равен 0xE000, потому что для начала считывания памяти данных нужно провести хотябы одну команду,
        								// а затем инкрементировать дарес до нужного значения
      
        								// получаем значение рельного адреса указанного в хекс файле
        progAdres = _LINE_ADDR; 		// берем значение адерса указанное в строке хекс файла
       
        if (progAdres < picDataAdres) {
             PicIncrementAdres();       // инкрементируем адрес в камне
             picDataAdres++;
        }
      
        												// инициализируем переменные перед циклом
        dataBytesAmount = (*HEX_Line)[AMOUNT_OF_BYTES]; // количество байт данных в текущей строке хекс файла
        uint8_t byteIndex = FIRST_BYTE_PLACE;
        
        byteCnt = 0; 
        while (byteCnt < dataBytesAmount) {
            byteToCheck = (*HEX_Line)[byteIndex + byteCnt];
            byteRceived = PicReadDataFromDataMem();
            
            test1 = byteToCheck;
            test2 = byteRceived;
            
            if(byteToCheck != byteRceived)
               return FALSE;

            PicIncrementAdres();
            byteCnt++;
            byteCnt++; // второй байт в строке хекс файла 0х00, он не значащий и его пропускаем
            //Delay_us(10);
        }
        lineNumber++;    
    }
    //PicVhighOff();
  
    return TRUE;
}

int PicVerifyUserID (uint16_t lines) { // верификация памяти данных
    uint8_t      lineNumber      = 0;
    uint16_t     byteToCheck     = 0;
    uint16_t     byteRceived     = 0;
    uint8_t      dataBytesAmount = 0;
    uint8_t      byteCnt         = 0;
    
    HEX_Line_TypeDef * HEX_Line;
  
    Reset_PIC();
    //Delay(100);

    					 // эта команда приводит к установке адреса 0х8000
    PicLoadConfig(0x00); // текущий адрес устанавливается 0x8000 ..

    HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    
    while ((lineNumber < lines) && (_LINE_ADDR != 0x0000)) { // find USER ID data
      lineNumber++;
      HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    }
  
    if (_LINE_ADDR == 0x0000) {
        dataBytesAmount  = (*HEX_Line)[AMOUNT_OF_BYTES]; // количество байт данных в текущей строке хекс файла
        uint8_t byteIndex = FIRST_BYTE_PLACE;
        
        byteCnt = 0; 
        while (byteCnt < dataBytesAmount) {
            byteToCheck = (*HEX_Line)[byteIndex + byteCnt];
            byteRceived = PicReadDataFromProgMem();
            
            if(byteToCheck != byteRceived) return FALSE;

            PicIncrementAdres();
            byteCnt++;
            byteCnt++; // второй байт в строке хекс файла 0х00, он не значащий и его пропускаем
        }
        lineNumber++;    
    }
  
    return TRUE;
}

int PicLoadConfigSeq (uint16_t lines) { // последовательность загрузки слов конфигурации
    uint8_t lineNumber = 0;
    HEX_Line_TypeDef * HEX_Line;
  
    HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    
    while ((lineNumber < lines) && (_LINE_ADDR < CONFIG_WORDS_OFS)) { // find USER ID data
      lineNumber++;
      HEX_Line = FW_CACHE_Get_EEPROM_Line(lineNumber);
    }
    
    if (_LINE_ADDR == CONFIG_WORDS_OFS) {
      uint8_t ind = FIRST_BYTE_PLACE;
      uint16_t tempWord;
      
      uint16_t firstWord =  ((uint16_t)((*HEX_Line)[ind  ])) + (((uint16_t)((*HEX_Line)[ind+1]))<<8);
      uint16_t secondWord = ((uint16_t)((*HEX_Line)[ind+2])) + (((uint16_t)((*HEX_Line)[ind+3]))<<8);
      
      firstWord  &= 0x3FFF; // 14 bits only
      secondWord &= 0x3FFF;
      
//      uint16_t firstWord  = 0x09AC;    // 0х09АС - бит защиты от записи-чтения не установлен
//      uint16_t secondWord = 0x1600;
    
      Reset_PIC();

      	  	  	  	  	   // эта команда приводит к установке адреса 0х8000
      PicLoadConfig(0x00); // текущий адрес устанавливается 0x8000 ..
      PicIncrementAdres(); // 0x8001
      PicIncrementAdres(); // 0x8002
      PicIncrementAdres(); // 0x8003
      PicIncrementAdres(); // 0x8004
      PicIncrementAdres(); // 0x8005
      PicIncrementAdres(); // 0x8006
      PicIncrementAdres(); // 0x8007

      	  	  	  	  	  	  	  	    // запись первого слова конфигурации ( CONFIGURATION WORD 1 )
      PicLoadDataForProgMem(firstWord); // передаем данные в защелку
      //Delay_us(20);
      PicBeginInternTimedProg(T_PINT_CONF); // инициируем запись данных в память

      tempWord = PicReadDataFromProgMem();

      if(tempWord != firstWord) return FALSE;   
      
      PicIncrementAdres();   // 0x8008

      	  	  	  	  	  	  	  	  	  	// запись второго слова конфигурации ( CONFIGURATION WORD 2 )
      PicLoadDataForProgMem(secondWord);  	// передаем данные в защелку
      //Delay_us(20);
      PicBeginInternTimedProg(T_PINT_CONF); // инициируем запись данных в память

      tempWord = PicReadDataFromProgMem();
      //tempWord &= 0x3703;
      
      if(tempWord != secondWord) return FALSE;
      
      return TRUE; 
    } else return FALSE;
}

int PicCheckConfigSeq (void) { // последовательность проверки битов конфигурации
    uint16_t firstWord  = 0x09AC;
    uint16_t secondWord = 0x1600;
    uint16_t tempWord;

    Reset_PIC(); // VppFirst();

    // задержка 100 милиСек
    //Delay(100);

    					 // эта команда приводит к установке адреса 0х8000
    PicLoadConfig(0x00); // текущий адрес устанавливается 0x8000 ..
    PicIncrementAdres(); // 0x8001
    PicIncrementAdres(); // 0x8002
    PicIncrementAdres(); // 0x8003
    PicIncrementAdres(); // 0x8004
    PicIncrementAdres(); // 0x8005
    PicIncrementAdres(); // 0x8006
    PicIncrementAdres(); // 0x8007

    tempWord = PicReadDataFromProgMem();

    if(tempWord != firstWord)
        return FALSE;

    PicIncrementAdres();   // 0x8008

    tempWord = PicReadDataFromProgMem();

    if(tempWord != secondWord)
        return FALSE;
    
    return TRUE; 
}

#ifdef PIC_USE_SPI
	void PIC_SPI_IRQHandler() {
		_PIC_SPI_MODE_TX;             // нужно быстро переключить в режим передачи, чтоб лишние клоки не проскакивали
		spi->CR2  &= ~SPI_CR2_RXNEIE; // отключаем прерывание
	}
#endif

void PIC_MSP_Init() {
	#ifdef PIC_USE_SPI
		Set_Pin_AF(PIC_CLK_PORT, PIC_CLK_PIN, PIC_SPI_CLK_AF);
		Set_Pin_AF(PIC_DAT_PORT, PIC_DAT_PIN, PIC_SPI_DAT_AF);

		RCC->PIC_SPI_RCC_REG |= PIC_SPI_RCC_BIT;

		spi->CR2  = ((spi->CR2) & ~SPI_CR2_DS) | SPI_CR2_DS_2 | SPI_CR2_DS_0;     // DS[3:0] - 0101 , длинна посылки 6 бит так. как команда при программировании pic равна 6 битам

		spi->CR1  = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |                     // режим master, пин NSS - общего назначения, управление происходит софтварно
  				    SPI_CR1_BR_2 |    SPI_CR1_BR_1 | // SPI_CR1_BR_0 |                                  // входная частота делится на 16
				    SPI_CR1_LSBFIRST |                                             // прием/передача начинается с младшего бита
				    SPI_CR1_CPHA |                                                 // выборка по заднему падающему фронту
				    SPI_CR1_BIDIMODE;                                              // полудуплексный режим работы т.е. прием и передача по одному проводу
		spi->CR2 |= SPI_CR2_FRXTH |
				    SPI_CR2_RXNEIE;                                                 // флаг RXNE поднимается если в приемном буфере 8 бит или больше

		NVIC_SetPriority (PIC_SPI_IRQn, 1);
		NVIC_EnableIRQ(PIC_SPI_IRQn);

		Delay(10);

		//TODO: SPI auto-sends byte
		_PIC_SPI_ENABLE;
		_PIC_SPI_MODE_TX;
	#else
		SET_DOUT (PIC_CLK_DIR);									// CLK
		_SET_GPIO_MODE_OUTPUT(PIC_CLK_PORT, PIC_CLK_PIN);
		_SET_GPIO_OUTPUT     (PIC_CLK_PORT, PIC_CLK_PIN, 0);

		SET_DOUT (PIC_DAT_DIR);									// DAT
		_SET_GPIO_MODE_OUTPUT(PIC_DAT_PORT, PIC_DAT_PIN);
		_SET_GPIO_OUTPUT     (PIC_DAT_PORT, PIC_DAT_PIN, 0);
	#endif
}

void PIC_MSP_DeInit() {
	#ifdef PIC_USE_SPI
	  _PIC_SPI_DISABLE; // makes pins as float
	#endif
  _SET_GPIO_MODE_INPUT(PIC_CLK_PORT, PIC_CLK_PIN);
  _SET_GPIO_MODE_INPUT(PIC_DAT_PORT, PIC_DAT_PIN);
  RESET_DOUT(PIC_CLK_DIR);
  RESET_DOUT(PIC_DAT_DIR);
}

#define _CHECK_TO_STOP  if (*to_stop) { \
                          pic_res = 6; \
                          break; }

#define _DELAY_WITH_CHECK_TO_STOP(D) _CHECK_TO_STOP; \
                                     if (D) Delay(D); \
                                     _CHECK_TO_STOP;

#define _BREAK_IF_RES \
		if (res)\
		{\
			prog_res = res;\
			break;\
		}

void PIC10_IncAddress() {
    PicSendCommand(PIC10_INC_ADDR_CMD);
    Delay_us(T_DLY2);
    picAdres = (picAdres + 1) & PIC10_MAX_ADDR;
}

void PIC10_BulkErase() {
	PicSendCommand(PIC10_BULK_ERASE_CMD); // стираем память программ
	Delay_us(T_ERA);
}

void PIC10_LoadData(uint16_t data) {
    PicSendCommand(PIC10_LOAD_DATA_CMD);
	Delay_us(T_DLY2);
    PicSendData(data);
}

void PIC10_BeginEndProgramming() {
    PicSendCommand(PIC10_BEGIN_PROG_CMD);
	Delay_us(T_PROG);
    PicSendCommand(PIC10_END_PROG_CMD);
}

uint16_t PIC10_ReadData() {
    PicSendCommand(PIC10_READ_DATA_CMD);
	Delay_us(T_DLY2);
    return ((PicReadData() >> 1) & PIC10_DATA_MASK);
}

int PIC10_ProgramVerify_Word(uint16_t w) {
	PIC10_LoadData(w);
	PIC10_BeginEndProgramming();
	uint16_t r = PIC10_ReadData();
	return (r == (w & PIC10_DATA_MASK));
}

int PIC10_WriteVerify_ProgramMemory(uint16_t lines) {
	for (int l = 0; l < lines; l++) {
		HEX_Line_TypeDef * HEX_Line = FW_CACHE_Get_Flash_Line(l); // get HEX-line from HEX-data array

		uint16_t progAdres  = (HEX_Line[0][ADRES+1] + (HEX_Line[0][ADRES] << 8)) / 2; // start address

		if (progAdres == PIC10_CONF_ADDR_IN_HEX)
			continue; 							// ignore line with Configuration Words

		if (progAdres > PIC10_MAX_ADDR)
			return 1; 							// HEX-data addr error

	    while( picAdres != progAdres ) 			// point to dest addr
	    	PIC10_IncAddress();

	    										// read words from HEX-line, program and verify them
	    uint8_t  words_in_line  = (uint8_t) (HEX_Line[0][AMOUNT_OF_BYTES] / 2);
	    uint16_t * p = (uint16_t*) &HEX_Line[0][FIRST_BYTE_PLACE];

	    for (int c = 0; c < words_in_line; c++) {
	    	if (!PIC10_ProgramVerify_Word(p[c]))
	    		return 2; // Verifying error or chip is not connected

	    	PIC10_IncAddress();
	    }
	}

	return 0; // ok
}

int PIC10_WriteVerify_ConfigurationWord(uint16_t lines) {
	for (int l = 0; l < lines; l++) {
		HEX_Line_TypeDef * HEX_Line = FW_CACHE_Get_Flash_Line(l); // get HEX-line from HEX-data array

		uint16_t progAdres  = (HEX_Line[0][ADRES+1] + (HEX_Line[0][ADRES] << 8)) / 2; // start address

		if (progAdres != PIC10_CONF_ADDR_IN_HEX) {
			if (progAdres > PIC10_MAX_ADDR)
				return 1; 										   // HEX-data addr error
			else
				continue; 										   // ignore line with Configuration Words
		}

	    uint16_t * p = (uint16_t*) &HEX_Line[0][FIRST_BYTE_PLACE]; // read Configuration word from HEX-line, program and verify it

    	if (!PIC10_ProgramVerify_Word(*p))
    		return 2; 											   // Verifying error or chip is not connected

    	return 0; 												   // ok
	}

	return 3; 													   // Configuration Words not found in HEX-data
}

int PIC10_DownloadFirmware (uint16_t  fw_lines,
                            int     * to_stop,
                            int       verify_only)
{
	#ifdef PIC_USE_SPI
		  spi = PIC_SPI;
	#endif

    pulse_width = PIC10_PULSE_WIDTH;
	pic_res = 0;

	PIC_MSP_Init();
	Delay(1);

	do {
		Reset_PIC10();

		while (picAdres != PIC10_USER_ID_ADDR) // point to USER ID to erase it. See TABLE 3-2 on p. 7
			PIC10_IncAddress();

		_DELAY_WITH_CHECK_TO_STOP(1);

		PIC10_BulkErase();

		_DELAY_WITH_CHECK_TO_STOP(1);

		int res = PIC10_WriteVerify_ProgramMemory(fw_lines);
		if (res) {
			if      (res == 1) pic_res = 2; // HEX file addr error
			else if (res == 2) pic_res = 3; // Verifying error or chip is not connected
			else Error_Handler();
			break;
		}
		_DELAY_WITH_CHECK_TO_STOP(1);

		Reset_PIC10();

		res = PIC10_WriteVerify_ConfigurationWord(fw_lines);
		if (res) {
			if      (res == 1) pic_res = 2; // HEX file addr error
			else if (res == 2) pic_res = 3; // Verifying error or chip is not connected
			else if (res == 3) pic_res = 4; // Configuration Words not found in HEX-data
			else Error_Handler();
			break;
		}

	} while (0);

	PIC_VCC_Off();
	_SET_MCLR_FLOAT;
	PIC_MSP_DeInit();

	return pic_res;
}

int PIC12_DownloadFirmware (uint16_t           pic_id,
						    uint16_t           fw_lines,
						    uint16_t           data_lines,
						    int * to_stop,
						    int verify_only)
{
	#ifdef PIC_USE_SPI
		  spi = PIC_SPI;
	#endif

  pic_res = 0;
  pulse_width = PIC12_PULSE_WIDTH;

  PIC_MSP_Init();
  
  Delay(1);

  do {
    if (PicCheckIdSeq(pic_id) == FALSE) { // проверяем ID камня прежде чем залить прошивку
        pic_res = 1;
        break;
    }
    _DELAY_WITH_CHECK_TO_STOP(1);

    if (!verify_only) {
      PicRowEraseSeq();
      _DELAY_WITH_CHECK_TO_STOP(1);

      PicBulkEraseSeq();
      _DELAY_WITH_CHECK_TO_STOP(1);

      PicWriteFirmware(fw_lines);
      _DELAY_WITH_CHECK_TO_STOP(1);

      PicWrireEEPROM(data_lines);
      _DELAY_WITH_CHECK_TO_STOP(1);

      PicWrireUserID(data_lines);
      _DELAY_WITH_CHECK_TO_STOP(10);

      if (PicVerifyProgramMemory(fw_lines) != TRUE) {
          pic_res = 2;
          break;
      }
      _DELAY_WITH_CHECK_TO_STOP(1);

      if(PicVerifyEEPROM(data_lines) != TRUE) {
          pic_res = 3;
          break;
      }

      _DELAY_WITH_CHECK_TO_STOP(1);
    }

    if (PicVerifyUserID(data_lines) != TRUE) {
        pic_res = 4;
        break;
    }
    _DELAY_WITH_CHECK_TO_STOP(1);

    if (!verify_only)
      if(PicLoadConfigSeq(data_lines) != TRUE) {
          pic_res = 5;
          break;
      }
  } while (0);

  RESET_DOUT(VIN1_SH_MAX_ON); // Снимаем питание с драйвера
  _SET_MCLR_FLOAT;
  PIC_MSP_DeInit();
  
  return pic_res;
}

#define PIC16_PFM_DATA_MASK      0x3FFF
#define PIC16_PC_ADDR_MASK       0xFFFF
#define PIC16_EEPROM_DATA_MASK   0x00FF
#define PIC16_LOAD_PC_ADDR_CMD   0x80
#define PIC16_LOAD_DATA_CMD      0x00
#define PIC16_LOAD_DATA_INC_CMD  0x02
#define PIC16_READ_DATA_CMD      0xFC
#define PIC16_READ_DATA_INC_CMD  0xFE
#define PIC16_BULK_ERASE_CMD     0x18
#define PIC16_BEGIN_INT_PROG_CMD 0xE0

#define PIC16_BLOCK_SIZE 32
#define PIC16_BLOCK_ADDR_MASK (~(PIC16_BLOCK_SIZE-1))

void PIC16_SendData(uint32_t data, uint16_t delay) {
	PIC_WriteMSB(data << 1, 24);
	Delay_us(delay);
}

void PIC16_SendCommand(uint8_t cmd, uint16_t delay) {
	PIC_WriteMSB(cmd, 8);
	Delay_us(delay);
}

uint32_t PIC16_Read24(uint16_t delay) {
	uint32_t data = PIC_ReadMSB(24) >> 1;
	Delay_us(delay);
	return data;
}

void PIC16_Load_PC_Address(uint32_t addr) {
	PIC16_SendCommand(PIC16_LOAD_PC_ADDR_CMD,     T_DLY);
	PIC16_SendData   (addr & PIC16_PC_ADDR_MASK,  T_DLY);
}

uint32_t PIC16_Read_Data_From_NVM(int inc_addr) {
	PIC16_SendCommand(inc_addr ? PIC16_READ_DATA_INC_CMD : PIC16_READ_DATA_CMD,  T_DLY);
	return (PIC16_Read24(T_DLY) & PIC16_PFM_DATA_MASK);
}

void PIC16_Load_Data_To_NVM(uint16_t data, int inc_addr) {
	PIC16_SendCommand(inc_addr ? PIC16_LOAD_DATA_INC_CMD : PIC16_LOAD_DATA_CMD,  T_DLY);
	PIC16_SendData   (data & PIC16_PFM_DATA_MASK,  T_DLY);
}

void PIC16_Begin_Int_Programming(int conf_words) {
	PIC16_SendCommand(PIC16_BEGIN_INT_PROG_CMD,  conf_words ? PIC16_T_PINT_CONF : PIC16_T_PINT_PFM);
}

void PIC16_Write_DataBlock(uint32_t addr, int count, uint16_t * data, uint16_t data_mask) {
	PIC16_Load_PC_Address(addr);
	for (int i=0; i<count; i++) {
		int to_inc_addr = (i+1 == count) ? 0 : 1;
		PIC16_Load_Data_To_NVM(data[i] & data_mask, to_inc_addr);
	}
	PIC16_Begin_Int_Programming(0);
}

int PIC16_ReadAndCompare_DataBlock(uint32_t addr, int count, uint16_t * data, uint16_t data_mask) {
	PIC16_Load_PC_Address(addr);
	for (int i=0; i<count; i++) {
		uint16_t read_data = PIC16_Read_Data_From_NVM(1);
		if (read_data != (data[i] & data_mask)) return 0; // verification error
	}
	return 1; // ok
}

uint16_t PIC16_Read_Device_ID() {
	PIC16_Load_PC_Address(PIC16_DEVICE_ID_ADDR);
	return PIC16_Read_Data_From_NVM(0);
}

void PIC16_BulkErase() {
	PIC16_Load_PC_Address(PIC16_DEVICE_ID_ADDR); 			// point PC to 0x8000-0x80FD region to erase all memory
	PIC16_SendCommand(PIC16_BULK_ERASE_CMD,  PIC16_T_ERAB);
}

void PIC16_Write_Program_Memory(int lines, int * to_stop) {
	uint16_t block_data[PIC16_BLOCK_SIZE];
	uint32_t block_addr = -1;
	uint32_t block_not_empty = 0;

	HEX_Line_TypeDef * HEX_Line;

	for (int i=0; i<lines; i++) {
		HEX_Line  =  FW_CACHE_Get_Flash_Line(i);

		uint8_t count = (*HEX_Line)[0] / 2;
		uint32_t row_addr = (((*HEX_Line)[1] << 8) + (*HEX_Line)[2]) / 2;

		for (int j=0; j<count; j++) {
			uint32_t addr = row_addr+j;

			int new_block = (addr & PIC16_BLOCK_ADDR_MASK) != (block_addr & PIC16_BLOCK_ADDR_MASK);

			if (new_block) {
				if (block_not_empty)
					PIC16_Write_DataBlock(block_addr, PIC16_BLOCK_SIZE, block_data, PIC16_PFM_DATA_MASK);

				block_addr = addr & PIC16_BLOCK_ADDR_MASK;
				block_not_empty = 0;
				memset(block_data, 0xFF, sizeof(block_data)); // fill with FF
			}

			block_data[addr-block_addr] = (*HEX_Line)[3+j*2] + ((*HEX_Line)[3+j*2+1] << 8);
			block_not_empty = 1;
		}

		if (*to_stop) return;
	}

	if (block_not_empty) // last block
		PIC16_Write_DataBlock(block_addr, PIC16_BLOCK_SIZE, block_data, PIC16_PFM_DATA_MASK);
}


int  PIC16_Verify_Program_Memory(int lines, int * to_stop) {
	uint16_t block_data     [PIC16_BLOCK_SIZE];
	uint32_t block_addr = -1;
	uint32_t block_not_empty = 0;

	HEX_Line_TypeDef * HEX_Line;

	for (int i=0; i<lines; i++) {
		HEX_Line  =  FW_CACHE_Get_Flash_Line(i);

		uint8_t count =  (*HEX_Line)[0] / 2;
		uint32_t row_addr = (((*HEX_Line)[1] << 8) + (*HEX_Line)[2]) / 2;

		for (int j=0; j<count; j++) {
			uint32_t addr = row_addr+j;

			int new_block = (addr & PIC16_BLOCK_ADDR_MASK) != (block_addr & PIC16_BLOCK_ADDR_MASK);

			if (new_block) {
				if (block_not_empty)
					if (!PIC16_ReadAndCompare_DataBlock(block_addr, PIC16_BLOCK_SIZE, block_data, PIC16_PFM_DATA_MASK)) return 5; // verification error

				block_addr = addr & PIC16_BLOCK_ADDR_MASK;
				block_not_empty = 0;
				memset(block_data, 0xFF, sizeof(block_data)); // fill with FF
			}

			block_data[addr-block_addr] = (*HEX_Line)[3+j*2] + ((*HEX_Line)[3+j*2+1] << 8);
			block_not_empty = 1;
		}

		if (*to_stop) return 6; // to stop
	}

	if (block_not_empty) // last block
		if (!PIC16_ReadAndCompare_DataBlock(block_addr, PIC16_BLOCK_SIZE, block_data, PIC16_PFM_DATA_MASK)) return 5; // verification error

	return 0; // ok
}

int PIC16_WriteAndVerify_UserID_ConfigWords(int lines) {
	for (int i=0; i<lines; i++) {
		HEX_Line_TypeDef * HEX_Line  =  FW_CACHE_Get_EEPROM_Line(i);

		uint8_t count =  (*HEX_Line)[0] / 2;
		uint32_t row_addr = (((*HEX_Line)[1] << 8) + (*HEX_Line)[2]) / 2;

		uint16_t addr;

		if (row_addr == PIC16_CONF_WORDS_HEX_ADDR)
			addr = PIC16_CONF_WORDS_ADDR;
		else if (row_addr == PIC16_USED_ID_HEX_ADDR)
			addr = PIC16_USER_ID_ADDR;
		else
			continue;

		PIC16_Load_PC_Address(addr);		// Write, read and verify

		for (int j=0; j<count; j++) {
			uint16_t data = (*HEX_Line)[3+j*2] + ((*HEX_Line)[3+j*2+1] << 8);
			PIC16_Load_Data_To_NVM(data & PIC16_PFM_DATA_MASK, 0);

			PIC16_Begin_Int_Programming(1);

			uint16_t read_data = PIC16_Read_Data_From_NVM(1);

			if ((read_data & PIC16_PFM_DATA_MASK) != (data & PIC16_PFM_DATA_MASK))
				return 5;
		}
	}
	return 0; // ok
}

int PIC16_DownloadFirmware (uint16_t           pic_id,
						    uint16_t           fw_lines,
						    uint16_t           ob_lines,
						    int * to_stop)
{
	#ifdef PIC_USE_SPI
		  spi = PIC_SPI;
	#endif

  pic_res = 0;
  pulse_width = PIC16_PULSE_WIDTH;

  PIC_MSP_Init();
  Delay(1);

  do {
	Reset_PIC16();

	pic_id_ = PIC16_Read_Device_ID();	// проверяем ID камня прежде чем залить прошивку
    if (pic_id_ != pic_id) {
    	if ((pic_id_ == PIC16_PFM_DATA_MASK) || (pic_id_== 0x0000))
            pic_res = 1; // chip is not connected
    	else
    		pic_res = 2; // wrong chip ID

        break;
    }

    PIC16_BulkErase();  // Bulk erase device

    _DELAY_WITH_CHECK_TO_STOP(0);

    PIC16_Write_Program_Memory(fw_lines, to_stop); // Write Program Memory

    _DELAY_WITH_CHECK_TO_STOP(0);

    int res = PIC16_Verify_Program_Memory(fw_lines, to_stop); // Verify Program Memory
    if (res) {
    	if (res == 5)
    		pic_res = res; // Verifying error or chip is not connected
    	else if (res == 6)
    		pic_res = res; // to stop
    	else
    		Error_Handler();

    	break;
    }

    res = PIC16_WriteAndVerify_UserID_ConfigWords(ob_lines); // Write and Verify UserID and Configuration Words
    if (res) {
    	if (res == 5)
    		pic_res = res; 		// Verifying error or chip is not connected
    	else if (res == 6)
    		pic_res = res; 		// to stop
    	else
    		Error_Handler();

    	break;
    }

  } while (0);

  PIC_VCC_Off();
  _SET_MCLR_FLOAT;
  PIC_MSP_DeInit();

  return pic_res;
}
