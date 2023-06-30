#define F_CPU 8000000
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "LCDI2C.h"

volatile uint16_t adcread[2];
volatile uint8_t adcreadchannel=0;
uint8_t steperMode=4;

volatile uint16_t TimerCal=0;// variable for collect echo data
uint16_t ultraINTL=0;
uint16_t ultraINTR=0;

#define stepDDR DDRC
#define stepPORT PORTC
#define DIRL 4
#define DIRR 5
#define STEPL 6
#define STEPR 7

uint8_t modeSelect=0;
uint8_t prevmodeSelect=0;
uint8_t emergencyst=0;

#define modeSlectpin PINB
#define modeSlectPORT PORTB
#define JOY 4
#define VOICE 6
#define NONE 5

char lcddata[20];

void selectscan();
void stepperM(uint8_t side,uint16_t cycle);


int main(void)
{
	
	TCCR1B|=(1<<WGM12);//Enable Compare match mode
	TCCR1B|=(1<<CS11)|(1<<CS10);//start timer prescaler =64
	TCNT1=0;	
	OCR1A=2000;
	stepDDR|=(1<<DIRL)|(1<<DIRR)|(1<<STEPL)|(1<<STEPR);
	TIMSK|=(1<<OCIE1A);
	/*register value= time*(clock speed/prescale)
	register value=0.05*(8000000/64)
	register value=2000*/
	//TIMSK|=(1<<OCIE1A);//enable timer Compare inturrept
	DDRA|=(1<<3);
	PORTD|=(1<<3);
	
	
	/*Ultrasonic Timer Part*/
	DDRA|=(1<<0); //D4 as output
	TCCR0|=(1<<WGM01);//Enable Compare match mode
	TCCR0|=(1<<CS11);//Start timer  prescaler =8
	TCNT0=0;
	OCR0=10;
	/*register value= time*(clock speed/prescale)
	register value=0.000001*(8000000/1)
	register value=10*/
	TIMSK|=(1<<OCIE0);//enable timer Compare inturrept
	
	modeSlectPORT|=(1<<JOY)|(1<<VOICE)|(1<<NONE);
	
	ADMUX|=(1<<REFS0);//AVCC
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADSC)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADATE);
	adcreadchannel=0;
	ADMUX=(ADMUX&0b11100000)|(6+adcreadchannel);
	//ADCSRA|=(1<<ADSC);
	
	
	
	sei();
	
    LcdInit(0x27);
	LcdSetCursor(5,0,"Welcome");
	_delay_ms(500);
	
	
    while (1) 
    {
		selectscan();
		
		if (!(PIND&(1<<3)))
		{
			emergencyst=1;
		}
		
		if (modeSelect==2)
		{
			uint8_t data=((PINB&0b1110)>>1);
			
			if ((0<data)&&(data<5)&&!emergencyst)
			{
				switch(data){
					case 1:steperMode=3; break;
					case 2:steperMode=2; break;
					case 3:steperMode=1; break;
					case 4:steperMode=4; break;
					
				}
				
			TIMSK|=(1<<OCIE1A);
			} 
			else
			{
				TIMSK&=~(1<<OCIE1A);
			}
					
			
			if (data==1)
			{PORTA&=~(1<<0);//TRIG pin low
				_delay_us(50);//wait 50 micro sec
				PORTA|=(1<<0);//TRIG pin high
				_delay_us(50);//wait 50 micro sec
				PORTA&=~(1<<0);////TRIG pin low
				while(!(PINA&(1<<1)))//wait for pulse
					TimerCal=0;//rest timer
				while((PINA&(1<<1)))////wait for pulse down
					ultraINTL=TimerCal;//copy timer value
				ultraINTL=ultraINTL/4.04;
				_delay_ms(50);
				
				PORTA&=~(1<<0);//TRIG pin low
				_delay_us(50);//wait 50 micro sec
				PORTA|=(1<<0);//TRIG pin high
				_delay_us(50);//wait 50 micro sec
				PORTA&=~(1<<0);////TRIG pin low
				while(!(PINA&(1<<2)))//wait for pulse
					TimerCal=0;//rest timer
				while((PINA&(1<<2)))////wait for pulse down
					ultraINTR=TimerCal;//copy timer value
				ultraINTR=ultraINTR/4.04;
				sprintf(lcddata,"%ucm %ucm ",ultraINTL,ultraINTR);
				LcdSetCursor(0,1,lcddata);
				
				
				if ((ultraINTL<50) && (ultraINTR>49))
				{
					PORTA|=(1<<3);
					_delay_ms(100);
					PORTA&=~(1<<3);

					stepperM(1,50);//right
					stepperM(3,50);//forward
					stepperM(4,50);//left
					stepperM(3,50);//forward
					stepperM(4,50);//left
					stepperM(3,50);//forward
					stepperM(1,50);//right
				}
				
				if ((ultraINTR<50) && (ultraINTL>49))
				{
					
					PORTA|=(1<<3);
					_delay_ms(100);
					PORTA&=~(1<<3);
					
					stepperM(4,50);//left
					stepperM(3,50);//forward
					stepperM(1,50);//right
					stepperM(3,50);//forward
					stepperM(1,50);//right
					stepperM(3,50);//forward
					stepperM(4,50);//left
				}
				
				if ((ultraINTL<50) && (ultraINTR<50)){
					PORTA|=(1<<3);
					_delay_ms(100);
					PORTA&=~(1<<3);
					
					emergencyst=1;//stop stepper motor rotation
				}
			}
		}//end of mode select	
    }
}

ISR(TIMER1_COMPA_vect){//stepper motor run
	TCNT1=0;
	stepPORT^=(1<<STEPL);//toggle pulse signal
	stepPORT^=(1<<STEPR);//toggle pulse signal

	if (steperMode==1)
	{
		stepPORT&=~((1<<DIRR)|(1<<DIRL));//right
	}
	else if(steperMode==2)
	{
		stepPORT|=(1<<DIRL);stepPORT&=~(1<<DIRR);//reverse
	}
	else if(steperMode==3){
		stepPORT|=(1<<DIRR);stepPORT&=~(1<<DIRL);//forward
	}
	
	else if(steperMode==4){
		stepPORT|=(1<<DIRR)|(1<<DIRL);//left
	}


}

void selectscan(){
	if (!(modeSlectpin&(1<<JOY)))
	{
		modeSelect=0;
	
	}else if(!(modeSlectpin&(1<<NONE))){
		modeSelect=1;
	
	}else if(!(modeSlectpin&(1<<VOICE)))
	{
		modeSelect=2;
	
	}

	if (modeSelect!=prevmodeSelect){
		LcdCommand(LCD_CLEARDISPLAY);
	
		switch(modeSelect){
			case 0:LcdSetCursor(0,0,"Joystick Mode"); break;
			case 1:LcdSetCursor(0,0,"Manual Mode");TIMSK&=~(1<<OCIE1A); break;
			case 2:LcdSetCursor(0,0,"Voice cont. Mode"); break;
		}
		prevmodeSelect=modeSelect;
	}	
}

ISR(ADC_vect){
	
	ADMUX=(ADMUX&0b11100000)|(6+adcreadchannel);
	adcread[adcreadchannel]=ADCW;
	adcreadchannel=!adcreadchannel;
	
	if (modeSelect==0)
	{
		if (adcread[0]>812)
		{	steperMode=3;
			TIMSK|=(1<<OCIE1A);
		}
		else if(adcread[0]<212){
			steperMode=2;
			TIMSK|=(1<<OCIE1A);
		}
		else if(adcread[1]>812){
			steperMode=1;
			TIMSK|=(1<<OCIE1A);
		}
		else if(adcread[1]<212){
			steperMode=4;
			TIMSK|=(1<<OCIE1A);
		}
		else{
		TIMSK&=~(1<<OCIE1A);
		}
	}
	
}


void stepperM(uint8_t side,uint16_t cycle){
if (side==1)
{
	stepPORT&=~((1<<DIRR)|(1<<DIRL));//right   1343431
}
else if(side==2)
{
	stepPORT|=(1<<DIRL);stepPORT&=~(1<<DIRR);//reverse
}
else if(side==3){
	stepPORT|=(1<<DIRR);stepPORT&=~(1<<DIRL);//forward
}

else if(side==4){
	stepPORT|=(1<<DIRR)|(1<<DIRL);//left
}
for(uint16_t i=0;i<cycle;i++){
	stepPORT|=(1<<STEPL)|(1<<STEPR);
	_delay_us(1000);
	stepPORT&=~((1<<STEPL)|(1<<STEPR));
	_delay_us(1000);	
}
}


ISR(TIMER0_COMP_vect){//ultrasonic
	TimerCal++;
	TCNT0=0;
}

