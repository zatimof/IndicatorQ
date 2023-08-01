//Управляющая программа микроконтроллера ATtiny2313 индикатора добротности v.1.0
//Распиновка индикатора SPI:
//PD2 - D0 - SCK
//PD3 - D1 - SDIN
//PD4 - RES - RESET (LOW - RESET ACTIVE)
//PD5 - DC - DATA/COMMAND (LOW - COMMAND, HIGH - DATA)
//PD6 - CS - CHIP SELECT (LOW - ACTIVE)
//Автор Захаров Т. Р. 2023 г.

#include <iotiny2313.h>
#include <intrinsics.h> 

//functions declare;
void initialize(void);																//initialize function
void send(unsigned char data,unsigned char dc);				//SPI send function

//variables declare
unsigned char screen[2]={11,11};											//массив индикатора количества осцилляций
unsigned char string[16]={12,16,0,0,0,0,0,13,14,15,18,10,17,1,18,0};//массив строки 8
unsigned char i,k,d,h,line,n=0,cnt_val=0;
unsigned long cnt=0,cnt_old=0,freq=0,cnt_aver=0;

__flash const char img[20][5]={
{0x3E,0x41,0x41,0x41,0x3E},			//0										//digit codegen
{0x44,0x42,0x7F,0x40,0x40},			//1
{0x42,0x61,0x51,0x49,0x46},			//2
{0x21,0x41,0x45,0x4B,0x31},			//3
{0x18,0x14,0x12,0x7F,0x10},			//4
{0x27,0x45,0x45,0x45,0x39},			//5
{0x3C,0x4A,0x49,0x49,0x30},			//6
{0x01,0x71,0x09,0x05,0x03},			//7
{0x36,0x49,0x49,0x49,0x36},			//8
{0x06,0x49,0x49,0x29,0x1E},			//9
{0x00,0x00,0x00,0x00,0x00},		//_	10
{0x08,0x08,0x08,0x08,0x08},		//-	11
{0x7F,0x09,0x09,0x01,0x01},		//F	12
{0x7F,0x10,0x28,0x44,0x00},		//k	13
{0x7F,0x08,0x08,0x08,0x7F},		//H	14
{0x44,0x64,0x54,0x4C,0x44},		//z	15
{0x14,0x14,0x14,0x14,0x14},		//=	16
{0x1C,0x20,0x40,0x20,0x1C},		//v	17
{0xC0,0xC0,0x00,0x00,0x00},		//.	18
{0x7E,0x49,0x45,0x7E,0x01}};	//bat 19

__flash const char init1[23]={0xAE,0xD5,0x90,0xA8,
0x3F,0xD3,0x00,0x40,0x8D,0x14,0xA1,0xC8,0xDA,0x12,
0x81,0xB0,0xD9,0x22,0xDB,0x30,0xA4,0xA6,0xAF};				//init string 1
__flash const char init2[8]={0x20,0x00,0x21,0x00,0x7F,0x22,0x00,0x07};//init string 2

//main function
void main(void)
{
  initialize();																				//начальная инициализация
	
  while(1)
  {
    PORTB|=0x08;																			//контроль напряжения батареи
		__delay_cycles(50);
		if(PINB&0x04)
			string[11]=10;
		else
			string[11]=19;
		PORTB&=0xF7;

		n=0;																							//старт счетного интервала
		ACSR=0x0B;
		PORTB|=0x10;
	  __delay_cycles(500000);														//50 мс
		
		if(ACSR&0x20)																			//конец счетного интервала,
			n=0;																						//обработка данных
		ACSR=0x00;
		PORTB&=0xEF;
		
		if(cnt_aver!=0)																		//вычисление частоты
			freq=9999999/cnt_aver;
		else
			freq=0;

		if(freq>999999)																		//ограничение частоты
			freq=999999;
		
		i=0;																							//преобразование в двоично-
    while(freq>=100000)																//десятичный вид и вывод
    {
     	freq-=100000;
     	i++;
    }
    string[2]=i;
		
		i=0;
		while(freq>=10000)
    {
     	freq-=10000;
     	i++;
    }
    string[3]=i;
		
		i=0;
		while(freq>=1000)
    {
     	freq-=1000;
     	i++;
    }
    string[4]=i;

		i=0;
		while(freq>=100)
    {
     	freq-=100;
     	i++;
    }
		string[5]=18;    
		string[6]=i;		
	
    if(n)																							//обработка числа осцилляций
		{
			if(n>99)																				//ограничение числа
				n=99;
			
			i=0;																						//преобразование в двоично-
    	while(n>=10)																		//десятичный вид и вывод
    	{
      	n-=10;
      	i++;
    	}
    	screen[1]=i;
    	screen[0]=n;
		}
		else																							//если n=0 то вывод прочерков
		{																									//в знакоместах количества и частоты 
			screen[1]=11;
    	screen[0]=11;
			string[2]=11;
			string[3]=11;
			string[4]=11;
			string[5]=11;
			string[6]=11;			
		}

		for(line=0;line<7;line++)													//обновление индикатора
		{																									//цикл по строкам
			d=0x01<<line;
		
	  	for(h=1;h!=255;h--)															//цикл по знакоместам
				for(i=0;i<64;i++)															//цикл по столбцам
				{
					k=i>>3;
	
					if((k>1)&&(k<7)&&(img[screen[h]][k-2]&d))		//вывод количества осцилляций
						send(0xFF,1);
					else
						send(0x00,1);
				}
		}
		
		for(i=0;i<128;i++)																//цикл по столбцам в строке 8
		{
			k=i&0x07;
	
			if((k>1)&&(k<7))																//вывод строки сообщения
				send(img[string[i>>3]][k-2]<<1,1);
			else
				send(0x00,1);
		}
  }
}

//initialize function
void initialize(void)
{
	//init ports
	DDRA=0x00;
	PORTA=0x00;
  
  DDRB=0x18;
	PORTB=0x00;
	
	DDRD=0x7C;
	PORTD=0x6C;
	  
	//init timer indicator
	TCCR1A=0x00;
	TCCR1B=0x01;
  TCCR1C=0x00;
	
	//init interrupts
	TIMSK=0x80;
	SREG|=128;
	
	//init comparator
	ACSR=0x0B;
	DIDR=0x03;
	
	//init LCD display
  __delay_cycles(100);
	PORTD=0x7C;
	__delay_cycles(100);

	for(i=0;i<23;i++)																		//загрузка строки инициализации
		send(init1[i],0);
	
	__delay_cycles(1000000);
	
	for(i=0;i<8;i++)
		send(init2[i],0);
	return;
}

#pragma vector=ANA_COMP_vect													//прерывание по фронту
__interrupt void comparator(void)											//компаратора
{
	n++;																								//увеличиваем счетчик осцилляций
	if(cnt_val)																					//контроль валидности cnt
	{
		cnt_old=cnt;
		cnt=TCNT1;
		if(cnt<(cnt_aver>>1))															//контроль измеренного периода
		{																									//если меньше чем 1/2 среднего
			cnt=cnt_old;																		//то это помеха, отбрасываем								
			n--;																				
		}
	}
	else 																								//обнуляем переменные если
	{																										//нет валидности cnt
		cnt=0;
		cnt_old=0;
		cnt_aver=0;
	}
	
	cnt_val=1;
	TCNT1=0;																						//обнуляем счетчик периода

	if(cnt_old&&cnt)
		cnt_aver=(cnt_old+cnt)>>1;												//расчет среднего периода
	
	return;
}

#pragma vector=TIMER1_OVF1_vect												//прерывание по переполнению 
__interrupt void timer1(void)													//таймера 1
{
	cnt_val=0;																					//сбрасываем флаг валидности cnt		
	return;
}

void send(unsigned char data,unsigned char dc)				//SPI send function
{
  if(dc)																							//выставляем сигнал DC и CS
    PORTD=0x78;
  else
    PORTD=0x70;
  
  for(unsigned char l=7;l!=255;l--)										//цикл из 8 бит
  {
    PORTD&=0x1F;																			//формируем спад CLK
		
    if(data&(0x01<<l))																//выставляем данные
      PORTD|=0x20;
		
		PORTD|=0x40;																			//формируем фронт CLK
  }
  
  PORTD=0x7C;																					//убираем сигнал DC и CS
	return;
}
