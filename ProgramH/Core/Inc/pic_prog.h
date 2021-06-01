#ifndef PIC_PROG_H
#define PIC_PROG_H

#include "inc.h"
#include "string.h"

#define HEX_DATA_BYTES_IN_LINE 							 16
#define HEX_LINE_SIZE 			(3 + HEX_DATA_BYTES_IN_LINE)

typedef unsigned char HEX_Line_TypeDef[HEX_LINE_SIZE];

//#include "global.h"

//#define _SET_MCLR_0V    do {RESET_DOUT(MCLR_9V_ON); SET_DOUT(MCLR_0V_ON);} while (0)
//#define _SET_MCLR_9V    do {RESET_DOUT(MCLR_0V_ON); SET_DOUT(MCLR_9V_ON);} while (0)
//#define _SET_MCLR_FLOAT do {RESET_DOUT(MCLR_0V_ON); RESET_DOUT(MCLR_9V_ON);} while (0)

//int PIC12_DownloadFirmware (uint16_t           pic_id,
//						  	uint16_t           fw_lines,
//							uint16_t           data_lines,
//							int * to_stop,
//							int verify_only);
//
//int PIC10_DownloadFirmware (uint16_t  fw_lines,
//                            int     * to_stop,
//                            int       verify_only);

int PIC16_DownloadFirmware (uint16_t           pic_id,
						    uint16_t           fw_lines,
						    uint16_t           ob_lines,
						    int * to_stop);

void PIC_SPI_IRQHandler();

void PIC_MSP_DeInit();

#endif
