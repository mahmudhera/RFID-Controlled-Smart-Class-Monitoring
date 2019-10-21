/*
 * my_header.h
 * Author: Mahmudur Rahman Hera, Amatur Rahman
 */ 

/***********************************************
Description of the header file
************************************************/
/*
This header file includes the collection of headers to make the 16x2 Alphanumeric LCD Display and
the RFID-RC522 Reader Module functioning. It also includes code for SPI interfacing with Atmega32.

This is the original library source. 
https://github.com/asif-mahmud/MIFARE-RFID-with-AVR/blob/master/avr-rfid-library-1.0.0.tar.gz

Modifications were made to get the library working for 
- 16x2 LCD display (original code was for 20x4)
- Interfacing RFID-RC522 with Atmega32 by changing the pin numbers (original code was for Atmega48) 
- The two libraries were merged into one

It requires some additional header files to be included.
-mfrc522.h
-mfrc522_reg.h
-mfrc522_cmd.h
*/


#define BLUE 	2
#define WHITE 	3
#define _CONCAT(a,b) a##b
#define PORT(x) _CONCAT(PORT,x)
#define PIN(x) _CONCAT(PIN,x)
#define DDR(x) _CONCAT(DDR,x)
#define LCD_RS D			//RS SIGNAL
#define LCD_RS_POS PD0

#define LCD_RW D			//RW SIGNAL
#define LCD_RW_POS PD1

#define LCD_E D 			//Enable/strobe signal
#define LCD_E_POS	PD2		//Position of enable in above port

#define LCD_DATA D			//Port PD0-PD3 are connected to D4-D7
#define LCD_DATA_POS 3


/***********************************************
LCD Type Selection
************************************************/
#define LCD_TYPE_162	//For 16 Chars by 2 lines
//************************************************
#define LS_BLINK 0B00000001
#define LS_ULINE 0B00000010
#define LS_NONE	 0B00000000
//************************************************


/***************************************************
LCD F U N C T I O N S PROTOTYPE
****************************************************/
void LCDInit(uint8_t style);
void LCDWriteString(const char *msg);
void LCDWriteInt(int val,unsigned int field_length);
void LCDGotoXY(uint8_t x,uint8_t y);
void LCDHexDumpXY(uint8_t x, uint8_t y,uint8_t d);
//Low level
void LCDByte(uint8_t,uint8_t);
#define LCDCmd(c) (LCDByte(c,0))
#define LCDData(d) (LCDByte(d,1))
//
void LCDBusyLoop();
/***************************************************
E N D  LCD F U N C T I O N S PROTOTYPE
****************************************************/


/***************************************************
L C D    M A C R O S
***************************************************/
#define LCDClear() LCDCmd(0b00000001)
#define LCDHome() LCDCmd(0b00000010);

#define LCDWriteStringXY(x,y,msg) {\
	LCDGotoXY(x,y);\
	LCDWriteString(msg);\
}

#define LCDWriteIntXY(x,y,val,fl) {\
	LCDGotoXY(x,y);\
	LCDWriteInt(val,fl);\
}


/***************************************************
L C D   DEFINITIONS
***************************************************/
//Custom Charset support
#define LCD_DATA_PORT 	PORT(LCD_DATA)
#define LCD_E_PORT 		PORT(LCD_E)
#define LCD_RS_PORT 	PORT(LCD_RS)
#define LCD_RW_PORT 	PORT(LCD_RW)
//
#define LCD_DATA_DDR 	DDR(LCD_DATA)
#define LCD_E_DDR 		DDR(LCD_E)
#define LCD_RS_DDR 		DDR(LCD_RS)
#define LCD_RW_DDR 		DDR(LCD_RW)
//
#define LCD_DATA_PIN	PIN(LCD_DATA)
//
#define SET_E() (LCD_E_PORT|=(1<<LCD_E_POS))
#define SET_RS() (LCD_RS_PORT|=(1<<LCD_RS_POS))
#define SET_RW() (LCD_RW_PORT|=(1<<LCD_RW_POS))
//
#define CLEAR_E() (LCD_E_PORT&=(~(1<<LCD_E_POS)))
#define CLEAR_RS() (LCD_RS_PORT&=(~(1<<LCD_RS_POS)))
#define CLEAR_RW() (LCD_RW_PORT&=(~(1<<LCD_RW_POS)))
/***************************************************
END   L C D   DEFINITIONS
***************************************************/


/***************************************************
L C D   FUNCTION DEFINITION
***************************************************/
void LCDByte(uint8_t c,uint8_t isdata)
{
	//Sends a byte to the LCD in 4bit mode
	//cmd=0 for data
	//cmd=1 for command

	//NOTE: THIS FUNCTION RETURS ONLY WHEN LCD HAS PROCESSED THE COMMAND

	uint8_t hn,ln;			//Nibbles
	uint8_t temp;

	hn=c>>4;
	ln=(c & 0x0F);

	if(isdata==0)
	CLEAR_RS();
	else
	SET_RS();

	_delay_us(0.500);		//tAS

	SET_E();

	//Send high nibble

	temp=(LCD_DATA_PORT & (~(0X0F<<LCD_DATA_POS)))|((hn<<LCD_DATA_POS));
	LCD_DATA_PORT=temp;

	_delay_us(1);			//tEH

	//Now data lines are stable pull E low for transmission

	CLEAR_E();

	_delay_us(1);

	//Send the lower nibble
	SET_E();

	temp=(LCD_DATA_PORT & (~(0X0F<<LCD_DATA_POS)))|((ln<<LCD_DATA_POS));

	LCD_DATA_PORT=temp;

	_delay_us(1);			//tEH

	//SEND

	CLEAR_E();

	_delay_us(1);			//tEL

	LCDBusyLoop();
}

void LCDBusyLoop()
{
	//This function waits till lcd is BUSY

	uint8_t busy,status=0x00,temp;

	//Change Port to input type because we are reading data
	LCD_DATA_DDR&=(~(0x0f<<LCD_DATA_POS));

	//change LCD mode
	SET_RW();		//Read mode
	CLEAR_RS();		//Read status

	//Let the RW/RS lines stabilize

	_delay_us(0.5);		//tAS

	
	do
	{

		SET_E();

		//Wait tDA for data to become available
		_delay_us(0.5);

		status=(LCD_DATA_PIN>>LCD_DATA_POS);
		status=status<<4;

		_delay_us(0.5);

		//Pull E low
		CLEAR_E();
		_delay_us(1);	//tEL

		SET_E();
		_delay_us(0.5);

		temp=(LCD_DATA_PIN>>LCD_DATA_POS);
		temp&=0x0F;

		status=status|temp;

		busy=status & 0b10000000;

		_delay_us(0.5);
		CLEAR_E();
		_delay_us(1);	//tEL
	}while(busy);

	CLEAR_RW();		//write mode
	//Change Port to output
	LCD_DATA_DDR|=(0x0F<<LCD_DATA_POS);

}

void LCDInit(uint8_t style)
{
	/*****************************************************************
	
	This function Initializes the lcd module
	must be called before calling lcd related functions

	Arguments:
	style = LS_BLINK,LS_ULINE(can be "OR"ed for combination)
	LS_BLINK :The cursor is blinking type
	LS_ULINE :Cursor is "underline" type else "block" type

	*****************************************************************/
	
	//After power on Wait for LCD to Initialize
	_delay_ms(30);
	
	//Set IO Ports
	LCD_DATA_DDR|=(0x0F<<LCD_DATA_POS);
	LCD_E_DDR|=(1<<LCD_E_POS);
	LCD_RS_DDR|=(1<<LCD_RS_POS);
	LCD_RW_DDR|=(1<<LCD_RW_POS);

	LCD_DATA_PORT&=(~(0x0F<<LCD_DATA_POS));
	CLEAR_E();
	CLEAR_RW();
	CLEAR_RS();

	//Set 4-bit mode
	_delay_us(0.3);	//tAS

	SET_E();
	LCD_DATA_PORT|=((0b00000010)<<LCD_DATA_POS); //[B] To transfer 0b00100000 i was using LCD_DATA_PORT|=0b00100000
	_delay_us(1);
	CLEAR_E();
	_delay_us(1);
	
	//Wait for LCD to execute the Functionset Command
	LCDBusyLoop();                                    //[B] Forgot this delay

	//Now the LCD is in 4-bit mode

	LCDCmd(0b00001100|style);	//Display On
	LCDCmd(0b00101000);			//function set 4-bit,2 line 5x7 dot format

	
	LCDGotoXY(0,0);

}
void LCDWriteString(const char *msg)
{
	/*****************************************************************
	
	This function Writes a given string to lcd at the current cursor
	location.

	Arguments:
	msg: a null terminated string to print

	Their are 8 custom char in the LCD they can be defined using
	"LCD Custom Character Builder" PC Software.

	You can print custom character using the % symbol. For example
	to print custom char number 0 (which is a degree symbol), you
	need to write

	LCDWriteString("Temp is 30%0C");
	^^
	|----> %0 will be replaced by
	custom char 0.

	So it will be printed like.
	
	Temp is 30°C
	
	In the same way you can insert any symbol numbered 0-7


	*****************************************************************/
	while(*msg!='\0')
	{
		//Custom Char Support
		if(*msg=='%')
		{
			msg++;
			int8_t cc=*msg-'0';

			if(cc>=0 && cc<=7)
			{
				LCDData(cc);
			}
			else
			{
				LCDData('%');
				LCDData(*msg);
			}
		}
		else
		{
			LCDData(*msg);
		}
		msg++;
	}
}

void LCDWriteInt(int val,unsigned int field_length)
{
	/***************************************************************
	This function writes a integer type value to LCD module

	Arguments:
	1)int val	: Value to print

	2)unsigned int field_length :total length of field in which the value is printed
	must be between 1-5 if it is -1 the field length is no of digits in the val

	****************************************************************/

	char str[5]={0,0,0,0,0};
	int i=4,j=0;
	while(val)
	{
		str[i]=val%10;
		val=val/10;
		i--;
	}
	if(field_length==-1)
	while(str[j]==0) j++;
	else
	j=5-field_length;

	if(val<0) LCDData('-');
	for(i=j;i<5;i++)
	{
		LCDData(48+str[i]);
	}
}

void LCDGotoXY(uint8_t x,uint8_t y)
{
	if(x>=16) return;
	
	switch(y)
	{
		case 0:
			break;
		case 1:
			x|=0b01000000;
		break;
	}
	
	x |= 0b10000000;
	LCDCmd(x);
}


void LCDHexDumpXY(uint8_t x, uint8_t y,uint8_t d)
{
	LCDGotoXY(x,y);
	uint8_t byte = '0';
	(((d>>4)&0x0F)<=9) ? (byte='0'+((d>>4)&0x0F)) : (byte='A'+ ((d>>4)&0x0F)-0x0A);
	LCDByte(byte,1);
	LCDBusyLoop();
	
	((d&0x0F)<=9) ? (byte='0'+ (d&0x0F)) : (byte='A'+ (d&0x0F)-0x0A);
	LCDByte(byte,1);
	LCDBusyLoop();
}
/***************************************************
END   L C D   DEFINITIONS
***************************************************/



/***************************************************
spi.c
***************************************************/
/*START spi header*/
#ifndef SPI_H
#define SPI_H
#include <stdint.h>
//START spi_config
/*
 * Set to 1, spi api will work in master mode
 * else in slave mode
 */
#define SPI_CONFIG_AS_MASTER 	1
/*
 * Config SPI pin diagram
 */
#define SPI_DDR		DDRB
#define SPI_PORT	PORTB
#define SPI_PIN		PINB
#define SPI_MOSI	PB5
#define SPI_MISO	PB6
#define SPI_SS		PB4
#define SPI_SCK		PB7
//END spi_config
void spi_init();
uint8_t spi_transmit(uint8_t data);
#define ENABLE_CHIP() (SPI_PORT &= (~(1<<SPI_SS)))
#define DISABLE_CHIP() (SPI_PORT |= (1<<SPI_SS))
/*END spi header*/


#if SPI_CONFIG_AS_MASTER
void spi_init()
{
	SPI_DDR = (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);//prescaler 16
}

uint8_t spi_transmit(uint8_t data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
	
	return SPDR;
}

#else
void spi_init()
{
	SPI_DDR = (1<<SPI_MISO);
	SPCR = (1<<SPE);
}

uint8_t spi_transmit(uint8_t data)
{
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}
#endif
//
/***************************************************
END   spi.c
***************************************************/


/***************************************************
mfrc522.c
***************************************************/
//start mfrc522.c
#include "mfrc522.h"
void mfrc522_init()
{
	uint8_t byte;
	mfrc522_reset();
	
	mfrc522_write(TModeReg, 0x8D);
	mfrc522_write(TPrescalerReg, 0x3E);
	mfrc522_write(TReloadReg_1, 30);
	mfrc522_write(TReloadReg_2, 0);
	mfrc522_write(TxASKReg, 0x40);
	mfrc522_write(ModeReg, 0x3D);
	
	byte = mfrc522_read(TxControlReg);
	if(!(byte&0x03))
	{
		mfrc522_write(TxControlReg,byte|0x03);
	}
}

void mfrc522_write(uint8_t reg, uint8_t data)
{
	ENABLE_CHIP();
	spi_transmit((reg<<1)&0x7E);
	spi_transmit(data);
	DISABLE_CHIP();
}

uint8_t mfrc522_read(uint8_t reg)
{
	uint8_t data;
	ENABLE_CHIP();
	spi_transmit(((reg<<1)&0x7E)|0x80);
	data = spi_transmit(0x00);
	DISABLE_CHIP();
	return data;
}

void mfrc522_reset()
{
	mfrc522_write(CommandReg,SoftReset_CMD);
}

uint8_t	mfrc522_request(uint8_t req_mode, uint8_t * tag_type)
{
	uint8_t  status;
	uint32_t backBits;//The received data bits

	mfrc522_write(BitFramingReg, 0x07);//TxLastBists = BitFramingReg[2..0]	???
	
	tag_type[0] = req_mode;
	status = mfrc522_to_card(Transceive_CMD, tag_type, 1, tag_type, &backBits);

	if ((status != CARD_FOUND) || (backBits != 0x10))
	{
		status = ERROR;
	}
	
	return status;
}

uint8_t mfrc522_to_card(uint8_t cmd, uint8_t *send_data, uint8_t send_data_len, uint8_t *back_data, uint32_t *back_data_len)
{
	uint8_t status = ERROR;
	uint8_t irqEn = 0x00;
	uint8_t waitIRq = 0x00;
	uint8_t lastBits;
	uint8_t n;
	uint8_t	tmp;
	uint32_t i;

	switch (cmd)
	{
		case MFAuthent_CMD:		//Certification cards close
		{
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case Transceive_CMD:	//Transmit FIFO data
		{
			irqEn = 0x77;
			waitIRq = 0x30;
			break;
		}
		default:
		break;
	}
	
	//mfrc522_write(ComIEnReg, irqEn|0x80);	//Interrupt request
	n=mfrc522_read(ComIrqReg);
	mfrc522_write(ComIrqReg,n&(~0x80));//clear all interrupt bits
	n=mfrc522_read(FIFOLevelReg);
	mfrc522_write(FIFOLevelReg,n|0x80);//flush FIFO data
	
	mfrc522_write(CommandReg, Idle_CMD);	//NO action; Cancel the current cmd???

	//Writing data to the FIFO
	for (i=0; i<send_data_len; i++)
	{
		mfrc522_write(FIFODataReg, send_data[i]);
	}

	//Execute the cmd
	mfrc522_write(CommandReg, cmd);
	if (cmd == Transceive_CMD)
	{
		n=mfrc522_read(BitFramingReg);
		mfrc522_write(BitFramingReg,n|0x80);
	}
	
	//Waiting to receive data to complete
	i = 2000;	//i according to the clock frequency adjustment, the operator M1 card maximum waiting time 25ms???
	do
	{
		//CommIrqReg[7..0]
		//Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
		n = mfrc522_read(ComIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitIRq));

	tmp=mfrc522_read(BitFramingReg);
	mfrc522_write(BitFramingReg,tmp&(~0x80));
	
	if (i != 0)
	{
		if(!(mfrc522_read(ErrorReg) & 0x1B))	//BufferOvfl Collerr CRCErr ProtecolErr
		{
			status = CARD_FOUND;
			if (n & irqEn & 0x01)
			{
				status = CARD_NOT_FOUND;			//??
			}

			if (cmd == Transceive_CMD)
			{
				n = mfrc522_read(FIFOLevelReg);
				lastBits = mfrc522_read(ControlReg) & 0x07;
				if (lastBits)
				{
					*back_data_len = (n-1)*8 + lastBits;
				}
				else
				{
					*back_data_len = n*8;
				}

				if (n == 0)
				{
					n = 1;
				}
				if (n > MAX_LEN)
				{
					n = MAX_LEN;
				}
				
				//Reading the received data in FIFO
				for (i=0; i<n; i++)
				{
					back_data[i] = mfrc522_read(FIFODataReg);
				}
			}
		}
		else
		{
			status = ERROR;
		}
		
	}
	
	//SetBitMask(ControlReg,0x80);           //timer stops
	//mfrc522_write(cmdReg, PCD_IDLE);

	return status;
}


uint8_t mfrc522_get_card_serial(uint8_t * serial_out)
{
	uint8_t status;
	uint8_t i;
	uint8_t serNumCheck=0;
	uint32_t unLen;
	
	mfrc522_write(BitFramingReg, 0x00);		//TxLastBists = BitFramingReg[2..0]
	
	serial_out[0] = PICC_ANTICOLL;
	serial_out[1] = 0x20;
	status = mfrc522_to_card(Transceive_CMD, serial_out, 2, serial_out, &unLen);

	if (status == CARD_FOUND)
	{
		//Check card serial number
		for (i=0; i<4; i++)
		{
			serNumCheck ^= serial_out[i];
		}
		if (serNumCheck != serial_out[i])
		{
			status = ERROR;
		}
	}
	return status;
}
//end mfrc22
/***************************************************
END   mfrc22.c
***************************************************/

#endif /* MY_HEADER_H_ */