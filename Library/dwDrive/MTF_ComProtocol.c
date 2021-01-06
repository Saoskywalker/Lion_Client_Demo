/**********************************************
 * name: MTF protocol v1.3
 * email:support@mtf123.club
 * Copyright(c) MTF 2016~2030. www.mtf123.club
 * All Rights Reserved
 * ******************************************/

#include "MTF_ComProtocol.h"
#include "usart.h"
#include "mbcrc.h"
#include "delay.h"

/*协议为半双模式, 一定情况下可全双工, MODBUS自定义功能码*/
/*地址1 选项1 后面的字节长度1 指令 数据 CRC16(地址1+选项1+后面的字节长度1+指令+数据,低位在前)*/

#define RDorDE_PIN PBout(12)

static u8 _addr = 0XA1, _option = 0X66, _check_sum = 1;
u8 frameData[MTF_BUFF_LEN];
int rec_total = 0;
u8 frameFlag = 0; //has one frame
static u8 frameStart = 0;

/******************port**************/
static __inline void _send_mode(void)
{
    while(frameStart); //半双工时,避免发收冲突
    RDorDE_PIN = 1;
}
static __inline void _read_mode(void) 
{
    RDorDE_PIN = 0;
}
static __inline void com_uart_putc(u8 c)
{
    uasrt2SendByte(c);
}
static __inline u8 com_uart_getc(void)
{
    return (u8)USART_ReceiveData(USART2);
}

u8 MTF_Com_init(u8 check)
{
    RDorDE_PIN = 0; //read mode
    _check_sum = check; //open frame verify function
    return 0;
}

/*rec uart data and judge frame(need use in uart rec int)
data: frame data(not include head and end byte)
num: data length*/
/*read data over time error(long time without 1 data), need use in timer irq*/
static u8 overCnt = 0;
void MTF_Com_readDataTimer(void)
{
    if(frameStart)
    {
        if(overCnt<10)
        {
            frameStart = 0;
            overCnt++;
        }
    }
}
void MTF_Com_recData(u8 *data, int *num)
{
    static u8 step = 0;
    static u8 frameLength = 0;
    u16 crc16 = 0;
    u8 head[3];
    u8 rec = 0;

    rec = com_uart_getc();
    if(frameFlag) //a frame without handle
        return;
    if (overCnt>10) //frame rec over time error
    {
        overCnt = 0; //initialize
        step = 0;
        *num = 0;
        frameStart = 0;
    }

    if(1) //uart has data
    {
        overCnt = 0;
        if (*num >= MTF_BUFF_LEN)
            return; //帧过大
        switch (step)
        {
        case 0:
            if (rec == _addr) //检头
            {
                *num = 0;
                frameStart = 1;
                step++;
            }
            break;
        case 1:
            if (rec == _option)
            {
                step++;
            }
            else
            {
                frameStart = 0;
                step = 0;
            }
            break;
        case 2:
            if (rec!=0)
            {
                frameLength = rec;
                step++;
            }
            else
            {
                frameStart = 0;
                step = 0;
            }
            break;
        case 3:
            *num = *num + 1;
            data[*num - 1] = rec;
            break;
        default:
            step = 0;
            break;
        }

        if (step == 3)
        {
            if (*num >= frameLength) //检数量
            {
                if (_check_sum) //开启校验
                {
                    head[0] = _addr;
                    head[1] = _option;
                    head[2] = frameLength;
                    crc16 = usMBCRC16_multi(&head[0], sizeof(head), data, frameLength - 2);
                    if (crc16 == (((u16)data[*num - 1] << 8) + data[*num - 2]))
                    {
                        *num -= 2;
                        step = 0;
                        frameStart = 0;
                        frameFlag = 1; //有一帧
                    }
                    else
                    {
                        step = 0;
                        frameStart = 0;
                        frameFlag = 0; //帧错误
                    }
                }
                else
                {
                    step = 0;
                    frameStart = 0;
                    frameFlag = 1; //有一帧
                }
            }
        }
    }
}

#define MTF_REC_BUFF 32
static u8 _buffer[MTF_REC_BUFF], _cntBuffer = 0;
void MTF_Com_sendHead(void)
{
    _cntBuffer = 0;
    _buffer[_cntBuffer] = _addr;
    _cntBuffer++;
    _buffer[_cntBuffer] = _option;
    _cntBuffer = 3; //2元素用于放数量
}
void MTF_Com_sendTail(void)
{
    u8 i = 0;
    u16 crc16 = 0;

    _send_mode();
    if (_check_sum) //开启校验
    {
        _buffer[2] = _cntBuffer-1;
        crc16 = usMBCRC16(&_buffer[0], _cntBuffer);
        _buffer[_cntBuffer] = (u8)crc16;
        _cntBuffer++;
        _buffer[_cntBuffer] = (u8)(crc16>>8);
        _cntBuffer++;
    }
    else
    {
        _buffer[2] = _cntBuffer-3;
    }
    for (i = 0; i < _cntBuffer; i++)
        com_uart_putc(_buffer[i]);
    _read_mode();
    delay_ms(4);
}
void MTF_Com_sendData(u8 data)
{
    //BODY
    if (_cntBuffer <= MTF_REC_BUFF - 2)
    {
        _buffer[_cntBuffer] = data;
        _cntBuffer++;
    }
}

/**************demo**************/
/* 
void TIM3_IRQHandler(void)   //timer int 100us
{	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)
	{
        MTF_Com_readDataTimer();
    }
}
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		MTF_Com_recData(&frameData[0], &rec_total);
	}
}

void main(void) //demo
{
    u8 data = 0, i = 1; 
    uart2_init(57600);
    TIM3_Int_Init(99, 70); //100us
    MTF_Com_sendHead();
    MTF_Com_sendData(data); //Inquire command
    MTF_Com_sendTail();
    while (1)
    { 
        if(frameFlag)
        {
            frameFlag = 0;
            if(frameData[0]==0) //Inquire command
                printf("function = %#X, length = %d\r\n", 
                        frameData[2], rec_total);
        }
    }
}
 */
