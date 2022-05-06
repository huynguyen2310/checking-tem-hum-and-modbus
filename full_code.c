

/* Includes */
#include <stddef.h>
#include "stm32l1xx.h"
#include "nucleo152start.h"
#include <stdio.h>

/* Private typedef */
/* Private define  */
/* Private macro */
/* Private variables */
/* Private function prototypes */
/* Private functions */
void USART2_Init(void);
void USART2_write(char data);
void delay_Ms(int delay);
void read_dht22_humidity_and_temperature(int *hum, int *temp);
void delay_us(unsigned long delay);
void delay_Us(int delay);

/**
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/
int main(void)
{
  /* Configure the system clock to 32 MHz and update SystemCoreClock */
  SetSysClock();
  SystemCoreClockUpdate();
  USART2_Init();
  /* TODO - Add your application code here */
  RCC->AHBENR|=1; //GPIOA ABH bus clock ON. p154
  GPIOA->MODER|=0x400; //GPIOA pin 5 to output. p184
  /* Infinite loop */
  int hum=0;
  int temp=0;
  char buf[20]="";
  char buf2[20]="";

  while (1)
  {
		  //GPIOA->ODR|=0x20; //0010 0000 set bit 5. p186
	  	  GPIOA->ODR|=0x20; //0010 0000 set bit 5. p186
		  delay_Ms(1500);
		  GPIOA->ODR&=~0x20; //0000 0000 clear bit 5. p186
		  delay_Ms(1500);
		  read_dht22_humidity_and_temperature(&hum,&temp);

		  sprintf(buf,"temperature: %d \n\r",temp);
		  for(int i=0;i<20;i++)
			  USART2_write(buf[i]);

		  sprintf(buf2,"humidity: %d \n\r",hum);
		  for(int i=0;i<20;i++)
			  USART2_write(buf2[i]);
  }
  return 0;
}

void USART2_Init(void)
{
	RCC->APB1ENR|=0x00020000; 	//set bit 17 (USART2 EN)
	RCC->AHBENR|=0x00000001; 	//enable GPIOA port clock bit 0 (GPIOA EN)
	GPIOA->AFR[0]=0x00000700;	//GPIOx_AFRL p.188,AF7 p.177
	GPIOA->AFR[0]|=0x00007000;	//GPIOx_AFRL p.188,AF7 p.177
	GPIOA->MODER|=0x00000020; 	//MODER2=PA2(TX) to mode 10=alternate function mode. p184
	GPIOA->MODER|=0x00000080; 	//MODER2=PA3(RX) to mode 10=alternate function mode. p184

	USART2->BRR = 0x00000D05;	//9600 BAUD and crystal 32MHz. p710, D05
	USART2->CR1 = 0x00000008;	//TE bit. p739-740. Enable transmit
	USART2->CR1 |= 0x00000004;	//RE bit. p739-740. Enable receiver
	USART2->CR1 |= 0x00002000;	//UE bit. p739-740. Uart enable
}

void USART2_write(char data)
{
	//wait while TX buffer is empty
	while(!(USART2->SR&0x0080)){} 	//TXE: Transmit data register empty. p736-737
		USART2->DR=(data);		//p739
}

void delay_Ms(int delay)
{
	int i=0;
	for(; delay>0;delay--)
		for(i=0;i<2460;i++); //measured with oscilloscope
}

void delay_Us(int delay)
{
	for(int i=0;i<(delay*2);i++) //accurate range 10us-100us
	{
		asm("mov r0,r0");
	}
}

void delay_us(unsigned long delay)
{
	unsigned long i=0;
	RCC->APB2ENR|=(1<<4); 	//TIM11EN: Timer 11 clock enable. p160
	TIM11->PSC=1; 		//32 000 000 MHz / 32 = 1 000 000 Hz. p435
	TIM11->ARR=1;	 		//TIM11 counter. 1 000 000 Hz / 1000 = 1000 Hz ~ 1ms. p435
	TIM11->CNT=0;			//counter start value = 0
	TIM11->CR1=1; 			//TIM11 Counter enabled. p421

	  while(i<delay)
	  {
		  while(!((TIM11->SR)&1)){} //Update interrupt flag. p427
		  i++;
		  TIM11->SR &= ~1; 	//flag cleared. p427
		  TIM11->CNT=0;	  	//counter start value = 0
	  }
	  TIM11->CR1=0; 		//TIM11 Counter disabled. p421
}

void read_dht22_humidity_and_temperature(int *hum, int *temp)
{
	unsigned int humidity=0, i=0,temperature=0;
	unsigned int mask=0x80000000;
	RCC->AHBENR|=1; //GPIOA ABH bus clock ON. p154
	GPIOA->MODER|=0x1000; //GPIOA pin 6 to output. p184
	GPIOA->ODR|=0x40; //0100 0000 set bit 6. p186
	delay_Ms(10);
	GPIOA->ODR&=~0x40; //low-state at least 500 us
	delay_Ms(1);
	GPIOA->ODR|=0x40; //pin 6 high state and sensor gives this 20us-40us
	GPIOA->MODER&=~0x3000; //GPIOA pin 6 to input. p184

	//response from sensor
	while((GPIOA->IDR & 0x40)){}
	while(!(GPIOA->IDR & 0x40)){}
	while((GPIOA->IDR & 0x40)){}

	//read values from sensor
	while(i<32)
	{
		while(!(GPIOA->IDR & 0x40)){}

		delay_Us(35);

		if((GPIOA->IDR & 0x40)&&i<16)
		{
			humidity=humidity|(mask>>16);
		}
		if((GPIOA->IDR & 0x40)&&i>=16)
		{
			temperature=temperature|mask;
		}
		mask=(mask>>1);
		i++;

		while((GPIOA->IDR & 0x40)){}
	}
	*hum=(int)humidity;
	*temp=(int)temperature;
}