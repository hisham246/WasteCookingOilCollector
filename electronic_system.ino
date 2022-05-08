#include <SPI.h>
#include <RFID.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins
/*
  RFID MODULE        MEGA
  SDA                  D48
  SCK                  D52
  MOSI                 D51
  MISO                 D50
  IRQ                  N/A
  GND                  GND
  RST                  D49
  3.3V                 3.3V
*/

#define SDA_DIO 48
#define RESET_DIO 49
#define SERVOPIN 12
#define enA  30
#define enB  35
#define in1  32
#define in2  34
#define in3  33
#define in4  31
#define FlowSensor 19
#define LevelSensor 18
#define Relay_Pin 45


LiquidCrystal_I2C lcd(0x27, 20, 4);
// set the LCD address to 0x27 for a 16 chars and 2 line display

Servo doorservo; // change across program

RFID RC522(SDA_DIO, RESET_DIO);

float flow = 0;
float flow_rate = 0; //Liters of passing water volume

unsigned long pulse_freq;
unsigned long currentTime;
unsigned long lastTime;


int Pump_On = 0;
unsigned long currentTime2 = 0; 
unsigned long lastTime2 = 0;
unsigned long Stop_Pump_Timer = 300000; // 30 seconds
unsigned long Time_Storage = 0;
 
int Liquid_level = 0;
int MaxSpeedDoor = 250;
int MinSpeedDoor = 200;
int MaxSpeedFilter = 170;
int FilterMotorSpeed = 0;
int StartingFilterMotorSpeed = 100;

String USERNAME[] = {"TAG", "CARD", ""};
String USERID[] = {"14022424147178", "24910125211014", ""};
bool Status = 0;
String Temporary = "";

int unlockposition = 0;
int lockposition = 180; // or 90
int val = 0;

// Setups
void RFID_setup()
{
  SPI.begin();
  RC522.init();
  delay(10);
}

void LCD_setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();      // Make sure backlight is on
  delay(5);
}

void Servo_setup()
{
  doorservo.attach(SERVOPIN);
  doorservo.write(lockposition);
  delay(10);
}

void DCMotor1_setup()
{
  pinMode(enA, OUTPUT); //G1 dc motor
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void DCMotor2_setup()
{
  pinMode(enB, OUTPUT); //G2 dc motor
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  delay(5);
}

void FlowSensor_setup()
{
  pinMode(FlowSensor, INPUT);
  attachInterrupt(4, Pulse, RISING);
  currentTime = millis();
  lastTime = currentTime;
  delay(5);
}

void LevelSensor_setup()
{
  pinMode(LevelSensor, INPUT);
  // attachInterrupt(5, Level_Sensor, RISING);
  delay(5);
}

void Pump_setup()
{
  pinMode(Relay_Pin, OUTPUT);
  digitalWrite(Relay_Pin, HIGH);
  delay(10);
}

void Arduino_setup()
{
  Serial.begin(9600);
  delay(5);
}

// Arduino Setup
void setup()
{
  Arduino_setup();
  Servo_setup();
  LCD_setup();
  RFID_setup();
  FlowSensor_setup();
  LevelSensor_setup();
  DCMotor1_setup();//motor 1
  DCMotor2_setup();
  Pump_setup();
  PrintLCD();
}

void loop()
{
  Monozukuri_Function();
}

void Monozukuri_Function()
{
  PrintLCD();
  /* Has a card been detected? */
  if (RC522.isCard() == false && Status == 0)
  {
    Status = 1;
    Serial.print("No card detected \n");
    PrintLCD();
  }

  else if (Status == 1)
  {
    PrintLCD();
    ReadCardID();
    PrintLCD();
    CompareID();
    PrintLCD();
    Status = 2;
    PrintLCD();
    delay(50);
    PrintLCD();
  }
  else if (Status == 2)
  {
    PrintLCD();
    Status = 0;
    PrintLCD();
    delay(100);
    PrintLCD();
  }

  else
  {
    Status = 0;
    PrintLCD();
  }
  PrintLCD();
}

void PrintLCD()
{
  UpdateTime();
  lcd.setCursor(1, 0);
  lcd.print("Scan your card.");
  lcd.setCursor(1, 1);
  flow = .0004678 * pulse_freq;
  
  lcd.print(flow, 2);
  lcd.print(" Liters");
  Liquid_level = digitalRead(LevelSensor);
  lcd.setCursor(1, 2);
  Liquid_level = digitalRead(LevelSensor);
  if (Liquid_level < 0.4)
  {
    lcd.setCursor(1, 2);
    lcd.print("Tank is not full.");
    Liquid_level = digitalRead(LevelSensor);
  }
  else if (Liquid_level >= 0.5)
  {
    lcd.setCursor(1, 2);
    lcd.print("TANK IS FULL NOW.");
    //    lcd.setCursor(1, 3);
    //    lcd.print("Stop pouring!");
    UpdateTime();
    return;
  }
}

void Pulse() // Interrupt function
{
  pulse_freq++;
}

void ReadCardID()

{ /* If so then get its serial number */
  PrintLCD();
  Temporary = "";
  RC522.readCardSerial();
  //  Serial.println("Card detected:");
  for (int i = 0; i < 5; i++)
  {
    Temporary = Temporary + RC522.serNum[i];
  }
  PrintLCD();
}

void CompareID()
{
  PrintLCD();
  if  (val != 1 && (Temporary.equals(USERID[0]) || Temporary.equals(USERID[1])))
  {
    val = 0;
    Serial.print("Recorded User: ");
    Serial.println(USERNAME[0]);
    doorservo.write(unlockposition);
    PrintLCD();
    OpenDoor();
    PrintLCD();
    delay(10);
    PrintLCD();
    StartPump();
    StartFilter();
    PrintLCD();

    PrintLCD();
    delay(10);
    while (RC522.isCard() == false || val == 0)
    {
      PrintLCD();
      delay(10);
      val = 1;
    }
  }

  else if (val == 1 && (Temporary.equals(USERID[0]) || Temporary.equals(USERID[1])))
  {
    PrintLCD();
    Serial.print("Recorded User: ");
    Serial.println(USERNAME[0]);
    CloseDoor();
    delay(30000);
    StopFilter();
    delay(5000);
    StopPump();
    delay(1000);
    doorservo.write(lockposition);
    ClearLCD();
    val = 0;

    delay(500);
    ClearLCD();
  }
}

void OpenDoor()
{
  PrintLCD();
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  for (int i = MinSpeedDoor; i <= MaxSpeedDoor; i++)
  {
    PrintLCD();
    analogWrite(enB, i);
    delay(1);
  }

  delay(29000); // Calculate for door opening

  for (int i = MaxSpeedDoor ; i >= MinSpeedDoor; i--)
  {
    PrintLCD();
    analogWrite(enB, i);
    delay(1);
  }
  PrintLCD();
  //Now turn off motors
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  PrintLCD();
}

void CloseDoor()
{
  // Now change motor directions
  PrintLCD();
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
  PrintLCD();
  for (int i = MinSpeedDoor; i <= MaxSpeedDoor; i += 7)
  {
    PrintLCD();
    analogWrite(enB, i);
    delay(25);
  }

  delay(34000);
  for (int i = MaxSpeedDoor; i >= MinSpeedDoor; i -= 7)
  {
    PrintLCD();
    analogWrite(enB, i);
    delay(25);
  }

  /* Commenting this for later!!!!!!





  
  stopmotor();
  */

  
  PrintLCD();
  // Now turn off motors
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB,0);
  PrintLCD();
  delay(10);
  PrintLCD();
}


void StartFilter()
{
  PrintLCD();
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  PrintLCD();

  //Define in3 and in4 enB
  // Accelerate from zero to maximum speed
  PrintLCD();
  for (FilterMotorSpeed = StartingFilterMotorSpeed; FilterMotorSpeed < MaxSpeedFilter; FilterMotorSpeed += 7)
  {
    PrintLCD();
    analogWrite(enA, FilterMotorSpeed);
    delay(15);
  }
  PrintLCD();
}

void StopFilter()
{
  ClearLCD();
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA,0);
  ClearLCD();
  delay(15);
}

void StartPump()
{
  digitalWrite(Relay_Pin, LOW); // Turning the Relay pin LOW turns the relay on
  delay(50);
  Pump_On = 1; 
  lastTime2 = millis(); 
  Time_Storage = lastTime2; 
}

void StopPump()
{
  ClearLCD();
  digitalWrite(Relay_Pin, HIGH); // Turning the Relay pin HIGH turns the relay off
  delay(50);
  ClearLCD();
}

void UpdateTime()
{
  currentTime2 = millis(); 
  lastTime2 = Time_Storage;
  if((currentTime2 - lastTime2 >= Stop_Pump_Timer) && Pump_On == 1)
  {
    StopPump();
    ClearLCD();
    StopFilter();
    ClearLCD();
    Pump_On = 0;
    lastTime2 = currentTime2;
    Time_Storage = lastTime2;
    delay(5000);
    ClearLCD();
  }

  else{}
}

void ClearLCD()
{
  lcd.setCursor(1, 0);
  lcd.print("Scan your card.");
  lcd.setCursor(1, 1);
 
  flow = 0;
  lcd.print(flow, 2);
  lcd.print(" Liters");
  
  Liquid_level = digitalRead(LevelSensor);
  lcd.setCursor(1, 2);
  
  if (Liquid_level < 0.4)
  {
    lcd.setCursor(1, 2);
    lcd.print("Tank is not full.");
    Liquid_level = digitalRead(LevelSensor);
  }
  else if (Liquid_level >= 0.5)
  {
    lcd.setCursor(1, 2);
    lcd.print("TANK IS FULL NOW.");
    UpdateTime();
    return;
  }
}

void stopmotor()
{
  delay(5000);
  digitalWrite(in1,LOW);
  digitalWrite(in2,LOW);
  analogWrite(enA,0);
}
