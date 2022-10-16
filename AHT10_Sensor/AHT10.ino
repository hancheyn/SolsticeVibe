
#include <avr/io.h>
#define F_CPU 16000000UL

#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global Variables: Easy Access */
unsigned char address = 0x38; 
unsigned long int hum; 
unsigned long int temp; 
unsigned char str[100];
unsigned char buffer[10];

/* I2C Stop Bit*/
void i2c_stop() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

/* I2C Writes 1 Byte to I2C Bus */
void i2c_write(unsigned char data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

/* I2C Reads 1 Byte to I2C Bus ack value 1 = more data | 0 = end of burst*/
unsigned char i2c_read(unsigned char ackVal) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (ackVal << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

/* I2C Start Bit */
void i2c_start() {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

/* Essential I2C Init */
void i2c_init() {
    TWSR = 0x00;
    TWBR = 152;
    TWCR = 0x04;
}

/* I2C Helper Functions */
void i2c_write_byte(unsigned char address, unsigned char memory, unsigned char data) {
    i2c_start();
    i2c_write(address << 1);
    i2c_write(memory);
    i2c_write(data);
    i2c_stop();
}

unsigned char i2c_read_byte(unsigned char address, unsigned char memory) {
    i2c_start();
    i2c_write(address << 1 | 1);
    i2c_write(memory);
    unsigned char data = i2c_read(0);
    i2c_stop();

    return data;
}

unsigned int i2c_read_word(unsigned char address, unsigned char memory) {
    i2c_start();
    i2c_write(address << 1 | 1);
    i2c_write(memory);
    unsigned int data = i2c_read(1) << 8;
    data |= i2c_read(0);
    i2c_stop();

    return data;
}


/* AHT10 IC */
void i2c_init_aht10(unsigned char address) {
    i2c_init();
    i2c_start();
    i2c_write((address << 1));
    i2c_write(0xBE);  //0b10101100
    i2c_write(0x08);  //0b00110011
    i2c_write(0x00);  //0b00000000
    i2c_stop();
    _delay_ms(80);

    i2c_start();
    i2c_write((address << 1));
    i2c_write(0xAC);  //0b10101100
    i2c_write(0x33);  //0b00110011
    i2c_write(0x00);  //0b00000000
    i2c_stop();
    _delay_ms(80);
}

void i2c_measure_aht10(unsigned char address) {

    i2c_start();
    i2c_write((address << 1));
    i2c_write(0xAC);  //0b10101100
    i2c_write(0x33);  //0b00110011
    i2c_write(0x00);  //0b00000000
    i2c_stop();

    _delay_ms(80);
}

void i2c_read_aht10(unsigned char address, uint32_t* humidity, uint32_t* temp) {
   
    unsigned char bytes[6];

    i2c_start();
    i2c_write(address << 1 | 1);
   
    bytes[0] = i2c_read(1);
    bytes[1] = i2c_read(1);
    bytes[2] = i2c_read(1);
    bytes[3] = i2c_read(1);
    bytes[4] = i2c_read(1);
    bytes[5] = i2c_read(1);
    i2c_read(0);
    i2c_stop();

    *humidity = (((uint32_t)bytes[1]) << 12) + (((uint32_t)bytes[2]) << 4) + (((uint32_t)bytes[3]) >> 4);
    *temp = ((((uint32_t)bytes[3]) & 0x0F) << 16) + (((uint32_t)bytes[4]) << 8) + (((uint32_t)bytes[5]) >> 0);
}


/* UART INIT */
void usart_init() {
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UBRR0L = 103; // baud rate = 9600
}

/* UART Helper Functions */
void usart_sendByte(unsigned char ch) {
    while(!(UCSR0A&(1 << UDRE0))); //Wait until Buff is empty
    UDR0 = ch;
}

void usart_sendPackedBCD(unsigned char data) {
    usart_sendByte('0' + (data >> 4));
    usart_sendByte('0' + (data & 0x0F));
}

void usart_sendString(unsigned char ch[]) {
    unsigned int len = strlen(ch);
    int i = 0;
    for(i = 0; i < len; i++) {
        usart_sendByte(ch[i]);
    }

    usart_sendByte('\n');
}


/* ARDUINO CORE SETUP */
void setup() {
  usart_init();
  usart_sendByte('1');
  _delay_ms(100);
  i2c_init();
  _delay_ms(100);
  i2c_init_aht10(address);
}

/* ARDUINO MAIN LOOP */
void loop() {
    usart_sendByte('2');
    usart_sendByte('\n');
    unsigned int state;
    uint32_t hum = 0; 
    uint32_t temp = 0; 

    //I2C Loop
    int i = 0;
    
    i2c_read_aht10(address, &hum, &temp);
    sprintf(str, "%ld\0", hum);
    
    //Send str to UART   
    usart_sendString(str);  
  
    _delay_ms(1000);
     i2c_measure_aht10(address);
    _delay_ms(1000);
}
