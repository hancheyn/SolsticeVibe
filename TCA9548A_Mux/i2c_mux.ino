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


/* I2C Mux: TCA9548A */
void i2c_mux_write(unsigned char address, unsigned char mux) {
    i2c_start();
    i2c_write(address << 1 | 0);
    
    i2c_write(mux);
    i2c_stop();
}

unsigned char i2c_mux_read(unsigned char address) {
    i2c_start();
    i2c_write(address << 1 | 1);
    
    unsigned char mux = i2c_read(0);
    i2c_stop();
    
    return mux;
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
  usart_sendByte('\n');
  i2c_mux_write(0x70, 0x00);
  i2c_mux_write(0x70, 0x01);
}

/* ARDUINO MAIN LOOP */
void loop() {
    unsigned int state;
    
    //I2C Loop
    int i = 0;

    for(i = 0; i < 8; i++) {
      
      //! Write New Mux Channel
      i2c_mux_write(0x70, 0x01 << i);
      _delay_ms(100); 

      //! Read the Current Channel
      state = (unsigned int)i2c_mux_read(0x70);
      sprintf(str, "Mux State: %d\n", state);     
      //Send str to UART   
      usart_sendString(str);  
    
      _delay_ms(2000);
    }
    _delay_ms(1000);
}
