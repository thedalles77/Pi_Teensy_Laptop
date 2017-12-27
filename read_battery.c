// This program reads the laptop battery status registers over a bit-
// bang SMBus created with two of the Pi's GPIO pins and wiringPi. 
// SMBus Data and clock are pulled to 3.3 volts with 3K resistors.
// Data is wired from Pi GPIO 19 to battery pin 3.
// Clock is wired from Pi GPIO 26 to battery pin 4.
//
// This program does not monitor the clock for clock stretching.
// The bus was monitored with a logic analyzer to see when the battery
// holds the clock low and large delays were added before the Pi sends 
// more data. Sometimes the program reads back FFFF because Linux
// will switch to some other task and mess up the timing of the bus.
// The program does a second read if the value is out of range.
//
// Add -l wiringPi to the Compile & Build and sudo to the execute per:
// https://learn.sparkfun.com/tutorials/raspberry-gpio/using-an-ide
//
#include <stdio.h>
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
	setupbus(); // setup before data transfer
//***************Battery Status**********
	error = 0; // initialize to no error
	startbus(); // send start condition
	send8(0x16); // send battery address 0x16 (0x0b w/ write)
	send8(0x16); // load register pointer 0x16
	sendrptstart(); // send repeated start codition				
	send8(0x17); // send battery address 0x17 (0x0b w/ read)
	unsigned short bat_stat = read16();
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
	}  //bat_stat printf comes after all the other registers are printed
	// Only proceed with reading the other registers if bat_stat is OK
	if (bat_stat != 0xffff)
//****************Voltage********	
	{
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x09); // load register pointer 0x09 
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		float bat_voltage = (float)read16()/1000;// convert mvolts to volts
		stopbus(); // send stop condition
		// check if out of range or if any NACKs were given by the battery	
		if ((bat_voltage >= 22) | (bat_voltage <= 6) | (error))
		{// try again and printf the result (good or bad)
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x09); // load register pointer 0x09 
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			bat_voltage = (float)read16()/1000;// convert mvolts to volts
			stopbus(); // send stop condition
			printf ("Voltage =  %6.3f Volts\n", bat_voltage);		  
		}
		else
		{// 1st try was in range and Ack'ed so printf the result 
			printf ("Voltage = %6.3f Volts\n", bat_voltage);
		}
	
//***************Current**********
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x0a); // load register pointer 0x0a
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		short bat_current = (short)read16();// signed 16 bit ma current
		stopbus(); // send stop condition
		// check if out of range or if any NACKs were given by the battery
		if ((bat_current >= 3000) | (bat_current == -1) | (error))
		{// try again and printf the result (good or bad)
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x0a); // load register pointer 0x0a
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			bat_current = (short)read16();// signed 16 bit ma current
			stopbus(); // send stop condition
			printf ("Current =  %d mA\n", bat_current);
		}
		else
		{// 1st try was in range and Ack'ed so printf the result 
			printf ("Current = %d mA\n", bat_current);
		}

//********Temperature********
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x08); // load register pointer 0x08
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		float temper = (float)read16()/10-273.15;//0.1K unit converted to C
		stopbus(); // send stop condition
		if ((temper >= 40) | (error)) // check if out of range or any NACK's 
		{   // try again and printf the result (good or bad)
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x08); // load register pointer 0x08
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			temper = (float)read16()/10-273.15;//0.1K unit converted to C
			stopbus(); // send stop condition		
			printf ("Temperature =  %5.2f degrees C\n", temper);
		}
		else
		{// 1st try was in range and Ack'ed so printf the result 
			printf ("Temperature = %5.2f degrees C\n", temper);
		}

//***************Relative State of Charge**********
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x0d); // load register pointer 0x0d
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		int soc = read16(); //read low&high bytes
		stopbus(); // send stop condition
		if ((soc >= 150) | (error)) // check if out of range or any nack's
		{// try again and printf the result (good or bad)
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x0d); // load register pointer 0x0d
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			soc = read16(); //read low&high bytes
			stopbus(); // send stop condition
			printf ("State of Charge =  %d percent\n", soc);
		}
		else
		{// 1st try was in range and Ack'ed so printf the result 
			printf ("State of Charge = %d percent\n", soc);
		}
    
//***************Average Time to Empty**********
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x12); // load register pointer 0x12
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		unsigned int time_to_empty = read16();
		stopbus(); // send stop condition	
		if ((time_to_empty <= 1000) & (!error)) // check if in range and ack
		{
			printf ("Time to empty = %d minutes\n", time_to_empty);
		}
		else
		{
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x12); // load register pointer 0x12
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			time_to_empty = read16();
			stopbus(); // send stop condition	
			if (time_to_empty <= 1000) //don't show bad values when charging
			{
				printf ("Time to empty =  %d minutes\n", time_to_empty);
			}
		} // Don't show FFFF minutes when at 100% soc and charger hooked up
		
		
//***************Average Time to Full**********
		error = 0; // initialize to no error
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x13); // load register pointer 0x13
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		unsigned int time_to_full = read16();
		stopbus(); // send stop condition
		// check if in range and not zero and ack)
		if ((time_to_full <= 1000) & (time_to_full != 0) & (!error)) 
		{
			printf ("Time to full = %d minutes\n", time_to_full);
		}
		else
		{
			error = 0; // initialize to no error
			startbus(); // send start condition
			send8(0x16); // send battery address 0x16 (0x0b w/ write)
			send8(0x13); // load register pointer 0x13
			sendrptstart(); // send repeated start codition				
			send8(0x17); // send battery address 0x17 (0x0b w/ read)
			time_to_full = read16();
			stopbus(); // send stop condition
		// Don't show FFFF minutes when charger not hooked up
		// Don't show 0 minutes when at 100 SOC and charger hooked up
			if ((time_to_full <= 1000) & (time_to_full != 0))  
			{
				printf ("Time to full =  %d minutes\n", time_to_full);
			}
		}
	
//************Print Battery Status**********
		printf ("Battery Status\n");
//    	printf ("Battery Status = %#06x Hex\n", bat_stat);	
		if ((bat_stat & 0x8000) == 0x8000)  
		{
			printf ("   OVERCHARGE ALARM\n");
		}
		if ((bat_stat & 0x4000) == 0x4000)  
		{
			printf ("   TERMINATE CHARGE ALARM\n");
		}
		if ((bat_stat & 0x1000) == 0x1000)  
		{
			printf ("   OVER TEMP ALARM\n");
		}
		if ((bat_stat & 0x0800) == 0x0800)  
		{
			printf ("   TERMINATE DISCHARGE ALARM\n");
		}	
		if ((bat_stat & 0x0200) == 0x0200)  
		{
			printf ("   REMAINING CAPACITY ALARM\n");
		}	
		if ((bat_stat & 0x0100) == 0x0100)  
		{
			printf ("   REMAINING TIME ALARM\n");
		}	
		if ((bat_stat & 0x0080) == 0x0080)  
		{
			printf ("   Initialized\n");
		}	
		if ((bat_stat & 0x0040) == 0x0040)  
		{
			printf ("   Discharging\n");
		}	
		if ((bat_stat & 0x0020) == 0x0020)  
		{
			printf ("   Fully Charged\n");
		}	
		if ((bat_stat & 0x0010) == 0x0010)  
		{
			printf ("   Fully Discharged\n");
		}
/*		if ((bat_stat & 0x240f) != 0x0000)  // check if unknown bits set
		{
			printf ("   Unknown\n");
		}
*/	
//*******Generic Register Read**********
/*
		unsigned int reg_pointer = 0x09;// initialized but changed by user
		printf ("Enter the register to read in Hex, ie 0x?? "); 
		scanf ("%x", &reg_pointer);
		printf ("0x%02x Register", reg_pointer);// show register to read
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(reg_pointer); // load register pointer
		sendrptstart(); // send repeated start codition				
		send8(0x17); // send battery address 0x17 (0x0b w/ read)
		unsigned int value = read16();
		stopbus(); // send stop condition
		printf (" = %#06x Hex, %d decimal\n", value, value);
*/
//*Register Write Example***Sets Remaining Time Alarm reg 0x02 to 10 min
/*        
		startbus(); // send start condition
		send8(0x16); // send battery address 0x16 (0x0b w/ write)
		send8(0x02); // load register pointer 0x02
	// Note: there is no repeated start on a write 
		send8(0x0a); // send low byte of 0x0a = 10 decimal minutes
		send8(0x00); // send high byte of 0x00	
		stopbus(); // send stop condition
*/
    }
    else    // the bat_stat read was FFFF so no battery communication
    {
		printf ("The battery did not respond\n");
	}
	return 0;
}

