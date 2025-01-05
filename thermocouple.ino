
#include <avr/io.h>
#define F_CPU 16000000UL

#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defines and Globals: Easy Access */
#define AHT10_ADDRESS 0x38

/* I2C Stop Bit */
/*
    Description: I2C Stop Bit
    This is dependent on avr/io.h.
*/
void i2c_stop() {
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

/*
    Description: I2C Writes 1 Byte to I2C Bus
    This is dependent on avr/io.h.

    @param data i2c data to write
*/
void i2c_write(unsigned char data) {
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

/*
    Description: I2C Reads 1 Byte to I2C Bus ack value 1 = more data | 0 = end of burst
    This is dependent on avr/io.h.

    @param ackVal 1 = more data | 0 = end of burst
    @return TWDR i2c read 8 bit char
*/
unsigned char i2c_read(unsigned char ackVal) {
    TWCR = (1 << TWINT) | (1 << TWEN) | (ackVal << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}

/*
    Description: I2C Start Bit
    This is dependent on avr/io.h.
*/
void i2c_start() {
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

/*
    Description: Essential I2C Init
    This is dependent on avr/io.h.
*/
void i2c_init() {
    TWSR = 0x00;
    TWBR = 152;
    TWCR = 0x04;
}

/* I2C Helper Functions */

/*
    Description: I2C Write Function

    @param address i2c address to read
    @param memory i2c memory location to read
    @param data to write to i2c module
*/
void i2c_write_byte(unsigned char address, unsigned char memory, unsigned char data) {
    i2c_start();
    i2c_write(address << 1);
    i2c_write(memory);
    i2c_write(data);
    i2c_stop();
}

/*
    Description:  I2C read for a 8 bit value / byte.

    @param address i2c address to read
    @param memory i2c memory location to read
    @return data 8 bit char
*/
unsigned char i2c_read_byte(unsigned char address, unsigned char memory) {
    i2c_start();
    i2c_write(address << 1 | 1);
    i2c_write(memory);
    unsigned char data = i2c_read(0);
    i2c_stop();

    return data;
}

/*
    Description: I2C read for a 16 bit word.

    @param address i2c address to read
    @param memory i2c memory location to read
    @return data read for 16 bits as unsigned integer
*/
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
/*
    Description: Initialize AHT10 IC
    @param address for i2c device
*/
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

/*
    Description: This runs the process to populate new measurements in the AHT10 module.

    @param address for i2c device
*/
void i2c_measure_aht10(unsigned char address) {

    i2c_start();
    i2c_write((address << 1));
    i2c_write(0xAC);  // 0b10101100
    i2c_write(0x33);  // 0b00110011
    i2c_write(0x00);  // 0b00000000
    i2c_stop();

    _delay_ms(80);
}

/*
    Description: This is a read function for the AHT10 to collect the humidity and temp values.
    @param address for i2c device
*/
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

    /* Non-intuitive way of storing infomration on aht10 device */
    *humidity = (((uint32_t)bytes[1]) << 12) + (((uint32_t)bytes[2]) << 4) + (((uint32_t)bytes[3]) >> 4);
    *temp = ((((uint32_t)bytes[3]) & 0x0F) << 16) + (((uint32_t)bytes[4]) << 8) + (((uint32_t)bytes[5]) >> 0);
}


/* UART INIT */
/*
    Description: Initialization for UART on ATMEGA
    This is dependent on avr/io.h.
*/
void usart_init() {
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
    UBRR0L = 103; // baud rate = 9600
}

/* UART Helper Functions */
/*
    Description: USART Write of a single byte
    This is dependent on avr/io.h.

    @param ch character to send
*/
void usart_sendByte(unsigned char ch) {
    while(!(UCSR0A&(1 << UDRE0))); //Wait until Buff is empty
    UDR0 = ch;
}

/*
    Description: Packed binary-coded decimal send
    @param data[] character array to send as BCD type
*/
void usart_sendPackedBCD(unsigned char data) {
    usart_sendByte('0' + (data >> 4));
    usart_sendByte('0' + (data & 0x0F));
}

/*
    Description:  String send / write over UART
    @param ch[] character array to send
*/
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
  _delay_ms(100);
  i2c_init();
  _delay_ms(100);
  i2c_init_aht10(AHT10_ADDRESS);
}

/* ARDUINO MAIN LOOP */
void loop() {

    /* Local Variables */
    unsigned int state;
    uint32_t hum = 0; 
    uint32_t temp = 0;
    unsigned char str[100]; /* buffer for writing strings */
    
    /* Read temp and hum measurement from AHT10 module through pass by reference */
    i2c_read_aht10(AHT10_ADDRESS, &hum, &temp);

    /* Temperature Conversion */
    uint32_t convert = (temp/1048576.0) * 200.0;
    temp = (convert) - 50 ;

    /* Humidity Conversion */
    uint32_t convert_hum = (hum/1048576.0) * 100.0;

    sprintf(str, "AHT10 Device: Humidity %ld Percent | Temp %ld C\0", convert_hum, temp);
    
    /* Send str to UART */ 
    usart_sendString(str);  

    /* Measure the thermocouple ADCs */
    /* Thermocouple 1 from A0 input */
    double sensorValue = analogRead(A0);
    double mV = (sensorValue/204.4)*1000;
    double diff_temp = mV / 4.141;
    uint32_t thermo_temp = (uint32_t)diff_temp + temp; /* Add reference temp to thermocouple reading */
    /* Send str to UART */   
    sprintf(str, "%ld degrees C\0", thermo_temp);
    usart_sendString("Thermocouple 1: ");
    usart_sendString(str);

    /* Thermocouple 2 from A1 input */
    double sensorValue2 = analogRead(A1);
    double mV2 = (sensorValue2/204.4)*1000;
    double diff_temp2 = mV2 / 4.141;
    uint32_t thermo_temp2 = (uint32_t)diff_temp2 + temp; /* Add reference temp to thermocouple reading */
    /* Send str to UART */  
    sprintf(str, "%ld degrees C\n\0", thermo_temp2);
    usart_sendString("Thermocouple 2: ");
    usart_sendString(str);

    _delay_ms(2000);
    /* Run process for new temp / hum measurement */
    i2c_measure_aht10(AHT10_ADDRESS);
    _delay_ms(100);
}
