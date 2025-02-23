#include <NewPing.h>
#include <WiFiS3.h>

char ssid[] = "Y11 SAMM"; 
char pass[] = "12345678"; 
WiFiServer server(5200); 
WiFiClient client;

//For wheel encoders
volatile unsigned long Time_ENC_A = 0;
volatile unsigned long Time_ENC_B = 0;
const float pi = 3.14159265;
volatile unsigned long pulseCount_ENC_A = 0;
volatile unsigned long pulseCount_ENC_B = 0;
volatile unsigned long previous_time_A = 0;
volatile unsigned long previous_time_B = 0;
const int Pulse_pre_rev = 4;
//wheel circumference
const float Circumference = 6.2*pi/100.0; 

const int LEYE = 4;   //Eye = IR Sensors
const int REYE = 12;
const int speed = 115;
const int US_TRIG = 9;  //Sends the Ultrasonic Pulse
const int US_ECHO = 8;  //Receives the Ultrasonic Pulse

bool obstacle_detected = false;
bool BuggyActive = false;

//Motor A pins
const int ENA = 5;    
const int IN1 = 16;
const int IN2 = 15;

//Motor B pins
const int ENB = 11;
const int IN3 = 18;
const int IN4 = 17;

//Encoder pins
const int ENC_A = 2;
const int ENC_B = 3;

int LEYE_current_state;
int REYE_current_state;

unsigned long starting;
unsigned long ending;
int distance_maxxing = 50;
//int IncrSpeed = speed + Output;

//NewPing library helps remove unnecessary delays for US Sensor
NewPing UltraSonic(US_TRIG, US_ECHO, distance_maxxing);

float obst_distance = UltraSonic.ping_cm(); //current distance in cm from US Sensor
volatile float distance_travelled = 0.0;
float avg_time = 0.0;
float avg_speed = 0.0;
float avg_distance_travelled = 0.0;

void setup() {
  Serial.begin(115200);
  WiFi.beginAP(ssid, pass); 
  Serial.print("Connecting to WiFi...");
  while(WiFi.status() != WL_AP_CONNECTED){
    Serial.print(".");
    delay(1500);
  }
  Serial.println("Connected!");
  Serial.print("IP Address:");   
  Serial.println(WiFi.localIP()); 
  server.begin();

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(LEYE, INPUT);
  pinMode(REYE, INPUT);
  pinMode(US_TRIG, OUTPUT);
  pinMode(US_ECHO, INPUT);
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_A), time_monitor_A, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B), time_monitor_B, RISING);
}

void loop() {
  starting = micros();
  client = server.available();
  if (client.connected()){
    Serial.println("Client is still Online");
    if(client.available()) {
      String RequestFromClient = client.readStringUntil('\n');  // Read client request
      if (RequestFromClient == "Proceed lil bro")
        Proceed();

      else if (RequestFromClient == "Halt lil bro") 
        Halt();
      else if (RequestFromClient == "Update lil bro"){
      }
    }
    Serial.println("AVG Speed: " + String(avg_speed) + "," + "AVG Distance: " + String(avg_distance_travelled) + "\n");
  }
  Calc_avg_DST();
  if (client.connected()){
    client.print(String(avg_speed, 2) + "," + String(avg_distance_travelled, 2) + "\n");
}
  if(BuggyActive){
    ActivateBuggy();
  }
}

//functions for moving help with code redability
void moveForward(){
  Serial.println("moving forward");
  analogWrite(ENA, speed);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  analogWrite(ENB, speed);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW);
}

void turnLeft(){
  Serial.println("turning left");
  analogWrite(ENA, 0);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  analogWrite(ENB, speed+40);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW); 
}

void turnRight(){
  Serial.println("turning right");
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed+40);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
}

void stop(){
  Serial.println("stopping");
  analogWrite(ENA, 0);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, LOW);
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void time_monitor_A(){
  unsigned long current_time = micros();
  if(previous_time_A > 0){
    Time_ENC_A = current_time - previous_time_A;
  }
  previous_time_A = current_time;
  pulseCount_ENC_A++;
 
}

void time_monitor_B(){
  unsigned long current_time = micros();
  if(previous_time_B > 0){
    Time_ENC_B = current_time - previous_time_B;
  }
  previous_time_B = current_time;
  pulseCount_ENC_B++;
}


void Calc_avg_DST() {
  unsigned long total_pulses = pulseCount_ENC_A + pulseCount_ENC_B;
  Serial.print("Pulse Count a: ");
  Serial.println(pulseCount_ENC_A);
  Serial.print("Pulse Count b: ");
  Serial.println(pulseCount_ENC_B);
  Serial.print("Total pulses: ");
  Serial.println(total_pulses);
  avg_distance_travelled = Circumference*total_pulses/((float)(Pulse_pre_rev * 2));
  
  // Calculate time in seconds
  avg_time = (Time_ENC_A + Time_ENC_B)/2000000.0;
  if(avg_time > 0.0001) { // Avoid division by very small numbers
    avg_speed = Circumference/(Pulse_pre_rev*avg_time);
  } else {
      avg_speed = 0;
  }
        
  
}

void obstacle_detection(float distance){
  if(distance > 0){
    if(distance < 15 && !obstacle_detected){
      Serial.println("Holy Crap I'm about to hit something");
      client.print("Bout to hit something\n");
      obstacle_detected = true;
    }
    else if(distance > 15 && obstacle_detected){
      obstacle_detected = false;
      client.print("obstacle_cleared\n");
    }
  }
  else obstacle_detected = false;
}

void Proceed(){
  BuggyActive = true;
}

void Halt(){
  stop();
  BuggyActive = false;
  avg_speed = 0;
  Time_ENC_A = 0;
  Time_ENC_B = 0;
}

void ActivateBuggy(){
  LEYE_current_state = digitalRead(LEYE);
  REYE_current_state = digitalRead(REYE);

  //Adjust speed when turning according to PID evaluated Output
  obst_distance = UltraSonic.ping_cm(); //current distance in cm from US Sensor



  if(obst_distance > 0 ){
    Serial.print("Distance: ");
    Serial.println(obst_distance); 
  }

  obstacle_detection(obst_distance);
  if(obstacle_detected){
    stop();
  }
  else if(LEYE_current_state && REYE_current_state){ 
    moveForward();
  }
  else if(!LEYE_current_state && REYE_current_state){
    turnLeft();

  }
  else if(!REYE_current_state && LEYE_current_state){
    turnRight();

  }
  else if (!LEYE_current_state && !REYE_current_state){
    stop();  
  }
  ending = micros();
  Serial.println("loop " + String(ending - starting));
}