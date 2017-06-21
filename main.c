/*
 * Lab4Project.c
 *
 * Author : Oscar Choy
 *
 * This program uses an ATMega328P board, an external LCD, and an external button to create a
 * stopwatch of sorts that counts from 0 to 255. The "stopwatch" begins in a "Ready" state, in
 * which the LCD will display "0" on it's screen. When the external button is pressed while the
 * "stopwatch" is in the "Ready" state, it will enter the "Counting" state, and begin to count 
 * from 0 up to 255, one second at a time, and the LCD will display this value being counted up. 
 * When the stopwatch reaches 255, it will reset to 0 (and the LCD will display that 0 accordingly),
 * and will count from that 0 to 255 once again. If the button is pressed whist the stopwatch is in
 * the "Counting" state, the stopwatch will enter the "Stopped" state, and the count will stop at
 * whatever value it was at when it was stopped. The LCD will continue to display this value as long
 * as it is in the "Stopped" state. Pressing the external button at any time the stopwatch is in the
 * "Stopped" state will set it back into the "Ready" state, and will both clear the LCD of the previous
 * value it held and will re-display it at the "0" value. The stopwatch can now be used again.
 */ 
#define F_CPU 16000000UL
#define TIMER0_CNT 200
#define TIMER1_CNT 15625
#define NotPushed 1
#define Maybe 2
#define Pushed 3
#define rState 0
#define cState 1
#define sState 2
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

void LcdCommandWrite_UpperNibble(unsigned char cm);
void LcdCommandWrite(unsigned char cm);
void LcdDataWrite(unsigned char cm);
void InitTimer0(void);
void InitTimer1(void);
void LcdCounter(char * value);

unsigned char pushState = NotPushed;
char mode = rState;
char began = 0;
char incremented = 0;
char buffer [5];
int counter = 0;
char setSt = 0;

int main(void)
{
	
	DDRC |= 1<<DDB0;
	DDRC |= 1<<DDB1;
	DDRC |= 1<<DDB2;
	DDRC |= 1<<DDB3;
	DDRC |= 1<<DDB4;
	DDRC |= 1<<DDB5;
	PORTC |= 1<<PORTC0;
	PORTC |= 1<<PORTC1;
	PORTC |= 1<<PORTC2;
	PORTC |= 1<<PORTC3;
	PORTC |= 1<<PORTC4;
	PORTC |= 1<<PORTC5;
	
	DDRD |= 1<<DDD3;
	
	LcdCommandWrite_UpperNibble(0x30);
	_delay_ms(4.1);
	LcdCommandWrite_UpperNibble(0x30);
	_delay_us(100);
	LcdCommandWrite_UpperNibble(0x30);
	LcdCommandWrite_UpperNibble(0x20);
	LcdCommandWrite(0x28);  //4-bit, 2 lines, 5x8 dots
	LcdCommandWrite(0x08);  // display off, cursor off, blink off
	LcdCommandWrite(0x01);  //display clear
	LcdCommandWrite(0x06);  //increment cursor, display shift off
	LcdCommandWrite(0x0c);  //display on, cursor off, blink off
	_delay_ms(120);
	
	InitTimer0();
	InitTimer1();
	sei();
	
	
	LcdDataWrite(0x30);
	LcdCommandWrite(0x80);
	
	while(1) {
		if ((mode == rState) && (began == 1)) {
			began = 0;
			counter = 0;
			incremented = 0;
			LcdCommandWrite(0x01);
			LcdCommandWrite(0x80);
			_delay_ms(30);
			itoa (counter,buffer,10);
			LcdCounter(buffer);
		} else if ((mode == cState) && (began == 0)) {
			began = 1;
		}
		if ((mode == cState) && (incremented == 1)) {
			incremented = 0;
			if (counter == 256) {
				counter = 0;
				LcdCommandWrite(0x01);
				LcdCommandWrite(0x80);
				_delay_ms(30);
			}
			itoa (counter,buffer,10);
			LcdCounter(buffer);
		}
	} 
	return 0;
}

void LcdCommandWrite_UpperNibble(unsigned char cm) {
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC &= ~(1<<4);
	PORTC |= 1<<5;
	_delay_ms(1);
	PORTC &= ~(1<<5);
	_delay_ms(1);	
}

void LcdCommandWrite(unsigned char cm) {
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC &= ~(1<<4);
	PORTC |= 1<<5;
	_delay_ms(1);
	PORTC &= ~(1<<5);
	_delay_ms(1);
	
	PORTC = (PORTC & 0xf0) | (cm & 0x0f);
	PORTC &= ~(1<<4);
	PORTC |= 1<<5;
	_delay_ms(1);
	PORTC &= ~(1<<5);
	_delay_ms(1);
}

void LcdDataWrite(unsigned char cm) {
	PORTC = (PORTC & 0xf0) | (cm >> 4);
	PORTC |= 1<<4;
	PORTC |= 1<<5;
	_delay_ms(1);
	PORTC &= ~(1<<5);
	_delay_ms(1);
	
	PORTC = (PORTC & 0xf0) | (cm & 0x0f);
	PORTC |= 1<<4;
	PORTC |= 1<<5;
	_delay_ms(1);
	PORTC &= ~(1<<5);
	_delay_ms(1);
}

void InitTimer0(void) {
	TCCR0A = 0b00000000;
	TCCR0B = 0b00000101;
	OCR0A = TIMER0_CNT;
	TIMSK0 = _BV(OCIE0A);
	TCNT0 = 0;
}

void InitTimer1(void) {
	TCCR1A = 0b00000000;
	TCCR1B = 0b00000101;
	OCR1A = TIMER1_CNT;
	TIMSK1 = _BV(OCIE1A);
	TCNT1 =	0;
}

ISR(TIMER1_COMPA_vect) {
	if (mode == cState) {
		counter++;
		incremented = 1;
	}
	TCNT1 =	0;
}

ISR(TIMER0_COMPA_vect) {
	switch(pushState) {
		case NotPushed:
			setSt = 0;
			if (PIND & (1<<3)) {
				pushState = Maybe;
			} else { 
				pushState = NotPushed;
			}
			break;
		case Maybe:
			if (PIND & (1<<3)) {
				pushState = Pushed;
			} else {
				pushState = NotPushed;
			}
			break;
		case Pushed:
			//set readystate for all cases
			if (setSt == 0) {
				if (mode == rState) {
					mode = cState;
				} else if (mode == cState) {
					mode = sState;
				} else if (mode == sState) {
					mode = rState;
				}
				setSt = 1;
			}
			if (PIND & (1<<3)) {
				pushState = Pushed;
			} else { 
				pushState = Maybe;
			}
			break;
	}
}


void LcdCounter(char* value) {
	while (*value != 0x00) {
		LcdDataWrite(*value);
		_delay_ms(30);
		value++;
	}
	LcdCommandWrite(0x80);
}