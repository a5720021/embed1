/*
 * ultrasonic.c
 *
 * Created: 8/11/2560 16:16:09
 * Author : Pathompat
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#define F_CPU 16000000UL				// Clock Speed
#define BAUD 9600						// Baud Rate
#define MYUBRR (( F_CPU/16/BAUD)-1 )
volatile uint8_t Flag;					//Capture Flag
volatile uint16_t Capt1;				//Variable holding timestamp
volatile uint16_t time_echo;			//Value to send time capture

void USART_Transmit(unsigned char data){		//Send Data via USART0
	/* Wait for empty transmit buffer */ 
	while ( !(UCSR0A & (1<< UDRE0)) );
	/* Put data into buffer, sends the */ 
	UDR0 = data ; 
}

void USART_Init(unsigned int ubrr){				//Setup UART
		/*Set baud rate */ /*Set baud rate */
		UBRR0H = (unsigned char )( ubrr>> 8);
		UBRR0L = (unsigned char )ubrr;
		UCSR0A &= 0xFD;
		//Enable receiver and transmitter */ 
		UCSR0B = (1<< RXEN0)|( 1<< TXEN0);
		/* Set frame format: */
		UCSR0C = (3<<UCSZ00); 
}

void serial_print(volatile uint16_t text){		//Print number via UART
	char buff[4];
	int dis = text*(1024/16)/58;			//convert signal to distance
	itoa(dis, buff, 10);					//convert integer to ascii (char array)
	for(int i=0;i<sizeof(buff)-1;i++){		//loop print value
		USART_Transmit(buff[i]);
	}
	USART_Transmit('\n');					//new line when finished
}

void timer0_init(void)							//Set Timer0 for Trigger
{
	TCCR0A = 0x00;		// set up timer register TCCR0A
	TCCR0B = 0x02;		// set up timer register TCCR0B prescaler 8
	TCNT0 = 0;
}

void timer1_init(void)							//Set Timer1 for input capture
{
	TCNT1 = 0;			//Set Initial Timer value
	TCCR1B = 0x45;		// capture on rising edge, prescalar 1024
	TIMSK1 = 0x21;		//Enable input capture and overflow interrupts
	sei();				//Enable global interrupts
}

void send_trigger(void){		//Send Trigger signal 10us and wait 500ms for next
	if(PORTB != 0x02){
		_delay_ms(500);
	}
	PORTB ^= 0x02;				// toggle port B
}

int main(void)
{
	timer0_init();
	timer1_init();
	USART_Init (MYUBRR);
	DDRB = 0x02;				//set pin9 for output trigger
    while (1) 
    {
		_delay_ms(2000);
		if (TCNT0 >= 20)		//send trigger signal 10us
		{
			send_trigger();
			TCNT0 = 0;			// reset counter
		}

		if(Flag==2)				//check state captured
		{	
			time_echo = Capt1;
			serial_print(time_echo);
			Capt1 = 0;			//clear timer value
			Flag=0;				//clear status flag
			TIMSK1|=(1<<ICIE1)|(1<<TOIE1);//enable input capture and overflow interrupts
		}
    }
}

ISR(TIMER1_CAPT_vect) //capture ISR
{
	if (Flag==0) //detect first rising edge
	{
		TCNT1=0; //Set Initial Timer value
		TCCR1B &= ~(1<<ICES1);//change capture on falling edge
	}
	if (Flag==1)
	{
		Capt1 = ICR1; //capture timer
		TCCR1B |= (1<<ICES1);//change capture on rising edge
		TIMSK1 &= ~((1<<ICIE1)|(1<<TOIE1));//stop input capture and overflow interrupts
	}
	Flag++; //increment Flag
}