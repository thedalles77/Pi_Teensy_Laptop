// This program is in the public domain.
// Release History:
// Original Release - Feb 14, 2018
//
// Execute this program at startup so that it can monitor
// the battery state of charge every minute.
// At 10% SoC, a new terminal window displays a low battery warning. 
// At 7% SoC, the LCD blinks off and on to get the users attention. 
// At 5% SoC, a safe shutdown is executed.
// 
// This program reads the laptop battery status registers over a bit-
// bang SMBus created with two of the Pi's GPIO pins and wiringPi. 
// SMBus Data and clock are pulled to 3.3 volts with 3K resistors.
// Data is wired from Pi GPIO 19 to battery pin 3.
// Clock is wired from Pi GPIO 26 to battery pin 4.
//
// Add -l wiringPi to the Compile & Build and sudo to the execute per:
// https://learn.sparkfun.com/tutorials/raspberry-gpio/using-an-ide
//
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

// Pin number declarations
const int clock = 26; // SMBus clock on Pin 37, GPIO26
const int data = 19; // SMBus data on Pin 35, GPIO19
const int quarter = 10; // quarter period time in usec

// Global variables
int error = 0; // set to 1 when battery gives a NACK

// Functions
void go_z(int pin) // float the pin and let pullup or battery set level
{
	pinMode(pin, INPUT); // set pin as input to tri-state the driver
}
//
void go_0(int pin) // drive the pin low
{
	pinMode(pin, OUTPUT); // set pin as output
	digitalWrite(pin, LOW); // drive pin low
}
//
int read_pin(int pin) // read the pin and return logic level
{
	pinMode(pin, INPUT); // set pin as input
	return (digitalRead(pin)); // return the logic level
}
//
void setupbus(void)
{
	wiringPiSetupGpio(); //Init wiringPi using the Broadcom GPIO numbers
	piHiPri(99); //Make program the highest priority (doesn't help)
	go_z(clock); // set clock and data to inactive state
	go_z(data);
	delayMicroseconds(200); // wait before sending data
}
//
void startbus(void)
{
	delayMicroseconds(1000); // needed when doing multiple reads
	go_0(data);	// start condition - data low when clock goes low
	delayMicroseconds(quarter);
	go_0(clock);
	delayMicroseconds(4 * quarter); // wait 1 period before proceeding
}
//
void send8(int sendbits)
{
	// send bit 7
	if ((sendbits & 0x80) == 0x80) // check if bit 7 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	}
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 6
	if ((sendbits & 0x40) == 0x40) // check if bit 6 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	}
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 5
		if ((sendbits & 0x20) == 0x20) // check if bit 5 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 4
		if ((sendbits & 0x10) == 0x10) // check if bit 4 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 3
		if ((sendbits & 0x08) == 0x08) // check if bit 3 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 2
		if ((sendbits & 0x04) == 0x04) // check if bit 2 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 1
		if ((sendbits & 0x02) == 0x02) // check if bit 1 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// send bit 0
		if ((sendbits & 0x01) == 0x01) // check if bit 0 set
	{
		go_z(data); // send high
	}
		else
	{
		go_0(data); // send low
	} 
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// ack/nack
	delayMicroseconds(quarter * 4);
	go_z(data); // float data to see ack
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	// read data to see if battery sends a low (acknowledge transfer)
	if (read_pin(data))
	{
		error = 1; // battery did not acknowledge the transfer
	}
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	go_0(data); // data low
	delayMicroseconds(quarter * 90);
}
//
void sendrptstart(void) // send repeated start condition
{				
	go_z(data); // data high
	delayMicroseconds(quarter * 8);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(data); // data low
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter * 16);
}
//
int read16(void) // read low and high bytes
{
	int readval = 0x00; // initialize read word to zero
	// bit 7 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0080;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 6 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0040;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 5 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0020;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 4 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0010;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 3 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0008;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 2 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0004;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 1 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0002;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 0 of low byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0001;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// ack/nack of low byte
	delayMicroseconds(quarter * 2);
	go_0(data); // send ack back to battery
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	go_0(data); // data low
	delayMicroseconds(quarter * 40);		
	// bit 7 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x8000;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 6 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x4000;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 5 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x2000;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 4 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x1000;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 3 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0800;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 2 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0400;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 1 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0200;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// bit 0 of high byte
	go_z(data); 
	delayMicroseconds(quarter);
	if (read_pin(data))
	{
		readval = readval | 0x0100;
	}
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	delayMicroseconds(quarter);
	// ack/nack of high byte
	delayMicroseconds(quarter * 2);
	go_z(data); // send nack back to battery
	delayMicroseconds(quarter);
	go_z(clock); // clock high
	delayMicroseconds(quarter * 2);
	go_0(clock); // clock low
	go_0(data); // data low
	delayMicroseconds(quarter * 8);	
	return readval;
}
//
void stopbus(void) // stop condition, data low when clock goes high
{
	go_z(clock); // clock high
	delayMicroseconds(quarter);	
	go_z(data);	// data high
	delayMicroseconds(quarter * 30);	
}

// Main program	
int main(void)
{        
	delay(1000); // wait a second before starting
	setupbus(); // setup the GPIO SMBus
	printf("Battery State of Charge monitor program running\n");
	int led_on = 0; // variable to keep track of when warning led is on
	int soc; // variable to store the state of charge
	unsigned short bat_stat; // variable to store the battery status
	while(1)  // infinite loop
	{
		// Read Battery status to see if charger is plugged in
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x16); // load register pointer 0x16
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		bat_stat = read16();
		stopbus(); // send stop condition
		if ((bat_stat == 0xffff) | (error))// read again if all 1's or nack
		{
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x16); // load register pointer 0x16
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			bat_stat = read16();
			stopbus(); // send stop condition
		}  
	// Only proceed with reading the SoC if discharge bit is set
		if ((bat_stat & 0x0040) == 0x0040)
		{		
	// Read Battery Relative State of Charge
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x0d); // load soc register pointer 0x0d
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			soc = read16(); // read soc low & high bytes
			stopbus(); // send stop condition
			if ((soc >= 150) | (error))//check if out of range or any nack's
			{	// try again 
				startbus(); // send start condition
				send8(0x16); // send battery address 0x16 (0x0b w/ write)
				send8(0x0d); // load register pointer 0x0d
				sendrptstart(); // send repeated start codition				
				send8(0x17); // send battery address 0x17 (0x0b w/ read)
				soc = read16(); //read low & high bytes
				stopbus(); // send stop condition
			}
			// Check the battery State of Charge for the following:
			// <= 5% causes a safe shutdown.
			// <= 7% causes the display to blink off and on.
			// <= 10% turns on the disk LED as a warning.
			if (soc <= 5) // check for shutdown condition
			{   
				system("sudo shutdown -h now"); // safe shutdown of Pi
				// note that a systemd unit file sends 
				// i2cset -y 1 0x08 0x00 0x5a
				// which commands the Teensy to turn off the power 
			}
			else if (soc <= 7) // check for blink display condition
			{
				system("i2cset -y 1 0x08 0x00 0xe2"); // blink the display
			}
			else if ((soc <= 10) && (!led_on)) // soc at 10% and LED is off
			{
				system("i2cset -y 1 0x08 0x00 0x10"); // turn on disk LED
				led_on = 0x01; // variable shows led is turned on
			}
		}
		else if (led_on) // charger plugged in and LED is on
			{
				system("i2cset -y 1 0x08 0x00 0x11"); // turn off disk LED
				led_on = 0x00; // variable shows led is turned off
			}
	delay(60000);	// wait 60 seconds before repeating the loop
	}
	return 0;
}

