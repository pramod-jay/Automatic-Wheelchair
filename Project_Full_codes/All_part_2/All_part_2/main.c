#define F_CPU 8000000
#include <avr/io.h>
#include <stdio.h>
#include "LCDI2C.h"
#include "USART.h"

char lcddata[40];

uint8_t hx711H=0; //Load Scale High Bits
uint16_t hx711L=0;//Load Scale Low Bits
char lengthcal[40];
float loadCellRead();
void sendSMS(char*msg);
uint16_t ReadADC(uint8_t ADCchannel);
uint16_t Reading=0; //analog Reading variable
uint8_t sms_patentfall=0;
uint8_t sms_location=0;
uint8_t sms_batterylow=0;
uint8_t sms_emergency=0;
uint8_t sms_imhere=0;

int main(void)
{
	USART_Init(9600);
	LcdInit(0x27);
	DDRB|=(1<<0); //Load cell clock pin
	PORTB&=~(1<<0);//Clock pin low
	USART_TxStringln("AT");
	DDRA|=(1<<3);
	
	ADCSRA |= ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0));   // prescaler 128
	ADMUX |= (1<<REFS0);//internal 2.56 v ref
	ADCSRA |= (1<<ADEN);// Turn on ADC
	ADCSRA |= (1<<ADSC);// Do an initial conversion
	
	longitude=10.58;
	laditute=20.12;
	//USART_getLocation();
	
	_delay_ms(2000);
	LcdCommand(LCD_CLEARDISPLAY);
	//sendSMS("test sms");
	
	while (1)
	{	float load=loadCellRead();
		
		sprintf(lcddata,"%.3f kg ",load);
		LcdSetCursor(0,0,lcddata);
		_delay_ms(100);
		
		if ((load<20)&&(!sms_patentfall))
		{
			sprintf(lcddata,"patient has fallen \r\n %.3f %.3f",longitude,laditute);
			sendSMS(lcddata);
			sms_patentfall=1;
		}
		Reading=ReadADC(5);
		if ((Reading<512)&&(!sms_batterylow)){
			sprintf(lcddata,"battery low \r\n %.3f %.3f",longitude,laditute);
			sendSMS(lcddata);
			sms_batterylow=1;
		}
		
		if ((PINC&(1<<2))&&(!sms_emergency)){
			sprintf(lcddata,"emergency \r\n %.3f %.3f",longitude,laditute);
			sendSMS(lcddata);
			sms_emergency=1;
		}
		
		if ((PINC&(1<<3))&&(!sms_imhere)){
			sprintf(lcddata,"I am here \r\n %.3f %.3f",longitude,laditute);
			sendSMS(lcddata);
			sms_imhere=1;
		}
		
		_delay_ms(500);
		
	}
}

float loadCellRead(){
	hx711H=0;hx711L=0;  //clear variables
	for(uint8_t i=0;i<8;i++){  // Load cell data high 8 bits
		PORTB|=(1<<0); //Clock pin high
		_delay_us(10);
		if ((PINB&(1<<7))>>1){  //read data pin
			hx711H|=(1<<(7-i));//set hx 711 varible
		}
		else{
			hx711H&=~(1<<(7-i));
		}
		PORTB&=~(1<<0); //Clock pin low
		_delay_us(5);
	}
	
	
	for(uint8_t i=0;i<16;i++){ // Load cell data low 16 bits
		PORTB|=(1<<0); //Clock pin high
		_delay_us(10);
		if ((PINB&(1<<7))>>1){ //read data pin
			hx711L|=(1<<(15-i));
		}
		else{
			hx711L&=~(1<<(15-i));
		}
		PORTB&=~(1<<0); //Clock pin low
		_delay_us(5);
	}
	
	hx711L=hx711L>>1; //shift bits
	
	if (hx711H&1){  //bit setup
		hx711L|=(1<<15);
	}
	else{
		hx711L&=~(1<<15);
	}
	hx711H=hx711H>>1;
	
	return (hx711H*(65536/18029.6))+hx711L/18029.6; //load cell calibration
}

void sendSMS(char*msg){
	
	
	PORTA|=(1<<3);
	_delay_ms(100);
	PORTA&=~(1<<3);
	
	LcdCommand(LCD_CLEARDISPLAY);
	LcdSetCursor(0,0,"Sending sms");
	USART_TxStringln("AT");
	_delay_ms(500);
	USART_TxStringln("AT");
	_delay_ms(500);
	USART_TxStringln("AT+CMGF=1");
	_delay_ms(500);
	sprintf(lengthcal,"AT+CMGS=\"+94%s\"","776651535");
	USART_TxStringln(lengthcal);
	_delay_ms(200);
	USART_TxString(msg);
	_delay_ms(200);
	USART_Transmit(26);
	_delay_ms(200);
	USART_ClearRX();
	_delay_ms(30000);
	USART_ClearRX();
	LcdCommand(LCD_CLEARDISPLAY);
}




uint16_t ReadADC(uint8_t ADCchannel)
{
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADCW;
}