/**********************************************
 * name: T5L protocol v1.2
 * email:support@mtf123.club
 * Copyright(c) MTF 2016~2030. www.mtf123.club
 * All Rights Reserved
 * ******************************************/

// #include "stdio.h"
#include "MTF_ComProtocol.h"
#include "usart.h"
#include "delay.h"

static u8 sum_open = 1; //open frame verify function
static u8 Head[] = {0XAA}; //head define
static u8 End[] = {0XCC, 0X33, 0XC3, 0X3C}; //end define
u8 frameData[MTF_BUFF_LEN];
int rec_total = 0;

/******************port**************/
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
    sum_open = check;
    return 0;
}

/*rec uart data and judge frame(need use in uart rec int)
data: frame data(not include head and end byte)
num: data length*/
u8 frameFlag = 0; //has one frame
static u8 frameStart = 0;
/*read data over time error(long time without 1 data), need use in timer irq*/
static u8 overCnt = 0;
void MTF_Com_readDataTimer(void)
{
    if(frameStart)
    {
        if(overCnt<10)
            overCnt++;
    }
}
void MTF_Com_recData(u8 *data, int *num)
{
    static u8 step = 0;
    static int i = 0;
    static u16 k = 0, sum = 0;
    u8 rec = 0;
    
    rec = com_uart_getc();
    if(frameFlag) //a frame without handle
        return;
    if (overCnt>10) //frame rec over time error
    {
        overCnt = 0; //initialize
        i = 0;
        step = 0;
        *num = 0;
        frameStart = 0;
    }
    if(1) //uart has data
    {
        overCnt = 0;
        switch (step)
        {
        case 0:
            if (rec == 0xAA) //check head
            {
                *num = 0;
                frameStart = 1;
                step++;
                i = 0;
            }
            break;
        case 1:
            *num = *num+1; //data
            data[*num-1] = rec;
            break;
        default:
            step = 0;
            break;
        }

        if(frameStart) //check end
        {
            if(i==1)
            {
                if (rec == End[1])
                    i++;
                else
                    i = 0;
            }
            else if(i==2)
            {
                if (rec == End[2])
                    i++;
                else
                    i = 0;
            }
            else if(i==3)
            {
                if (rec == End[3])
                    i++;
                else
                    i = 0;
            }
            if (i == 0 && rec == End[0])
            {
                i++;
            }
        }
        if(i==4)
        {
            if (sum_open) //enable sum verify
            {
                for (k = 0, sum = 0; k < *num - 6; k++)
                    sum += data[k];
                if (sum == (((u16)data[*num - 6] << 8) + data[*num - 5]))
                {
                    i = 0;
                    step = 0;
                    *num -= 6;
                    frameStart = 0;
                    frameFlag = 1; //has one frame
                }
                else
                {
                    i = 0;
                    step = 0;
                    *num -= 6;
                    frameStart = 0; //error
                }
            }
            else
            {
                i = 0;
                step = 0;
                *num -= 4;
                frameStart = 0;
                frameFlag = 1; //has one frame
            }
        }
    }
}

/*send one frame to display
data: user data (not include head&end byte)
num: data length*/
static u16 _sum = 0;
void MTF_Com_sendHead(void)
{
    u8 i = 0;
    _sum = 0;
    for (i = 0; i<sizeof(Head); i++) //head
        com_uart_putc(Head[i]);
}
void MTF_Com_sendTail(void)
{
    u8 i = 0;
    if (sum_open) //enable sum verify
    {
        com_uart_putc((u8)(_sum >> 8));
        com_uart_putc((u8)_sum);
    }
    //end
    for (i = 0; i < sizeof(End); i++)
        com_uart_putc(End[i]);
}
void MTF_Com_sendData(u8 data)
{
    //BODY
    com_uart_putc(data);
    _sum += data;
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
