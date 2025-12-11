#include <avr/io.h>
#include <stdio.h>
#define F_CPU 16000000

//UART1 통신 설정
void UART_init1(int baud)
{
	uint16_t UBRR = F_CPU/16/baud -1;
	UBRR1H = UBRR>>8;
	UBRR1L = UBRR;
	UCSR1B |= (1<<RXEN1)|(1<<TXEN1); // UART 송수식 동작 허용
	UCSR1C |= (0<<UMSEL1)|(1<<UCSZ11)|(1<<UCSZ10);
}

//PWM 통신 설정
void PWM_init() // timer/count 2 이용
{
	TCCR2 |= (1<<WGM21)|(1<<WGM20); // fast PWM mode
	TCCR2 |= (1<<COM21)|(0<<COM20); // 비반전 모드
	TCCR2 |= (1<<CS21)|(1<<CS20); // 분주비 64
	TCNT2 = 0;
	OCR2 = 0;
}

int main(void)
{
	UART_init1(9600);
	PWM_init();
	int n=0;
	while (1)
	{
		if((UCSR1A & (1<<RXC1)))
		{
			n = UDR1;
		}
		if(n==0)
		{
			TCCR2 &= ~((1<<COM21));
			DDRB = 0x00;
			OCR2 = 0;
		}
		else if(n==1)
		{
			TCCR2 |= (1<<COM21)|(0<<COM20);
			DDRB|=(1<<PB7);
			OCR2 = 76; //30%
		}
		else if(n==2)
		{
			TCCR2 |= (1<<COM21)|(0<<COM20);
			DDRB|=(1<<PB7);
			OCR2 = 153; //60%
		}
		else if(n==3)
		{
			TCCR2 |= (1<<COM21)|(0<<COM20);
			DDRB|=(1<<PB7);
			OCR2 = 255;//100%
		}
	}
}

