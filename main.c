/*
 * main.c
 * Author: Mahmudur Rahman Hera, Amatur Rahman
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>

//This header includes all the necessary codes for interfacing
//16x2 LCD Alphanumeric Display and RFID-RC522 Reader Module with Atmega32
#include "my_header.h"

/***  We are simulating a classroom environment.  
For this, we need to define the time periods.
By default we allow students to enter for the first 1 minute (ENTRANCE_PERIOD_MINUTE).
The next 1 minute to simulate class running state (SESSION_PERIOD_MINUTE).
The last 1 minute to allow the students to leave the classroom. (AWAIT_PERIOD_MINUTE)

After these 3 phases are over, a check is done to see if all students have exited the classroom.
If some could not get out, the buzzer will buzz of alarming everyone around.
***/
#define ENTRANCE_PERIOD_MINUTE 1
#define SESSION_PERIOD_MINUTE 1
#define AWAIT_PERIOD_MINUTE 1

/**Maximum number of students in a classroom**/
#define  MAX_PEOPLE 3

int program_status;

// used for timer interrupts
volatile double miliseconds;
volatile int min;
volatile int sec;
volatile int hour;

// used for LED animations
volatile int sec_counter;
volatile uint8_t temp_PORTA;
volatile int LED_animation_status;
volatile int LED_animation_on;

volatile int program_status_2_first_time;
volatile int program_status_3_first_time;
volatile int program_status_4_first_time;

//used for EEPROM initialization
int write_enable_eeprom = 1;
int write_enable_update_name_eeprom = 1;
int write_enable_update_date_count_eeprom = 1;
int write_enable_update_date_first_time_eeprom = 1;
uint8_t   EEMEM  NonVolatileDayCount = 0;
uint8_t   EEMEM  NonVolatileHour[6][MAX_PEOPLE];
uint8_t   EEMEM  NonVolatileMinute[6][MAX_PEOPLE];
uint8_t   EEMEM  NonVolatileSecond[6][MAX_PEOPLE];
uint8_t   EEMEM  NonVolatileIsPresent[6][MAX_PEOPLE];
uint8_t   EEMEM  NonVolatileName[MAX_PEOPLE][10] ={"Sibat", "Ripon", "Nimi"};
volatile uint8_t		curr_day;

//To begin database display mode
ISR(INT2_vect)
{
	GIFR |= (1<<INTF2);
	_delay_ms(500); // Software debouncing control
		
	//database showing is enabled only if DPDT push switch is pressed	
	if((PINC & 0x01) == 0x01) {
		LCDClear();
		LCDWriteStringXY(0,0,"Loading database");
		LCDWriteStringXY(0,1,"Please wait...");
		_delay_ms(1000);
	
		for(int day_i=1; day_i<= curr_day; day_i++){
			//show read day count
			LCDClear();
			LCDWriteStringXY(0, 0, "Showing Data of");
			LCDWriteStringXY(0, 1, "Day: ");
			LCDWriteIntXY(6, 1, day_i, 1);
			_delay_ms(1600);
			
			uint8_t eeprom_read_string [MAX_PEOPLE][10];
			int stdCounter = 0;
			for(int i=0 ; i<MAX_PEOPLE; i++){
				uint8_t  NonVolatileIsPresentRead  = eeprom_read_byte (& NonVolatileIsPresent[day_i][i]);
				if (NonVolatileIsPresentRead == 1){
					stdCounter++;
					eeprom_read_block ((void*) eeprom_read_string[i] , (const	void *) NonVolatileName[i] , 10);
					uint8_t  read_hour = eeprom_read_byte (&NonVolatileHour[day_i][i] );
					uint8_t  read_min = eeprom_read_byte (&NonVolatileMinute[day_i][i] );
					uint8_t  read_sec = eeprom_read_byte (&NonVolatileSecond[day_i][i] );
					
					//show "at time"
					LCDClear();
					LCDWriteStringXY(0, 0,"At ");
					LCDWriteIntXY(3, 0, read_hour, 2);
					LCDWriteStringXY(5, 0,":");
					LCDWriteIntXY(6, 0,  read_min, 2);
					LCDWriteStringXY(8, 0,":");
					LCDWriteIntXY(9,0,  read_sec, 2);
					//show who
					LCDWriteIntXY(0, 1, stdCounter, 2);
					LCDWriteStringXY(2, 1, ".");
					LCDWriteStringXY(3, 1, eeprom_read_string[i]);
					LCDWriteStringXY(10, 1, " in");
					_delay_ms(3000);
				}
			}
			//show total present
			LCDClear();
			LCDWriteStringXY(0, 0, "Total Present:");
			LCDWriteIntXY(0, 1, stdCounter, 2);
			_delay_ms(2000);	
		}
		
		LCDClear();
		LCDWriteStringXY(0,0,"Exiting database");
		LCDWriteStringXY(0,1,"Please wait...");
		_delay_ms(2000);
		LCDClear();
	}else{
		//when dpdt is off
		LCDClear();
		LCDWriteStringXY(0,0,"Current Time:");
		LCDWriteIntXY(0,1, hour, 2);
		LCDWriteStringXY(2, 1,":");
		LCDWriteIntXY(3, 1, min, 2);
		LCDWriteStringXY(5,1,":");
		LCDWriteIntXY(6,1, sec, 2);
		LCDWriteStringXY(9, 1, ", Day ");
		LCDWriteIntXY(14, 1, curr_day, 1);
		_delay_ms(2000);
		LCDClear();
	}
	
	DDRC |= (1<<PC0);//Makes first pin of PORTC as Output
	PORTC &= ~(1<<PC0);
	DDRC &= ~(1<<PC0);//Makes first pin of PORTC as Input
	
	GIFR |= (1<<INTF2); // clear interrupt flag -> before adding this, this ISR was being executed twice in a row
}



/** In different phases of the program, the RGB LED pair will take different hues and blink continuosly **/
void animate_LED()
{
	if(LED_animation_on == 0)
	{
		return;
	}
	sec_counter++;
	if(sec_counter >= 1)
	{
		if(LED_animation_status == 0)
		{
			temp_PORTA = PORTA;
			PORTA = 0xFF;
			LED_animation_status = 1;
		}
		else
		{
			PORTA = temp_PORTA;
			LED_animation_status = 0;
		}
		sec_counter = 0;
	}
}


/**For time calculation**/
void add_milisecond(float ms)
{
	miliseconds += ms;
	if(miliseconds >= 1000)
	{
		sec++;
		animate_LED();
		miliseconds -= 1000;
	}
	if(sec == 60)
	{
		min++;
		sec = 0;
	}
	if(min == 60)
	{
		min = 0;
		hour++;
	}
}

/**To determine which state or phase the classroom is in**/
void calculate_program_state()
{
	int current_time = hour * 60 + min;
	if(current_time >= ENTRANCE_PERIOD_MINUTE)
	{
		program_status = 2;
		if(program_status_2_first_time == 1)
		{
			PORTA = 0xFB;
			temp_PORTA = PORTA;
			program_status_2_first_time = 0;
		}
	}
	if(current_time >= ENTRANCE_PERIOD_MINUTE + SESSION_PERIOD_MINUTE)
	{
		program_status = 3;
		if(program_status_3_first_time == 1)
		{
			PORTA = 0xFD;
			temp_PORTA = PORTA;
			program_status_3_first_time = 0;
		}
	}
	if(current_time >= ENTRANCE_PERIOD_MINUTE + SESSION_PERIOD_MINUTE + AWAIT_PERIOD_MINUTE)
	{
		program_status = 4;
		if(program_status_3_first_time == 1)
		{
			PORTA = 0xFB;
			temp_PORTA = PORTA;
			program_status_3_first_time = 0;
		}
	}
}


ISR(TIMER1_OVF_vect)
{
	add_milisecond(65.536);
	calculate_program_state();
}

//some more initialization of EEPROM
void increase_day_count_eeprom(){
	curr_day = eeprom_read_byte (& NonVolatileDayCount);
	curr_day++;
	if(curr_day>5){
		curr_day = 1;
	}
	if(write_enable_update_date_count_eeprom){
		eeprom_update_byte ((uint8_t*) (&NonVolatileDayCount), curr_day);
	}
	for(int i=0;i<MAX_PEOPLE;i++){
		eeprom_update_byte ((uint8_t*) (&NonVolatileIsPresent[curr_day][i]), 0);
	}
}

int main()
{
	DDRC &= ~(1<<PC0);				//input for DPDT switch
	increase_day_count_eeprom();	//some more initialization of EEPROM
	
	/*** Students list ***/
	/** These are the bytes that are read from the RFID tags of the students**/
	uint8_t person_byte[MAX_PEOPLE][5] = {
		{0x23, 0x6D, 0xD6, 0x00, 0x98} , 
		{0xF9, 0x46, 0x1D, 0x00, 0xA2}, 
		{0xA3, 0x7E, 0x30, 0x02, 0xEF}
		
	};
	
	//Some tags that we used to experiment
	//{0xF9, 0x46, 0x1D, 0x00, 0xA2}
	//{0x23, 0x6D, 0xD6, 0x00, 0x98} - recognized1
	//{0xF9, 0x46, 0x1D, 0x00, 0xA2} - recognized2 (change any bit in any of these 2 to see effect of unrecognized person entering)
	//{0xA3, 0x7E, 0x30, 0x02, 0xEF} - Nimi - white
	//{0x5B, 0xA8, 0x2C, 0x00, 0xDF} - Adnan - blue
		
	// person name list .. these used to control people names etc
	int detected_person;
	int person_entry_list[MAX_PEOPLE] = {0, 0, 0};
	char *person_name[MAX_LEN] = { (char *)"Sibat", (char *)"Ripon" , (char *)"Nimi" };
	char *entered_msg = (char *)" entered";
	char *left_msg = (char *)" left";
	char msg_to_show[100];
	int person_count = 0;
	
	// iterator and byte array to use later
	uint8_t byte, i;
	uint8_t str[MAX_LEN];
	_delay_ms(50);
	
	// initialize the LCD
	LCDInit(LS_BLINK);
	LCDWriteStringXY(2,0,"RFID Reader");
	
	// spi initialization
	spi_init();
	_delay_ms(1000);
	LCDClear();
	
	//init reader
	mfrc522_init();
	
	// poll until reader is found
	while(1)
	{
		byte = mfrc522_read(VersionReg);
		if(byte == 0x92)
		{
			LCDClear();
			LCDWriteStringXY(2,0,"Detected");
			_delay_ms(1000);
			break;
		}
		else
		{
			LCDClear();
			LCDWriteStringXY(0,0,"No reader found");
			_delay_ms(800);
			LCDClear();
			LCDWriteStringXY(0, 0, "Connect reader");
			_delay_ms(1000);
		}
	}
	
	// ready to run
	_delay_ms(1500);
	LCDClear();
	
	// initializing the RGB LEDs
	DDRA = 0xFF;
	PORTA = 0xFE;
	
	
	//Interrupt INT2
	DDRB |= (1<<PB2);		// Set PB2 as input (Using for interrupt INT2)
	PORTB |= (1<<PB2);		// Enable PB2 pull-up resistor
	
	// setting up the timer codes
	// setting up the LED animation initials
	// TODO timer code ... START HERE ...
	TCCR1A = 0x00;
	TCCR1B = 0x01;
	TIMSK = 0x04;
	min = 0;
	sec = 0;
	hour = 0;
	sec_counter = 0;
	LED_animation_status = 0;
	LED_animation_on = 1;
	program_status_2_first_time = 1;
	program_status_3_first_time = 1;
	program_status_4_first_time = 1;
	
	// TODO timer code ... ENDS  HERE ...
	
	// The loop starts here
	// program status : 1 means entrance period
	//				  : 2 means session period
	//				  : 3 means session ended
	program_status = 1;
	
	//Using interrupt 2 for reading history mode
	GICR &= ~(1<<INT2);		// Disable INT2
	MCUCSR |= (1<<ISC2);	// Trigger INT2 on 1 = rising edge , 0 falling edge
	GIFR |= (1<<INTF2);	// clear INTF2
	GICR |= (1<<INT2);		// Enable INT2
	
	sei();
	while(1)
	{
		spi_init();
		mfrc522_init();
		byte = mfrc522_read(ComIEnReg);
		mfrc522_write(ComIEnReg,byte|0x20);
		byte = mfrc522_read(DivIEnReg);
		mfrc522_write(DivIEnReg,byte|0x80);
		
		if(program_status == 1)
		{
			LCDClear();
			LCDWriteStringXY(0, 0, "Show your card.");
			LCDWriteStringXY(0, 1, "#students:");
			LCDWriteIntXY(12,1, person_count ,2 );
			
			byte = mfrc522_request(PICC_REQALL,str);
			if(byte == CARD_FOUND)
			{
				byte = mfrc522_get_card_serial(str);
				if(byte == CARD_FOUND)
				{
					LED_animation_on = 0;
					
					// check for person 1
					detected_person = -1;
					for(i = 0; i < MAX_PEOPLE; i++)
					{
						for(byte = 0; byte < 5; byte++)
						{
							/***The following piece of code was written to test and read the bytes off the tags and print them***/
							//LCDClear();
							//LCDWriteIntXY(0,0,byte,2);
							//LCDWriteIntXY(0,1,str[byte],14);
							//_delay_ms(3000);
							
							if(str[byte] != person_byte[i][byte])
							{
								break;
							}
						}
						if(byte == 5)
						{
							detected_person = i;
							break;
						}
					}
					
					if(i == MAX_PEOPLE)
					{
						detected_person = -1;
					}
					
					// showing message on LCD upon decision of the person
					LCDClear();
					if(detected_person == -1)
					{
						PORTA = 0x7E;
						LCDClear();
						LCDWriteStringXY(0, 0, "Access denied!");
						_delay_ms(2000);
						PORTA = 0xFE;
						LCDClear();
						LCDWriteStringXY(0, 0, "Unrecognized!");
					}
					else
					{
						PORTA = 0xFD;
						LCDClear();
						LCDWriteStringXY(0, 0, "Access granted!");
						_delay_ms(2000);
						if(person_entry_list[i] == 0)
						{
							person_entry_list[i] = 1;
						}
						else
						{
							person_entry_list[i] = 0;
						}
						strcpy(msg_to_show, person_name[detected_person]);
						if(person_entry_list[i] == 0)
						{
							/** Code to sound buzzer when a student is leaving **/
							PORTA = 0x7D;
							_delay_ms(200);
							PORTA = 0xFD;
							// end of buzzer code
							
							//EEPROM WRITE
							if(write_enable_eeprom == 1){
								eeprom_update_byte ((uint8_t*) (&NonVolatileIsPresent[curr_day][i]), 0);
							}
							person_count--;
							strcat(msg_to_show, left_msg);
						}
						else
						{
							/** Code to sound buzzer when a student is entering **/
							PORTA = 0x7D;
							_delay_ms(200);
							PORTA = 0xFD;
							_delay_ms(100);
							PORTA = 0x7D;
							_delay_ms(200);
							PORTA = 0xFD;
							// end of buzzer code
							
							//EEPROM WRITE
							if(write_enable_eeprom == 1) {
								eeprom_write_byte ((uint8_t*) (&NonVolatileIsPresent[curr_day][i]), 1);
								eeprom_update_byte ((uint8_t*) (&NonVolatileHour[curr_day][i]), hour);
								eeprom_update_byte ((uint8_t*) (&NonVolatileMinute[curr_day][i]), min);
								eeprom_update_byte ((uint8_t*) (&NonVolatileSecond[curr_day][i]), sec);
							}
							person_count++;
							strcat(msg_to_show, entered_msg);
						}
						LCDClear();
						LCDWriteStringXY(0, 0, msg_to_show);
					}
					
					_delay_ms(3000);
					
					PORTA = 0xFE;
					LCDClear();
					LED_animation_on = 1;
				}
				else
				{
					LCDClear();
					LCDWriteStringXY(0,1,"Error");
				}
			}
			
			_delay_ms(200);
		}
		else if(program_status == 2)
		{
			LCDClear();
			LCDWriteStringXY(0, 0, "In session now!");
			LCDWriteStringXY(0, 1, "#students:");
			LCDWriteIntXY(12,1, person_count ,2 );
			
			byte = mfrc522_request(PICC_REQALL,str);
			if(byte == CARD_FOUND)
			{
				LED_animation_on = 0;
				PORTA = 0x7E;
				LCDClear();
				LCDWriteStringXY(0, 0, "Warning!!");
				_delay_ms(2000);
				LCDClear();
				LCDWriteStringXY(0, 0, "Session running.");
				_delay_ms(2000);
				PORTA = 0xFE;
				LCDClear();
				LCDWriteStringXY(0, 0, "Can't go out/in.");
				_delay_ms(2000);
				LCDClear();
				PORTA = 0xFB;
				LED_animation_on = 1;
			}
			_delay_ms(200);
		}
		else if(program_status == 3)
		{
			LCDClear();
			LCDWriteStringXY(0, 0, "Session ended!");
			LCDWriteStringXY(0, 1, "#students:");
			LCDWriteIntXY(12,1, person_count ,2 );
			
			byte = mfrc522_request(PICC_REQALL,str);
			if(byte == CARD_FOUND)
			{
				byte = mfrc522_get_card_serial(str);
				if(byte == CARD_FOUND)
				{
					LED_animation_on = 0;
					
					// check for person 1
					detected_person = -1;
					for(i = 0; i < MAX_PEOPLE; i++)
					{
						for(byte = 0; byte < 5; byte++)
						{
							if(str[byte] != person_byte[i][byte])
							{
								break;
							}
						}
						if(byte == 5)
						{
							detected_person = i;
							break;
						}
					}
					
					// showing message on LCD upon decision of the person
					if(detected_person == -1)
					{
						PORTA = 0x7E;
						LCDClear();
						LCDWriteStringXY(0, 0, "Unrecognized!");
						_delay_ms(2000);
						PORTA = 0xFE;
					}
					else
					{
						
						PORTA = 0xFB;
						if(person_entry_list[detected_person] != 0){
							LCDClear();
							strcpy(msg_to_show, "Take care ");
							strcat(msg_to_show, person_name[detected_person]);
							LCDWriteStringXY(0, 0, msg_to_show);
							_delay_ms(1000);
							LCDClear();
							strcpy(msg_to_show, person_name[detected_person]);
							strcat(msg_to_show, left_msg);
							LCDWriteStringXY(0, 0, msg_to_show);
							person_entry_list[detected_person] = 0;
							person_count--;
							//unnecessary sanity check
							if(person_count == 0)
							{
								break;
							}
						}
						
					}
					
					_delay_ms(3000);
					
					PORTA = 0xFD;
					LCDClear();
					LED_animation_on = 1;
				}
				else
				{
					PORTA = 0x7E;
					LCDClear();
					LCDWriteStringXY(0,1,"Error");
					_delay_ms(2000);
					PORTA = 0xFE;
					LCDClear();
					PORTA = 0xFD;
				}
			}
			
			_delay_ms(200);
		}
		else
		{
			/***This is the warning phase.
			When leaving period has ended, but some students were still stuck in the classroom,
			the buzzer buzzes off continuously suspecting that some students might be sick or in trouble.
			***/
			if(person_count != 0)
			{
				LED_animation_on = 0;
				PORTA = 0x7E;
				
				LCDClear();
				LCDWriteIntXY(0,0, person_count ,2 );
				LCDWriteStringXY(3, 0, "student could");
				LCDWriteStringXY(0, 1, "not get out");
				_delay_ms(1500);
				LCDClear();
				LCDWriteStringXY(0, 0, "Take caution!");
				_delay_ms(1500);
				PORTA = 0xFE;
				_delay_ms(400);
			}
			else
			{
				/**When all students have left, nothing to do. **/
				LCDClear();
				break;
			}
		}
	}
	
	LED_animation_on = 1;
	PORTA = 0xFB;
	temp_PORTA = PORTA;
	LCDClear();
	LCDWriteStringXY(0, 0, "Everyone left.");
	while(1)
	{
		;
	}
	
	cli();
	
}