#define F_CPU 8000000
#include <avr/io.h>
#include <avr/interrupt.h>

uint16_t ubrr=9600;

int main(void)
{	DDRB|=(0b111<<1);
	
	ubrr=F_CPU/16/ubrr-1;
	/*Set baud rate */
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;
	/*Enable receiver and transmitter */
	UCSRB = (1<<RXEN)|(1<<TXEN)|(1<<RXCIE);
	/* Set frame format: 8data, 1stop bit */
	UCSRC = (1<<URSEL)|(1<<USBS)|(3<<UCSZ0);
	sei();
	
	while (1)
	{
	}
}

ISR(USART_RXC_vect){//A,B,C,D ???
	
	char rchar=UDR;
	
	PORTB=(PORTB&(0b11110001))|((rchar-64)<<1);
	
}


