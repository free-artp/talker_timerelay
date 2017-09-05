/*
 * STKbd.c
 *
 * Created: 04.09.2017 15:50:54
 *  Author: Artp
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#include "uart.h"
#include "timer.h"

//#define CONSOLE_DEBUG 1
#undef CONSOLE_DEBUG
#include "debug.h"

#define STROB_TIMEOUT (clock_time_t)(CLOCK_SECOND * 17 / 10)
#define STROB_PAUSE (clock_time_t)(CLOCK_SECOND * 5 / 10)
//#define STROB_TIMEOUT (clock_time_t)(CLOCK_SECOND * 5)

//=============== Pin assigment
/*
#define KEY1 PORTD4		// D4
#define KEY2 PORTD5		// D5
#define KEY3 PORTD6		// D6
#define KEY4 PORTD7		// D7

#define RS1 PORTB0		// Rotation sensor #1 (8) (2) - PD2 - INT0
#define RS2 PORTB1		// Rotation sensor #2 (9) (3) - PD3 - INT1

#define STROB1			// PORTC0 (A0)
#define STROB2			// PORTC1 (A1)
#define STROB3			// PORTC2 (A2)
#define STROB4			// PORTC3 (A3)

// KEY1, KEY2, RS1 -> STROB1,STROB2
*/

// first channel
#define key1_pressed bit_is_set(PIND,PORTD4)
#define key2_pressed bit_is_set(PIND,PORTD5)

#define set_strob1_run (PORTC |= _BV(PORTC0))
#define set_strob1_stop (PORTC &= ~_BV(PORTC0))
#define strob1_runing (PINC & _BV(PORTC0))

#define set_strob2_run (PORTC |= _BV(PORTC1))
#define set_strob2_stop (PORTC &= ~_BV(PORTC1))
#define strob2_runing (PINC & _BV(PORTC1))

#define set_strob12_stop (PORTC &= ~( _BV(PORTC1) | _BV(PORTC0)))
#define strob12_runing (PINC & (_BV(PORTC1) | _BV(PORTC0) ) )

struct timer timer_c1;	// длина строба
struct timer timer_t1;	// пауза между стробами

// second channel
#define key3_pressed bit_is_set(PIND,PORTD6)
#define key4_pressed bit_is_set(PIND,PORTD7)

#define set_strob3_run (PORTC |= _BV(PORTC2))
#define set_strob3_stop (PORTC &= ~_BV(PORTC2))
#define strob3_runing (PINC & _BV(PORTC1))

#define set_strob4_run (PORTC |= _BV(PORTC3))
#define set_strob4_stop (PORTC &= ~_BV(PORTC3))
#define strob4_runing (PINC & _BV(PORTC0))

#define set_strob34_stop (PORTC &= ~( _BV(PORTC3) | _BV(PORTC2)))
#define strob34_runing (PINC & (_BV(PORTC3) | _BV(PORTC2) ) )

struct timer timer_c2;
struct timer timer_t2;	// пауза между стробами


ISR(INT0_vect){
/*
	if ( bit_is_clear(PINB,PORTB5)  ) {
		PORTB |= _BV(PORTB5);
		} else {
		PORTB &= ~_BV(PORTB5);
	}
*/
	set_strob12_stop;
	timer_set( &timer_t1, STROB_PAUSE);
}

ISR(INT1_vect){
/*
	if ( bit_is_clear(PINB,PORTB5)  ) {
		PORTB |= _BV(PORTB5);
		} else {
		PORTB &= ~_BV(PORTB5);
	}
*/
	set_strob34_stop;
	timer_set( &timer_t2, STROB_PAUSE);
}

int main(void)
{
	
	// input - keys (4-7) and Rotation sensors (2-3)
	DDRD = (0<<PORTD2) | (0<<PORTD3) | (0<<PORTD4) | (0<<PORTD5) | (0<<PORTD6) | (0<<PORTD7);
	PORTD = (0<<PORTD2) | (0<<PORTD3) | (0<<PORTD4) | (0<<PORTD5) | (0<<PORTD6) | (0<<PORTD7);
	
	EICRA |= _BV(ISC11) | _BV(ISC01);	// задний фронт
	EIMSK = _BV(INT1) | _BV(INT0);
	
	// output - 0-3 strobs
	DDRC = (1<<PORTC0) | (1<<PORTC1) | (1<<PORTC2) | (1<<PORTC3);
	PORTC = 0;
		
	// LED - PORTB5 and rotation sensors: RS1 - PORTB0, RS2 - PORTB1
	DDRB |= (1<<PORTB5) | (0<<PORTB0) | (0<<PORTB1);
	PORTB = 0;
//	PCICR |=  _BV(PCIE0);
//	PCMSK0 |= ( _BV(PCINT0) | _BV(PCINT1) );
		
	//PORTB &= ~(1<<PORTB5);			// запишем 0

//	uart_init();

	
	clock_init();
	timer_set( &timer_t1, STROB_PAUSE);	
	timer_set( &timer_t2, STROB_PAUSE);
	
	sei();

    while(1)
    {
		// первый канал
		if( strob12_runing ){								// значит находимся в процессе генерации строба
			if( timer_expired( &timer_c1 ) ) {				// может слишком долго генерируем?
				set_strob12_stop;
				timer_set( &timer_t1, STROB_PAUSE);
			}
		} else {											// строба нет, значит можно опрашивать клавиши
			if (timer_expired(&timer_t1)) {
				if (key1_pressed ) {
					timer_set( &timer_c1, STROB_TIMEOUT);
					set_strob1_run;
				} else if (key2_pressed) {
					timer_set( &timer_c1, STROB_TIMEOUT);
					set_strob2_run;
				}
			}
		} // закончили с первым каналом
		// второй канал
		if( strob34_runing ){								// значит находимся в процессе генерации строба
			if( timer_expired( &timer_c2 ) ) {				// может слишком долго генерируем?
				set_strob34_stop;
				timer_set( &timer_t2, STROB_PAUSE);
			}
		} else {											// строба нет, значит можно опрашивать клавиши
			if (timer_expired(&timer_t2)) {
				if (key3_pressed ) {
					timer_set( &timer_c2, STROB_TIMEOUT);
					set_strob3_run;
					} else if (key4_pressed) {
					timer_set( &timer_c2, STROB_TIMEOUT);
					set_strob4_run;
				}
			}
		} // закончили с вторым каналом

    }
}
