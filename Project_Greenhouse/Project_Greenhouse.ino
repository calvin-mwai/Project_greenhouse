/**
 * This is my personal project meant to automate a greenhouse
 * It includes several functions:
 * 1.Temperature and Humidity monitoring  ^done
 * 2.Soil moisture monitoring
 * 3.Remote control using GSM module
 * 4.On-site LCD menu                     ^done
 */
//Libraries to be used in the project

#include <Wire.h>                 // To enable the I2C communication in the system
#include <LiquidCrystal_I2C.h>    // To use the I2C communication in the LCD module t oreduce the number of pins needed
#include "DHT.h"                  // To use the DHT sensor to measure both temperature and humidity
#include <DS3231.h>               // To use a real time clock(RTC) in the timing of the system

//GLOBAL VARIABLES AND PINS

//                      LCD module

LiquidCrystal_I2C lcd(0x3F,16,2);  // To set the address of the I2C LCD module (0x20 - Proteus simulation, 0x3F- Real life)
bool backLightState = true;

//                      DHT module

#define DHTPIN 11                 // DHT11 is connected to pin11 of Arduino
#define DHTTYPE DHT11             // DHT11 is being used

DHT dht(DHTPIN, DHTTYPE);         // Initialization of the DHT library giving the pin and type of DHT sensor

int Temp;                     // Value for temperature
int RH;                       // Value for relative humidity
/*
 * Note: Not yet decided whether to have two DHT sensors; internal and external
 * Therefore both Temp & RH are for internal Temp and Humidity respectively
 */
const uint8_t maxTemp = 25;
const uint8_t maxRH = 50;

//                      Menu

//      Setting up buttons
const int numOfInputs = 3;        // Number of buttons in the build
const int inputPins[numOfInputs] = {8,9,10}; // The pins of the inputs -> 8 = up, 9 = down, 10 = lcd on/off
int inputState[numOfInputs];
int lastInputState[numOfInputs]= {LOW,LOW,LOW};
bool inputFlags[numOfInputs] = {LOW,LOW,LOW};
unsigned long lastDebounceTime[numOfInputs] = {0,0,0};
unsigned long debounceDelay = 50;

//      Menu logic
const int numOfScreens = 4;       // Temperature, humidity, soil moisture, time
int currentScreen = 0;
String screens[numOfScreens][2] = {{"Temperature","degC"},{"Humidity","%"},{"Soil Moisture","%"},{"Time"," "}};
int parameters[numOfScreens];

//                    Actuators

//      Fans
#define FanRelay 2                // Fan relay connected to pin2 
#define ExhaustFanRelay 4         // Exhaust fan relay connected to pin4


//                    Clock

DS3231  rtc(SDA, SCL);            // Initialization of the DS3231 rtc module
String tm;                        // Variable to store the current time

void setup() {
 for(int i = 0; i < numOfInputs; i++){ 
    pinMode(inputPins[i], INPUT); // Set all the button pins as inputs
    digitalWrite(inputPins[i], HIGH); // Pull-up all the input pins using the 20k internal pull-up resistor 
    /*
     * https://www.arduino.cc/en/Tutorial/DigitalPins for information on this
     */
    }
    
  lcd.init();                     // Initialize the LCD
  //lcd.backlight();              // Turn on the LCD backlight       Was commented out to achieve on/off button for lcd backlight
  
  dht.begin();                    // Initialize the dht sensor
  
  rtc.begin();                    // Initialize the rtc
  Serial.begin(9600);             // For debugging
  
  pinMode(FanRelay,OUTPUT);       
  pinMode(PumpRelay,OUTPUT);
  pinMode(ExhaustFanRelay,OUTPUT);

//  // Setting the date and time 
//  rtc.setDOW(MONDAY);           // Setting the day of the week
//  rtc.setTime(18, 39, 0);       // (HH, MM, SS)
//  rtc.setDate(14, 9, 2020);     // (DD, MM, YYYY)

}//End of setup()

void loop() {
  readDHT();                      // Function to read the Temp and RH values from the DHT11 module connected on pin11, and store them in parameters[0,1]
  setInputFlags();                // Function to debounce the buttons and also set the input flags
  resolveInputFlags();            // Function to perform actions depending on the button pressed
  printScreen();
  fans();
  readRTC();                      // Function to read the rtc time and date
}//End of loop()

/*___________________________________________________________________________________________________________________________________________
 *                   Functions in the system
 */
//                   Set input flags and debounce the buttons

void setInputFlags() {
  for(int i = 0; i < numOfInputs; i++) { 
    int reading = digitalRead(inputPins[i]); 
    if (reading != lastInputState[i]) { 
      lastDebounceTime[i] = millis(); } 
      if ((millis() - lastDebounceTime[i]) > debounceDelay) {
        if (reading != inputState[i]) {
          inputState[i] = reading;
            if (inputState[i] == HIGH) {
              inputFlags[i] = HIGH;}
         }
      }
     lastInputState[i] = reading;
   }
   
}//End of setInputFlags()

//                    Resolve input flags 

void resolveInputFlags() {
  for(int i = 0; i < numOfInputs; i++) { 
    if(inputFlags[i] == HIGH) { 
      inputAction(i); 
      inputFlags[i] = LOW; 
      } 
  } 
}

//                    Perform input actions
/*
 * From the Arduino Bootloader source, 
 * "A bunch of if...else if... gives smaller code than switch... case..."
 */
void inputAction(int input) {     // ALL BUTTON FUNCTIONS SHOULD BE ADDED TO THIS FUNCTION.
  if(input == 0) {                // First button -> Up function
    lcd.clear();
    if (currentScreen == 0) {
      currentScreen = numOfScreens-1;}
      else{
        currentScreen--;}
  }
  else if(input == 1) {           // Second button -> Down function
    lcd.clear();
    if (currentScreen == numOfScreens-1) {
      currentScreen = 0;}
      else{
        currentScreen++;}
  }
  else if(input == 2) {           // Third button -> lcd on/off
    lcd.clear();
    lcd_State();                  // Function to toggle the state of the LCD if the button is pressed
    }
//else if(input == 3) {
//  parameterChange(0);}          // Incase we need to change some parameters in the system
/*
 * Parameter change not implemented in the system, but there is a chance to here
 */
}//End of inputAction()


//                  Perform changes in the parameters being monitored

//void parameterChange(int key) { // Function to enable parameter change
//  if(key == 0) {
//    parameters[currentScreen]++;} // maxTemp
//  else if(key == 1) {
//    parameters[currentScreen]--;} // maxRH
//}//End of parameterChange()

//                  Reading and storing the current time from RTC

void readRTC(){
  tm = rtc.getTimeStr();          // Store the current time in variable tm
  screens[3][1] = tm;             // Store variable tm in the screens list
}// End of readRTC()

//                  Reading and storing current values of temperature and humidity

void readDHT() {
  RH = dht.readHumidity();        // Read relative humidity in %
  Temp = dht.readTemperature();   // Read temp in degC

  // Check if any reads failed and exit early to try again
  if (isnan(RH) || isnan(Temp)){
    lcd.clear();
    lcd.setCursor(5,0);
    lcd.print("Error");
    return;
  }
  parameters[0] = dht.readTemperature();
  parameters[1] = dht.readHumidity();
}//End of readDHT()

//                  Toggle the state of the LCD if the button is pressed

void lcd_State(){
  if (backlightState == true) {   //check to see state of backlight
    backlightState = false;       //flip state of backlight
    }
  else if (backlightState == false) {
    backlightState = true;
    }
    
  if ( backlightState == true) {  //the bool argument for backlight state (true/false)
    lcd.setBacklight(HIGH);       //the statement that actualy changes the backlight state
    }
  else if ( backlightState == false) {
    lcd.setBacklight(LOW);
    }
}//End of lcd_state()

//                Display the menu and parameters in the LCD screen

void printScreen() {
  lcd.print(screens[currentScreen][0]);
  lcd.setCursor(0,1);
  if (currentScreen == 3){        // The screen containing Time
    lcd.print(screens[currentScreen][1]);
    lcd.print("         ");
  }
  else{
    lcd.print(parameters[currentScreen]);
    lcd.print(" ");
    lcd.print(screens[currentScreen][1]);
    lcd.print("                ");
  }
}//End of printScreen()

//               Actuate the fan relays

void fans() {
  if(parameters[0] > maxTemp || parameters[1] > maxRH){
    digitalWrite(FanRelay, HIGH);
    digitalWrite(ExhaustFanRelay,HIGH);
      }
  else{
    digitalWrite(FanRelay, LOW);
    digitalWrite(ExhaustFanRelay,LOW);
  }
}//End of fans()



























  
