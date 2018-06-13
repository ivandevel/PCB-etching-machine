/*
 * etching.c
 *
 * Created: 6/6/2018 5:14:04 PM
 * Author : User
 */ 

#define F_CPU 9600000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define MOTOR PB0 // OC0A
#define BUZZER PB1 // OC0B

#define LED1 PB2 // OC0A
#define LED2 PB3 // OC0B

#define BUTTON1 PB4 // PCINT4

#define SPEED_STOP 255
#define SPEED_SLOW 240
#define SPEED_MED 120
#define SPEED_HIGH 80
#define SPEED_SUPER 0
#define ETCHING_PERIOD_S (15*60*2)

uint8_t speed = 0;
uint32_t counter = 0;
uint8_t app_state = 0;

void do_beep(uint8_t n_times)
{
	TCCR0B = (0 << CS02)|(1 << CS01)|(0 << CS00);
	while (n_times--) {
		OCR0B=128;
		_delay_ms(30);
		OCR0B=255;
		_delay_ms(70);
		//PORTB ^= (1<<LED1);
	}
	OCR0B=255;
	
	TCCR0B = (0 << CS02)|(0 << CS01)|(1 << CS00);
}
	 
ISR(PCINT0_vect)
{
	_delay_ms (50); // антидребезг (использовать задержки в прерываниях некошерно, но пока и так сойдёт)
	if ( (PINB & (1<<BUTTON1)) == 0 ) // если нажата одна из кнопок
	{
		do_beep(1);
		
		if (app_state == 2) {
		switch (speed)
		{
			case 0:
			OCR0A = SPEED_MED;
			speed = 1;
			break;
			case 1:
			OCR0A = SPEED_HIGH;
			speed = 2;
			break;
			case 2:
			OCR0A = SPEED_SUPER;
			speed = 3;
			break;
			case 3:
			OCR0A = SPEED_STOP;	
			app_state = 3;					
			break;			
			
		}
		}
		 
		while ( (PINB & (1<<BUTTON1)) == 0 ) {} // ждём отпускания кнопки
	}
}


int main(void)
{
	 // Пины кнопок
	 DDRB &= ~(1<<BUTTON1); // входы
	 PORTB |= (1<<BUTTON1); // подтянуты
	
	 // Светидиоды:
	 DDRB |= (1 << MOTOR)|(1 << BUZZER)|(1 << LED1)|(1 << LED2); // выходы = 1
	 PORTB &= ~((1 << MOTOR)|(1 << BUZZER)|(1 << LED1)|(1 << LED2)); // по умолчанию отключены = 0
	 // Таймер для ШИМ:
	 //TCCR0A = 0xB3; // режим ШИМ, неинверсный сигнал на выходе OC0A, инверсный - на выходе OC0B
	 
	 TCCR0A |= (1<<WGM01) | (1<<WGM00) | (1<<COM0A1) | (1<<COM0A0) | (1<<COM0B1) | (1<<COM0B0); //Fast PWM with toggle on OC0A (Pin 5)
	 
	 //TCCR0B = 0x02; // предделитель тактовой частоты CLK/8
	 TCCR0B = (0 << CS02)|(0 << CS01)|(1 << CS00); // предделитель clk/1024 (101)
	 
	 TCNT0=0; // начальное значение счётчика
	 OCR0A=255; // регистр совпадения A
	 OCR0B=255; // регистр совпадения B
	
	GIMSK |= (1<<PCIE); // Разрешаем внешние прерывания PCINT0.
	PCMSK |= (1<<BUTTON1); // Разрешаем по маске прерывания на ногак кнопок (PCINT3, PCINT4)
	sei(); // Разрешаем прерывания глобально: SREG |= (1<<SREG_I)
	
	// Use the Power Down sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	

    while(1)
    {
		
		switch (app_state)
		{
		case 0:
		do_beep(5);
		counter = 0;
		app_state = 1;
		break;
			
		case 1:
		OCR0A = SPEED_SLOW;
		app_state = 2;
		break;
			
		case 2:
		PORTB ^= (1<<LED1); //переключаем состояние светодиода (вкл./выкл.)
		_delay_ms(500);
		if (++counter > ETCHING_PERIOD_S) app_state = 3;	
		break;	
			
		case 3:	
		OCR0A = SPEED_STOP;
		do_beep(5);
		app_state = 4;
		break;
		
		case 4:
		app_state = 0;
		speed = 0;
		counter = 0;
		PORTB &= ~(1<<LED1);
		sleep_mode();
		break;
			
		}
		
    }
}

