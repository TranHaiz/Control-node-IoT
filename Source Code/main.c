#include <string.h>
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "usart.h"

#define SLEEP (SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk)

#define devId_msb 0x80
#define devId_lsb 0x12

#define devId (devId_msb<<8 | devId_lsb)

#define TIMEOUT 1000000
#define USARTx USART1
//--------------------------------------

int check1=0;
int timeout=0;

uint8_t work=0;
int count=0;

uint8_t quat_state=0;
uint8_t bom_state=0;
uint8_t suong_state=0;
uint8_t den_state=0;


enum{
	MSB_ID,
	LSB_ID,
	QUAT,
	SUONG,
	BOM,
	DEN,
	CRC8,
	RECEIVE_SIZE
}data_receive_type;

uint8_t data_receive[RECEIVE_SIZE];
int i_receive=0;

enum{
	ID_MSB,
	ID_LSB,
	CMD,
	SEND_SIZE
}data_type;
//----------------------------------------------
void parse_data(const char *str) {
    char temp[100];
    strncpy(temp, str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, ";");
    int i = 0;
    while (token && i < RECEIVE_SIZE) {
        data_receive[i++] = atoi(token);
        token = strtok(NULL, ";");
    }
}
void send_data(uint8_t *data, uint8_t length) {
    char buffer[128] = "";
    char temp[8];

    for (int i = 0; i < length; i++) {
        snprintf(temp, sizeof(temp), "%d", data[i]);
        strcat(buffer, temp);
        if (i < length - 1) strcat(buffer, ";");
    }
    strcat(buffer, ".");
		
    USARTx_WRTITE(USARTx,buffer);
}
void turnOff_usart(void){
	USARTx->CR1 &= ~(1<<13);
}
void turnOn_usart(void){
	USARTx->CR1 |= 1<<13;
}
char ack[] = "ok";
//----------------------------------------------
int pwm=0;
int main()
{
	delay_init();
	USARTx_INIT(USART1, USART1_PB6PB7, 9600);
	
	GPIOx_INIT(GPIOA, 2, MODE_OUTPUT_PP,NO_PULL,HIGH_SPEED); //QUAT
	GPIOx_INIT(GPIOA, 3, MODE_OUTPUT_PP,NO_PULL,HIGH_SPEED); //SUONG
	GPIOx_INIT(GPIOB, 1, MODE_OUTPUT_PP,NO_PULL,HIGH_SPEED); //BOM
//	PWMx_SETUP(TIM2, PWM_CH4, TIM2_CH4_PB11, 1000, 10); //DEN
	
	USARTx_WRTITE(USART1,"hello");
	
	
	while(1)
	{
		if(work)
		{
			turnOff_usart();
			GPIOx_WRITE(GPIOA,2,quat_state);
			GPIOx_WRITE(GPIOB,1,bom_state);
			GPIOx_WRITE(GPIOA,3,suong_state);
//			int cuong_do_sang = 100 - den_state;
////			if(cuong_do_sang > 100) cuong_do_sang = 100;
////			else if(cuong_do_sang<0) cuong_do_sang = 0;
////			PWMx_SETUP(TIM2, PWM_CH4, TIM2_CH4_PB11, 1000, cuong_do_sang);
//				PWMx_SETUP(TIM2, PWM_CH4, TIM2_CH4_PB11, 1000, pwm);
			turnOn_usart();
			work=0;
		}
		else
		{
			turnOn_usart();
			delay_s(1);
			SLEEP;
		}
		
	}
}
int check=0;
char buff[14];

void USART1_IRQHandler(void) {
	volatile int i;
	for (i = 0; i < 10; i++) __NOP();

    uint8_t buffer = USARTx->DR;

    if (buffer != '.') {
			buff[i_receive++] = buffer;
        
    } else {
				parse_data(buff);
        
            if (((data_receive[MSB_ID] + data_receive[LSB_ID] + data_receive[QUAT]+data_receive[BOM]+data_receive[SUONG]
							+data_receive[DEN] ) & 0xFF) == data_receive[CRC8]) {
                if ((data_receive[MSB_ID] == devId_msb) && (data_receive[LSB_ID] == devId_lsb)) {
                    quat_state = data_receive[QUAT];
										bom_state = data_receive[BOM];
										suong_state = data_receive[SUONG];
										den_state = data_receive[DEN];

										work = 1;
//										USARTx_WRTITE(USARTx,"1");
                    //new--cmd
                }
            }
        
        i_receive = 0; // 
    }
}