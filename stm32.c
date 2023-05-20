
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Wire.h>
#include <SPI.h>

#define DELIMITER 0
#define SPACE 1
#define PERIOD 2
#define JUNK 3

//Identity 
#define Speed_ID 1
#define Temp_ID 2
#define Volt_ID 3
#define MISC_ID 4
#define NOT_A_ID_BYTE 9

//Variables for the PT100 boards
double resistance;
uint8_t reg1, reg2; //reg1 holds MSB, reg2 holds LSB for RTD
uint16_t fullreg; //fullreg holds the combined reg1 and reg2
double temperature; //global variable
//Variables and parameters for the R - T conversion
double Z1, Z2, Z3, Z4, Rt;
double RTDa = 3.9083e-3;
double RTDb = -5.775e-7;
double rpoly = 0;

/*Temperature Sensor SPI pins*/
const int chipSelectPin = 10; //CS pin #10
//pin 12 - MISO
//pin 11 - MOSI
//pin 13 - SCK


int binaryToDecimal(char* binary)
{
	int num;
	sscanf(binary, "%d", &num);
    int dec_value = 0;

    // Initializing base value to 1, i.e 2^0
    int base = 1;

    int temp = num;
    while (temp) {
        int last_digit = temp % 10;
        temp = temp / 10;

        dec_value += last_digit * base;

        base = base * 2;
    }

    return dec_value;
}

//if result is between 48 - 57 then value's decimal
int is_Decimal(char* buf){
	int value;
	int is_dec;

	value = binaryToDecimal(buf); //decimal value of binary

	if(value >= 48 && value <= 57){
		is_dec = 1;
	} else{
		is_dec = 0;
	}

	return is_dec;
}

int check_nondec_symbol(char* buf){
	int symbol_identifier;
	//first check if it's a delimiter byte
	if(strcmp(buf,"00000000") == 0 || strcmp(buf,"00001010") == 0){ //if NULL or New line
		symbol_identifier = DELIMITER; //0 means delimiter
	}
	else if(strcmp(buf,"00100000") == 0){ //if space
		symbol_identifier = SPACE; //1 means space
	}
	else if(strcmp(buf,"00101110") == 0){ //if .
		symbol_identifier = PERIOD; //2 means .
	}
	else{ //if anything else
		symbol_identifier = JUNK; //3 means JUNK (later parse them out)
	}

	return symbol_identifier;

}

int identity_byte(char* buf){
	int value_type;
	if(strcmp(buf, "01010011") == 0) { //if S (Speed)
		value_type = Speed_ID;
	}
	else if(strcmp(buf, "01010100") == 0) { //if T (Temp)
		value_type = Temp_ID;
	}
	else if(strcmp(buf, "01010110") == 0) { //if V (Voltage)
		value_type = Volt_ID;
	}
	else{
		value_type = NOT_A_ID_BYTE; //if junk value then do nothing (random value LOL)
	}
	return value_type;
}

char* ascii_to_decimal(char* buf){
	char* decimal;

	if(strcmp(buf,"00110000") == 0){
		decimal = "0";
	}
	else if(strcmp(buf,"00110001") == 0){
		decimal = "1";
	}
	else if(strcmp(buf, "00110010") == 0){
		decimal = "2";
	}
	else if(strcmp(buf, "00110011") == 0){
		decimal = "3";
	}
	else if(strcmp(buf, "00110100") == 0){
		decimal = "4";
	}
	else if(strcmp(buf, "00110101") == 0){
		decimal = "5";
	}
	else if(strcmp(buf, "00110110") == 0){
		decimal = "6";
	}
	else if(strcmp(buf, "00110111") == 0){
		decimal = "7";
	}
	else if(strcmp(buf, "00111000") == 0){
		decimal = "8";
	}
	else if(strcmp(buf, "00111001") == 0){
		decimal = "9";
	}

	return decimal;
}

void convertToTemperature()
{
  Rt = resistance;
  Rt /= 32768;
  Rt *= 430; //This is now the real resistance in Ohms

  Z1 = -RTDa;
  Z2 = RTDa * RTDa - (4 * RTDb);
  Z3 = (4 * RTDb) / 100;
  Z4 = 2 * RTDb;

  temperature = Z2 + (Z3 * Rt);
  temperature = (sqrt(temperature) + Z1) / Z4;

  if (temperature >= 0)
  {
    Serial.print("Temperature: ");
    Serial.println(temperature); //Temperature in Celsius degrees
    return; //exit
  }
  else
  {
    Rt /= 100;
    Rt *= 100; // normalize to 100 ohm

    rpoly = Rt;

    temperature = -242.02;
    temperature += 2.2228 * rpoly;
    rpoly *= Rt; // square
    temperature += 2.5859e-3 * rpoly;
    rpoly *= Rt; // ^3
    temperature -= 4.8260e-6 * rpoly;
    rpoly *= Rt; // ^4
    temperature -= 2.8183e-8 * rpoly;
    rpoly *= Rt; // ^5
    temperature += 1.5243e-10 * rpoly;

    Serial.print("Temperature: ");
    Serial.println(temperature); //Temperature in Celsius degrees
  }
  //Note: all formulas can be found in the AN-709 application note from Analog Devices
}

void readRegister()
{
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE1));
  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(0x80); //80h = 128 - config register
  SPI.transfer(0xB0); //B0h = 176 - 10110000: bias ON, 1-shot, start 1-shot, 3-wire, rest are 0
  digitalWrite(chipSelectPin, HIGH);

  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(1);
  reg1 = SPI.transfer(0xFF);
  reg2 = SPI.transfer(0xFF);
  digitalWrite(chipSelectPin, HIGH);

  fullreg = reg1; //read MSB
  fullreg <<= 8;  //Shift to the MSB part
  fullreg |= reg2; //read LSB and combine it with MSB
  fullreg >>= 1; //Shift D0 out.
  resistance = fullreg; //pass the value to the resistance variable
  //note: this is not yet the resistance of the RTD!

  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(0x80); //80h = 128
  SPI.transfer(144); //144 = 10010000
  SPI.endTransaction();
  digitalWrite(chipSelectPin, HIGH);

  Serial.print("\nResistance: ");
  Serial.println(resistance);
}

void Retrieve_Temperature_and_Print(){
  readRegister();
  convertToTemperature();
  //Serial.printf("\nTemperature: %f", temperature);
}


void setup() {
  SPI.begin();
  Serial.begin(115200, SERIAL_8N1); // Configures 1 stop bit and i think 1 start bit

  pinMode(chipSelectPin, OUTPUT); //because CS is manually switched  
}
char data_value[16] = "";
int saved_value_identity;

void loop() {
  char buffer[8];

	char* decimal_val;
	int is_decimal;
	int check_symbol;
	int pos = 0;
	int temporary_value_identity;

  Retrieve_Temperature_and_Print();

  if (Serial.available() > 0) {
    int bytesread = Serial.readBytes(buffer, 8);
  

    is_decimal = is_Decimal(buffer);
	  //check what non-decimal symbol it is
	  check_symbol = check_nondec_symbol(buffer);
	
	  temporary_value_identity = identity_byte(buffer); 

    if(temporary_value_identity != NOT_A_ID_BYTE){ //if identity byte is there
		  saved_value_identity = temporary_value_identity; //save that 
	  }

	
		
	  if(is_decimal == 1){
		  //if it's decimal then convert ascii to decimal val
		  decimal_val = ascii_to_decimal(buffer);
		  //Serial.printf("current decimal digit: %s", decimal_val);
		  strcat(data_value, decimal_val); //concatenate decimal digit to data arr

	  }
	  //if it's not decimal, then check what kind of character it is
  	else{

		  //if it's a period then we concatenate to data_value
		  if(check_symbol == PERIOD){
			  strcat(data_value, ".");
		  }

		  //if delimiter byte, then we clear data_value
		  if(check_symbol == DELIMITER){
		  	//display data_value on LCD and the cursor location will depend on saved_value_identity

		  	data_value[0] = '\0'; //clear Data Value for new income value
		  }

		  //if it's a white space or some junk value, we just do nothing
	  }
	

	
	

  
    
    buffer[8] = '\0'; 
    Serial.printf("\nbuffer: %s", buffer);
    //buffer[0] = '\0';

   
  }
  delay(1000);
  Serial.printf("\nData: %s", data_value);
  Serial.printf("\nIdentity: %d", saved_value_identity);



 
}