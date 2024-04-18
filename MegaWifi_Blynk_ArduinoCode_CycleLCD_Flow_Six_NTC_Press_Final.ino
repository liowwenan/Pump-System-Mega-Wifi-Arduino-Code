#include <U8g2lib.h>
#include <U8x8lib.h>

#define ntc_pin_1 A11
#define ntc_pin_2 A10
#define ntc_pin_3 A9
#define ntc_pin_4 A8
#define ntc_pin_5 A7
#define ntc_pin_6 A6
#define pressure_pin_1 A0
#define pressure_pin_2 A3 //be advise that the pin/ connection is inversed
#define pressure_pin_3 A2
#define pressure_pin_4 A1
#define pressure_pin_5 A4
#define pressure_pin_6 A5
#define vd_power_Tpin 6
#define vd_power_Ppin 5
#define buttonPinT 3
#define buttonPinP 4
#define nominal_resistance 10000
#define nominal_temperature 25
#define sampling_rate 5
#define beta 3950
#define Rref 10000

float flow_frequency; // Measures flow sensor pulses
unsigned int l_min; // Calculated litres/min
unsigned char flowsensor = 2; // Sensor Input

const int pressureZero[] = {
  (0.45/5*1024),  // Pressure sensor 1
  (0.41/5*1024),  // Pressure sensor 2
  (0.46/5*1024),  // Pressure sensor 3
  (0.44/5*1024),  // Pressure sensor 4
  (0.445/5*1024),  // Pressure sensor 5
  (0.445/5*1024)   // Pressure sensor 6
}; //Mega Pascal value of transducer being used
const int pressureMax = (4.5/5*1024); //analog reading of pressure transducer at 300psi
const float pressuretransducermaxMPa[] = {
  206.8427188,  // Pressure sensor 1
  206.8427188,  // Pressure sensor 2
  206.8427188,  // Pressure sensor 3
  206.8427188,  // Pressure sensor 4
  2068.427188,  // Pressure sensor 5
  2068.427188   // Pressure sensor 6 206.8427188
}; //Mega Pascal value of transducer being used
const int baudRate = 9600; //constant integer to set the baud rate for serial monitor
const int sensorreadDelay = 250; //constant integer to set the sensor read delay in milliseconds

float pressureValue = 0; //variable to store the value coming from the pressure transducer

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8);
int currentSensorT = 1, currentSensorP = 1; // Start with the first sensor

unsigned long previousMillis = 0;  // will store last time serial3 was updated
const long interval = 7000;  // interval for send data to serial3 (7 seconds)

void flow () // Interrupt function
{
   flow_frequency++;
}

void setup() {
  pinMode(vd_power_Tpin, OUTPUT);
  pinMode(vd_power_Ppin, OUTPUT);
  pinMode(buttonPinT, INPUT_PULLUP);
  pinMode(buttonPinP, INPUT_PULLUP);

  pinMode(flowsensor, INPUT);
  digitalWrite(flowsensor, HIGH); // Optional Internal Pull-Up
  attachInterrupt(0, flow, RISING); // Setup Interrupt
  sei(); // Enable interrupts

  Serial.begin(9600);
  Serial3.begin(9600);

  Serial.println("CLEARDATA");
  Serial.println("LABEL, Time, T1(°C), T2(°C), T3(°C), T4(°C), T5(°C), T6(°C), P1(kPa), P2(kPa), P3(kPa), P4(kPa), P5(kPa), P6(kPa), Flow Rate(LPM)");
  Serial.println("RESETTIMER");

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_helvB10_tf); 
  u8g2.setColorIndex(1);  
}

void loop() {
  digitalWrite(vd_power_Tpin, HIGH);
  digitalWrite(vd_power_Ppin, HIGH);

//PLX-Excel file print out in Excel & Serial Monitor
  Serial.print("DATA,TIME");

  // Read all sensor temperatures
  float temperatures[6];
  for (int Tsensor = 1; Tsensor <= 6; Tsensor++) {
    temperatures[Tsensor - 1] = readTemperature(Tsensor);
    //PLX Print
    Serial.print(",");
    Serial.print(temperatures[Tsensor - 1], 2); //Print out in PlX-Excel and Serial Monitor
  }

  // Read all sensor pressure
  float pressure[6];
  for (int Psensor = 1; Psensor <= 6; Psensor++) {
    pressure[Psensor - 1] = readPressure(Psensor);
    //PLX Print
    Serial.print(",");
    Serial.print(pressure[Psensor - 1], 2); //Print out in PlX-Excel and Serial Monitor
  }

 // Read flow sensor
 // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
 float l_min = float(flow_frequency / 4.8); // (Pulse frequency) / 4.8Q = flowrate in L/min, if liter per hour (flow_frequency * 60 / 7.5)
 flow_frequency = 0; // Reset Counter

  // Send all sensor readings to Serial3 every 7 seconds
  unsigned long currentMillis = millis();
   if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;  // reset the interval
  String send = "";
  for (int i = 0; i < 6; i++) {
    send += String(temperatures[i], 2) + ";";
  }
  send += "";
  for (int i = 0; i < 6; i++) {
    send += String(pressure[i], 2) + ";";
  }
  send += "";
  send += l_min;
  send.remove(send.length() - 2); // Remove the trailing "; " from the end
  Serial3.println(send);
   }
    /*if(Serial3.available()){
    String msg = "";
    while(Serial3.available()){
      msg += char(Serial3.read());
      delay(50);
    }
    Serial.println(msg);
  }*/

  // Display the temperature of the current sensor on Serial
  float currentTemp = temperatures[currentSensorT - 1];
  displayCurrentSensorTemperature(currentSensorT, currentTemp);

  // Display the pressure of the current sensor on Serial
  float currentPress = pressure[currentSensorP - 1];
  displayCurrentSensorPressure(currentSensorP, currentPress);

//Display flow rate on serial monitor
  //Serial.print(l_min, 3); // Print litre/minute
  //Serial.println(" L/min");

  // Check for button press to cycle to the next sensor
  if (digitalRead(buttonPinT) == LOW) {
    currentSensorT = (currentSensorT % 6) + 1; // Cycle to the next sensor
    delay(100);  // Debounce delay
  }

  // Check for button press to cycle to the next sensor
  if (digitalRead(buttonPinP) == LOW) {
    currentSensorP = (currentSensorP % 6) + 1; // Cycle to the next sensor
    delay(100);  // Debounce delay
  }

  Serial.print(",");
  Serial.println(l_min, 3); //Print out in PlX-Excel and Serial Monitor

  delay(500);

  u8g2.firstPage();
  do {   
    draw(currentSensorT, currentTemp, currentSensorP, currentPress, l_min);
  } while( u8g2.nextPage() );
}

float readTemperature(int Tsensor) {
  int Tpin;
  switch (Tsensor) {
    case 1: Tpin = ntc_pin_1; break;
    case 2: Tpin = ntc_pin_2; break;
    case 3: Tpin = ntc_pin_3; break;
    case 4: Tpin = ntc_pin_4; break;
    case 5: Tpin = ntc_pin_5; break;
    case 6: Tpin = ntc_pin_6; break;
    default: return 0.0;  // Should not happen
  }

  int samples = 0;
  for (uint8_t i = 0; i < sampling_rate; i++) {
    samples += analogRead(Tpin);
    delay(10);
  }

  float average = samples / static_cast<float>(sampling_rate);
  float thermistorResistance = Rref / (1023 / average - 1);
  return 1.0 / ((log(thermistorResistance / nominal_resistance) / beta) + 1.0 / (nominal_temperature + 273.15)) - 273.15;

  digitalWrite(vd_power_Tpin, LOW);
}

float readPressure(int Psensor) {
  int Ppin;
  switch (Psensor) {
    case 1: Ppin = pressure_pin_1; break;
    case 2: Ppin = pressure_pin_2; break;
    case 3: Ppin = pressure_pin_3; break;
    case 4: Ppin = pressure_pin_4; break;
    case 5: Ppin = pressure_pin_5; break;
    case 6: Ppin = pressure_pin_6; break;
    default: return 0.0;  // Should not happen
  }

  int samples = 0;
  for (uint8_t i = 0; i < sampling_rate; i++) {
    samples += analogRead(Ppin);
    delay(10);
  }

  pressureValue = analogRead(Ppin); //reads value from input pin and assigns to variable
  pressureValue = ((pressureValue-(pressureZero[Psensor - 1]))*pressuretransducermaxMPa[Psensor - 1])/(pressureMax-(pressureZero[Psensor - 1])); //conversion equation to convert analog reading to psi
  return pressureValue;
  delay(250);

  digitalWrite(vd_power_Ppin, LOW);
}

void displayCurrentSensorTemperature(int Tsensor, float temp) {
  //Serial.print("Temperature " + String(Tsensor) + ": " + String(temp, 2) + " °C  ");
}

void displayCurrentSensorPressure(int Psensor, float press) {
  //Serial.print("Pressure " + String(Psensor) + ": " + String(press, 2) + " kPa  ");
}

void draw(int Tsensor, float temp, int Psensor, float press, float l_min) {
    u8g2.drawFrame(0, 0, 128, 31);  
    u8g2.drawFrame(0,33,128,31);                  

    u8g2.drawStr(15, 13, ("T " + String(Tsensor)).c_str());  // Convert String to const char*
    u8g2.drawStr(50, 13, String(temp, 2).c_str());                     // Convert String to const char*
    u8g2.drawUTF8(90, 13, " °C");

    u8g2.drawStr(15, 28, ("P " + String(Psensor)).c_str());  // Convert String to const char*
    u8g2.drawStr(50, 28, String(press, 2).c_str());                     // Convert String to const char*
    u8g2.drawUTF8(90, 28, " kPa");

    u8g2.drawUTF8(5, 46, ("Flow "));  // Convert String to const char*
    u8g2.drawStr(45, 46, String(l_min, 3).c_str());                     // Convert String to const char*
    u8g2.drawUTF8(90, 46, " LPM");

    u8g2.drawUTF8(0, 60, " PumpSystem G12");
}