// 
// Library for communicating with DHT22 and AM2302 temperature and humidity sensors using pin polling.
// Author : Leonid (DLM)
//
// Update your PORT and pin configuration for your device in the beginning of this header.
// Create DHT22 object in program, call readData function, then you can access the updated values using provided functions.
// readData needs to be called every time you want to update the temp and humidity values, it will return -1 if error occurs.
//


//prevent multiple inclusions of header file
#ifndef DHT22_AM2302_V3_H
#define DHT22_AM2302_V3_H

//timeout retries
#define DHT_TIMEOUT 200			// Used to timeout of trying to read sensor if error occurs.

class DHT22 //Class for DHT22 sensor to read and return data.
{
public:
	DHT22();					// Constructor initializes private variables and pin to 0.
	DHT22(int);					// Constructor to select inputPin for the DHT22, defaults to PORTD.
	DHT22(volatile uint8_t *, volatile uint8_t *, volatile uint8_t *, int dataPin);		// example: DHT22 dht22(&DDRB, &PORTB, &PINB, 6) <-Sets Port B pin 6 as data pin
	int8_t readData();			// acquire data from sensor, this function updates the Temp and Humidity values.
	float gettemperatureC();	// returns float temperature in Celsius
	float gettemperatureF();	// returns float temperature in Fahrenheit
	float gethumidity();		// returns float Humidity
private:
	float temperature;			// Float variable to store Temperature in Celsius
	float humidity;				// Float variable to store Relative Humidity 
	volatile uint8_t *dht22_ddr = &DDRD;		// Data Direction Register (DDRX) 
	volatile uint8_t *dht22_port = &PORTD;		// Pin Output Register (PORTX)	
	volatile uint8_t *dht22_pin = &PIND;		// Pin Input Register (PINX)
	uint8_t dht22_inputPin;		// integer to store the Input Pin being used for the sensor (can define default pin here if c++11 support)

};

#endif

/*
Mesiti Electronics 2017
*/