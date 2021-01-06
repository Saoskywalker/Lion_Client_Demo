#ifndef __COM_H_
#define __COM_H_

#include "sys.h" 

extern u8 frameFlag; //has one frame

#define MTF_BUFF_LEN 32
extern u8 frameData[MTF_BUFF_LEN];
extern int rec_total;

void MTF_Com_readDataTimer(void);
void MTF_Com_recData(u8 *data, int *num);
void MTF_Com_sendHead(void);
void MTF_Com_sendTail(void);
void MTF_Com_sendData(u8 data);
u8 MTF_Com_init(u8 check);

#endif
