
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <Adafruit_MAX31865.h>
#include <MapleFreeRTOS821.h>
//#include <STM32FreeRTOS.h>
//#include <MapleFreeRTOS821.h>
#include <LiquidCrystal_I2C.h>

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

// Task parameters and UI parameters
#define TASK_DELAY pdMS_TO_TICKS(3000)  // 3 seconds
#define LAST_PAGE 3

//Task Object
TaskHandle_t xUI_Cycle;
TaskHandle_t xMainBlock;


//LCD handle
LiquidCrystal_I2C lcd(0x27,16,2);


// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31865 thermo = Adafruit_MAX31865(PA4, PA7, PA6, PA5);

// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF 430.0
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

//global variables
char data_value[16] = "";
int saved_value_identity;
float temperature;

SemaphoreHandle_t temperatureMutex;



int _next_page(int pg_num){
	if(pg_num < LAST_PAGE){
		pg_num++;
	}
	else if(pg_num == LAST_PAGE){
		pg_num = 1;
	}
	return pg_num;
}

void Page1_UI(float temperature){
  int temperature_int = (int)temperature;
  
  lcd.setCursor(0,0);
  lcd.print("Temp: "); 
  lcd.setCursor(6, 0); 
  lcd.println(temperature_int);
  lcd.setCursor(8, 0);
  lcd.print("'C");
 
  

}

void Page2_UI(){
  lcd.setCursor(0,0);
  lcd.print("Speed: ");
}

void Page3_UI(){
  lcd.setCursor(0,0);
  lcd.print("Volt: ");
}

void vRT_UI_Handler(void *pvParameters){

  int pg_num = 1;
	for(;;){
    xSemaphoreTake(temperatureMutex, portMAX_DELAY);
    float currentTemperature = temperature;
    xSemaphoreGive(temperatureMutex);

		pg_num = _next_page(pg_num); //goes to next page number
    if(pg_num == 1){
      Page1_UI(currentTemperature);
    }
    else if(pg_num == 2){
      Page2_UI();
    }
    else if(pg_num == 3){
      Page3_UI();
    }
		vTaskDelay(3000); //this delays the time for 150ms
    lcd.clear();
	}
}



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


void Temp_sensor_handler(){
  uint16_t rtd = thermo.readRTD();
  float ratio = rtd;
  ratio /= 32768;

  xSemaphoreTake(temperatureMutex, portMAX_DELAY);

  temperature = thermo.temperature(RNOMINAL, RREF);

  xSemaphoreGive(temperatureMutex);
  //delay(50);
  //Serial.printf("\nTemperature: %f", temperature); <-- this doesn't work
  Serial.print("\nTemperature = "); Serial.println(temperature);
  //byte* temperatureBytes = (byte*)&temperature; //convert temperature to bytes for serial.write()
  //for (int i = sizeof(float) - 1; i >= 0; i--) {
   // Serial.write(temperatureBytes[i]);
 // }

  return;
}


void vmainblock(void*pvParameters){
  //UNUSED(pvParameters);
  

  for(;;){
    char buffer[8];

	  char* decimal_val;
	  int is_decimal;
	  int check_symbol;
	  int pos = 0;
	  int temporary_value_identity;

    Temp_sensor_handler();
    //Serial.print("\nTest");
  

    if (Serial.available() > 0) {
      int bytesread = Serial.readBytes(buffer, 8);
      buffer[bytesread] = '\0'; // Add null character at the end of the buffer
  

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
      Serial.print("\nbuffer: "); Serial.println(buffer);
            //buffer[0] = '\0';
      
     
   
    }
    vTaskDelay(1000); //this delays the time for 150ms
    Serial.print("\nData: "); Serial.println(data_value);
    Serial.print("\nIdentity: "); Serial.println(saved_value_identity);
    

  }
}

void FreeRTOS_Tasks_Handler(){
  xTaskCreate(vRT_UI_Handler, "RT_UI_Handler", configMINIMAL_STACK_SIZE * 5, NULL , tskIDLE_PRIORITY + 2, &xUI_Cycle); 
  xTaskCreate(vmainblock, "vmainblock", configMINIMAL_STACK_SIZE * 6, NULL, tskIDLE_PRIORITY + 3, &xMainBlock); 
  
  
  

    // start FreeRTOS
  vTaskStartScheduler();
}


void setup() {
  //LCD SETUP
  lcd.begin();
  lcd.clear();         
  lcd.backlight();

  // Initialize the mutex
  temperatureMutex = xSemaphoreCreateMutex();
  
  //Main SETUP
  Serial.begin(115200, SERIAL_8N1); // Configures 1 stop bit and i think 1 start bit
  int page_number = 1; //we want to start on the 1st page on UI
  thermo.begin(MAX31865_4WIRE);  // set to 2WIRE or 4WIRE as necessary
  FreeRTOS_Tasks_Handler();
  
}


void loop() {
  while(1) {
  }
}

