/*
DHT22 & AM2302 Library
 Author : Leonid (DLM)
*/

#ifndef F_CPU
#define F_CPU 8000000UL //set fcpu for delay.h if not already set in pref
#endif

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>

#include "DHT22_AM2302_v3.h"

//Default Constructor - uses defaults set in header for port and pin
DHT22::DHT22()
{
	temperature = humidity = 0;		//initialize temperature and humidity values to 0
}

//This constructor accepts a pin value and uses the defaults in the header for the port
DHT22::DHT22(int dataPin)
{
	dht22_inputPin = dataPin;		// Assign data input pin for specified port
	temperature = humidity = 0;		//initialize temperature and humidity values to 0
}

//Initialize the object with (&DDRX, &PORTX, &PINX, X) accepts port and pin definitions
DHT22::DHT22(volatile uint8_t *ddr, volatile uint8_t *port, volatile uint8_t *pin, int dataPin)
{
	dht22_ddr = ddr;				// Copy pointer of DDRX address to class private pointer for later use
	dht22_pin = pin;				// Pointer to Pointer copy, address pointed to by pointer1(ddr) is copied to pointer2(dht22_ddr)
	dht22_port = port;				// which is why no * required
	dht22_inputPin = dataPin;		// Assign data input pin for specified port
	temperature = humidity = 0;		//initialize temperature and humidity values to 0
}

// get data from sensor
int8_t DHT22::readData()
{
	//*** uint8_t bits[5];
	uint8_t bits[5] = {};
	uint8_t i,j = 0;

	//reset port
	*dht22_ddr |= (1<<dht22_inputPin); //output
	*dht22_port |= (1<<dht22_inputPin); //high
	_delay_ms(100);

	//send request
	*dht22_port &= ~(1<<dht22_inputPin); //low
	_delay_us(500);
	*dht22_port |= (1<<dht22_inputPin); //high
	*dht22_ddr &= ~(1<<dht22_inputPin); //input
	_delay_us(40);

	//check start condition 1
	if((*dht22_pin & (1<<dht22_inputPin))) {
		return -1;
	}
	_delay_us(80);
	
	//check start condition 2
	if(!(*dht22_pin & (1<<dht22_inputPin))) {
		return -1;
	}
	_delay_us(80);

	//read the data
	uint16_t timeoutcounter = 0;
	for (j=0; j<5; j++) { //read 5 byte
		uint8_t result=0;
		for(i=0; i<8; i++) {//read every bit
			timeoutcounter = 0;
			while(!(*dht22_pin & (1<<dht22_inputPin))) { //wait for an high input (non blocking)
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -1; //timeout
				}
			}
			_delay_us(30);
			if(*dht22_pin & (1<<dht22_inputPin)) //if input is high after 30 us, get result
			result |= (1<<(7-i));
			timeoutcounter = 0;
			while(*dht22_pin & (1<<dht22_inputPin)) { //wait until input get low (non blocking)
				timeoutcounter++;
				if(timeoutcounter > DHT_TIMEOUT) {
					return -1; //timeout
				}
			}
		}
		bits[j] = result;
	}

	//reset port
	*dht22_ddr |= (1<<dht22_inputPin); //output
	*dht22_port |= (1<<dht22_inputPin); //low
	_delay_ms(100);

	//check checksum
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4])
	{
		//return temperature and humidity
		uint16_t rawhumidity = bits[0]<<8 | bits[1];
		uint16_t rawtemperature = bits[2]<<8 | bits[3];
		if(rawtemperature & 0x8000)
		{
			temperature = (float)((rawtemperature & 0x7FFF) / 10.0) * -1.0;
		}
		else
		{
			temperature = (float)(rawtemperature)/10.0;
		}
		humidity = (float)(rawhumidity)/10.0;
		return 0;
	}

	return -1;
}

//get temperature in Celsius
float DHT22::gettemperatureC()
{
	return temperature;
}

//get Humidity
float DHT22::gethumidity()
{
	return humidity;
}

//get temperature in Fahrenheit 
float DHT22::gettemperatureF()
{
	return (temperature * 1.8) + 32; //Return temp in F
}
