#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>

char value[10];
uint8_t Rs_1 = 0x01;
uint8_t Rw_1 = 0x02;
uint8_t backright_on = 0x08;
uint8_t En_1 = 0x04;
uint8_t TWDR_buffer = 0x00;
uint8_t mode_buffer = 0x00;

float temp;
float anlog_V;
float Vref = 2.4;
volatile signed int result;
char temp_s[10];
char result_s[10];
int gain=0;
volatile int ADC_flag=0;


//--------------
//타이머 인터럽트 플레그
volatile uint8_t timer1_flag = 0;

void ADC_init(void) // ADC 초기화
{
	ADMUX = 0b11110000; //내부전압 이용, 아날로그 0번핀 사용, 좌측정렬 사용 
	ADCSRA = (1<<ADIE)|(1<<ADEN)|(1<<ADSC)|(1<<ADIE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //일반모드 
}

void ADC_value(void) //좌측 정렬된 ADC 값을 읽는 과정
{
	result = ADCL + (ADCH<<8); 
	result = (result>>6); 
}

void Temp_value(void) //  온도 구해서 temp_s 배열에 저장
{
	gain =1;
	anlog_V = ((result-7.8)*Vref/512/gain);
	temp = (anlog_V )*100;
	dtostrf(temp, 6, 2, temp_s); 
	dtostrf(result,6,2,result_s);
	_delay_us(100);
}

void Timer1(void)
{
	TCCR1B = (1<<WGM12)|(1<<CS12)|(1<<CS10); //분주비 1024
	TIMSK = (1<<OCIE1A);
	OCR1A = 46874; //3초가 되기 위한 top값 , 1024/F_CPU = 1번 카운트하는데 걸리는시간 , 이게 46876번 반복되어야 3초가 된다.
}

void I2C_init(void) // I2C 초기화
{
	TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);
	TWSR = (0<<TWPS0)|(1<<TWPS1); // 분주비를 1로 설정 F_scl 를 100khz를 목표로 계산
	TWBR = F_CPU/100000/2 - 8;
	DDRD = 0b11000000; // SCL,SDA 핀을 사용한다는 뜻
}

void I2C_start(void) // 시작조건 전송
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while(!(TWCR&(1<<TWINT))); 
}

void I2C_slave(uint8_t Slave_addr) // 슬레이브 주소 전송
{
	TWDR = (Slave_addr<<1); 
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
}

void I2C_write(uint8_t data)// 데이터 전송
{
	TWDR_buffer = data;
	TWDR = data;
	TWCR = (1<<TWINT)|(1<<TWEN);
	while(!(TWCR&(1<<TWINT)));
}

void I2C_stop(void) // 정지조건 전송
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void I2C_send(uint8_t data) // I2C 데이터를 전송하는 과정
{
	I2C_start();
	I2C_slave(0x27);
	I2C_write(data);
}

void En_triger(void) // En 트리거 
{
	I2C_write((TWDR_buffer&0xF0)|En_1|mode_buffer); //En을 1로
	I2C_write(((TWDR_buffer&0xF0)&~En_1)|mode_buffer); // En을 0으로
}


void LCD_command(uint8_t data) // LCD에 명령 내리는 함수
{
	uint8_t data_H = data&0xF0;
	uint8_t data_L = (data<<4)&0xF0;
	
	I2C_write(data_H|mode_buffer);
	En_triger();
	I2C_write(data_L|mode_buffer);
	En_triger();
}

void LCD_data(uint8_t data) // LCD 출력할 데이터 입력 
{
	uint8_t data_H = data&0xF0;
	uint8_t data_L = (data<<4)&0xF0;
	
	I2C_write(data_H|mode_buffer);
	En_triger();
	I2C_write(data_L|mode_buffer);
	En_triger();
}

void Clear_disply(void)
{
	LCD_command(0x01);
	_delay_ms(2);
}
void Return_home(void)
{
	LCD_command(0x02);
	_delay_ms(2);
}
void Entry_mode_set(uint8_t data)
{
	LCD_command(data);
	_delay_us(50);
}
void Display_control(uint8_t data)
{
	LCD_command(data);
	_delay_us(50);
}
void Cursor_or_display_shift(uint8_t data)
{
	LCD_command(data);
	_delay_us(50);
}
void Function_set(uint8_t data)
{
	LCD_command(data);
	_delay_us(50);
}
void Set_CG_Ram(uint8_t data)
{
	LCD_command(data);
	_delay_us(50);
}

void mode_set_command(void) // LCD의 모드 설정 
{
	mode_buffer = (backright_on);
}

void mode_set_write(void)
{
	mode_buffer =  (Rs_1|backright_on);
}

void mode_set_read(void)
{
	mode_buffer =  (Rw_1|Rs_1|backright_on);
}

void LCD_printf(char *word) //LCD 문자 출력
{
	while(*word !='\0')
	{
		LCD_data(*word);
		word++;
	}
}

void LCD_init(void) // LCD 초기화
{
	_delay_ms(40);
	I2C_write(0x30); 
	En_triger();
	_delay_ms(5);
	I2C_write(0x30);
	En_triger();
	_delay_us(150);
	I2C_write(0x30);
	En_triger();
	_delay_us(40);
	I2C_write(0x20);
	En_triger();
	_delay_us(40);
	Function_set(0x28); 
	Display_control(0x0C);
	Clear_disply();
	Entry_mode_set(0x06);
	Set_CG_Ram(0x80|0x00);
}

int main(void)
{
	Timer1();
	mode_set_command();
	I2C_init();
	I2C_send(backright_on);
	LCD_init();
	I2C_stop();
	ADC_init();
	_delay_ms(2);
	sei();
	
	while (1)
	{
		
		if(timer1_flag ==1)
		{	
			timer1_flag=0;
			ADCSRA |= (1<<ADSC); //변환시작 하드웨어적인 그래서 메인문은 계속 진행됨 마지막에만 변환 완료되었을때 인터럽트 발생후 값 반환 
			while(ADC_flag !=1); //그래서 변환 완료 전까지 메인문은 진행시키지 않기 위한 무한 반복
			ADC_flag =0;
			Temp_value(); // 온도계산 
			mode_set_write();
			I2C_start();
			I2C_slave(0x27);
			Set_CG_Ram(0x80|0x00);
			LCD_printf("temp:");
			LCD_printf(temp_s);
			LCD_printf(" C");
			mode_set_command();
			Return_home();
			I2C_stop();
		}
	}
}


ISR(ADC_vect)
{
	ADC_value();
	ADC_flag =1;
}

ISR(TIMER1_COMPA_vect)
{
	timer1_flag =1;	
} //프리러닝 모드에서 인터럽트를 계속 요청하고 있고 이때문에 타이머 카운트의 인터럽트가 동작하지 않는다. 