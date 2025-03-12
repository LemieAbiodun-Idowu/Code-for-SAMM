#include <NewPing.h>    //used for US Sensor
#include <WiFiS3.h>     //Library for wifi
#include <PID_v1.h>

char ssid[] = "Y11 SAMM"; 
char pass[] = "12345678"; 
//our arduino is a Wireless Acess Point, it basically acts as its own wifi
WiFiServer server(5200);  //the port number used
WiFiClient client;

//For wheel encoders
//volatile ensures the values are the same when accessed outside the isr functions
//use unsigned long because the values can get very big
volatile unsigned long Time_ENC_A = 0;
volatile unsigned long Time_ENC_B = 0;
const float pi = 3.14159265;
volatile unsigned long pulseCount_ENC_A = 0;
volatile unsigned long pulseCount_ENC_B = 0;
volatile unsigned long previous_time_A = 0;
volatile unsigned long previous_time_B = 0;
//the wheels have 4 pulses per revolution
const int Pulse_pre_rev = 4;
//wheel circumference
const float Circumference = 6.2*pi/100.0; // divided by 100 for metres conversion .0 is used for float division

//PID Implementation for smooth turns - Needs Fine tuning
double Input, Setpoint, Output;
//double Kp = 35, Ki =1, Kd = 10;
double Kp = 25, Ki = 0.5, Kd = 12;

PID MBS(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

//PID Implementation for Mode 1
double Input1, Setpoint1, Output1;
double Kp1 = 0.0, Ki1 = 0.0, Kd1 = 0.0;

PID Mode1(&Input1, &Output1, &Setpoint1, Kp1, Ki1, Kd1, DIRECT);

//PID Implementation for Mode 2
double Input2, Setpoint2, Output2;
double Kp2 = 2.0, Ki2 = 0.5, Kd2 = 1.0;

PID Mode2(&Input2, &Output2, &Setpoint2, Kp2, Ki2, Kd2, DIRECT);

//const values are NEVER changed
const int LEYE = 4;   //Eye = IR Sensors
const int REYE = 12;
int speed = 0;
const int US_TRIG = 9;  //Sends the Ultrasonic Pulse
const int US_ECHO = 8;  //Receives the Ultrasonic Pulse

bool obstacle_detected = false;
bool info_sent = false;
bool BuggyActive = false;

//Motor A pins
const int ENA = 5;    //EN = enable pins for the motor
const int IN1 = 16;
const int IN2 = 15;

//Motor B pins
const int ENB = 11;
const int IN3 = 18;
const int IN4 = 17;

//Encoder pins, the wheel encoders help determine the distance and speed travelled
//encoder pins have to be 2 and 3 because fucking arduino.
const int ENC_A = 2;
const int ENC_B = 3;

int LEYE_current_state;
int REYE_current_state;

//unsigned long starting;
//unsigned long ending;     debugging stuff
int distance_maxxing = 50;  //maximum distance at which the us sensor detects objects
int IncrSpeed = speed + Output;
//int IncrSpeedL = speed + Output;

//NewPing library helps remove unnecessary delays for US Sensor
NewPing UltraSonic(US_TRIG, US_ECHO, distance_maxxing);

float obst_distance = UltraSonic.ping_cm(); //current distance in cm of obstacle from US Sensor
//volatile float distance_travelled = 0.0;  //this is distance travelled by buggy
float avg_time = 0.0;     //this is the time taken between each pulse emitted by the wheels
float avg_speed = 0.0;  //this is the speed between each pulse emitted by the wheels
float avg_distance_travelled = 0.0; //the total distance travelle by buggy

void setup() {
  Serial.begin(115200); //115200 is the baud rate, this is the number of times the signal (messages) are sent per second
  //increased from 9600 because some information wasnt being sent quick enough to the arduino
  WiFi.beginAP(ssid, pass);   //used beginAP because its a WAP
  Serial.print("Connecting to WiFi...");
  while(WiFi.status() != WL_AP_CONNECTED){  //while wifi is not connected
    Serial.print(".");
    delay(1500);
  }
  Serial.println("Connected!");
  Serial.print("IP Address:");   
  Serial.println(WiFi.localIP());  //the IP Address
  server.begin();

  //Outputs give out info, Inputs receive Info
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
  //Without INPUT_PULLUP → The encoder pin can randomly switch between HIGH and LOW, causing glitches.
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  //interrupts pause the main program and then run this function, which is time_monitor
  //RISING means the function is run when the encoder pins (which are interrupted) go from LOW to HIGH
  attachInterrupt(digitalPinToInterrupt(ENC_A), time_monitor_A, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B), time_monitor_B, RISING);
  Input = digitalRead(LEYE) - digitalRead(REYE);  //PID input is the state of the sensors
  Setpoint = 0; //setpoint is the desired error

  MBS.SetMode(AUTOMATIC);
  MBS.SetSampleTime(10);  //ensures PID is computed quickly
}

void loop() {
  //starting = micros();
  client = server.available();
  if (client.connected()){  //used instead of (client) because this checks for an inactive connection
    //Serial.println("Client is still Online");
    if(client.available()) {  //if there is data to be read
      String RequestFromClient = client.readStringUntil('\n');  // Read client request
      if (RequestFromClient.substring(0, 6) == "Speed:"){
        speed = RequestFromClient.substring(6).toInt();
        //Serial.println(RequestFromClient);
      }
      else if (RequestFromClient == "Proceed lil bro")
        Proceed();
      else if (RequestFromClient == "Halt lil bro") 
        Halt();
      else if (RequestFromClient == "Update lil bro"){  //this is mainly used to just ensure new speed and distance values are sent
      }
      }
    }
 //   Serial.println("AVG Speed: " + String(avg_speed) + "," + "AVG Distance: " + String(avg_distance_travelled) + "\n");
  Calc_avg_DST();
  if (client.connected()){
    client.print(String(avg_speed, 2) + "," + String(avg_distance_travelled, 2) + "\n");
  }
  if(BuggyActive){
    ActivateBuggy();  //this runs the ir sensor and us sensor stuff
  }
}

//functions for moving help with code redability
void moveForward(){
  //Serial.println("moving forward");
  analogWrite(ENA, speed);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  analogWrite(ENB, speed);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW);
}

void turnLeft(int speedchange){
  //Serial.println("turning left");
  analogWrite(ENA, 0);  //one wheel turns off when turning
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, LOW);
  analogWrite(ENB, speedchange);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW); 
}

void turnRight(int speedchange){
  //Serial.println("turning right");
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speedchange);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
}

void stop(){
  //Serial.println("stopping");
  analogWrite(ENA, 0);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, LOW);
  analogWrite(ENB, 0);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

//isr functions
void time_monitor_A(){
  unsigned long current_time = micros();    //TIME FROM INITIALISATION
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
  // Serial.print("Pulse Count a: ");
  // Serial.println(pulseCount_ENC_A);
  // Serial.print("Pulse Count b: ");
  // Serial.println(pulseCount_ENC_B);
  // Serial.print("Total pulses: ");
  // Serial.println(total_pulses);

  //you travel once circumference per revolution, the *2 is because of both wheels
  avg_distance_travelled = Circumference*total_pulses/((float)(Pulse_pre_rev * 2));
  
  // Calculate time in seconds
  avg_time = (Time_ENC_A + Time_ENC_B)/2000000.0; // because Time_ENC_A and B are measured in mircoseconds
  if(avg_time > 0.0001) { // helps avoid division by very small numbers
    avg_speed = Circumference/(Pulse_pre_rev*avg_time); //remember this is the speed for each pulse
  } else {
      avg_speed = 0;  //if time between pulses is just say speed is 0 at that point
  }
        
  
}

void obstacle_detection(float distance){
  if(distance > 0){ //ignore if the sensor says 0 or if it somehow says < 0
    if(distance < 15){ //if an obstacle is close and it hasnt reported an obstacle yet
      obstacle_detected = true; //now it knows the message has been sent that the obstacle has been detected
      //Serial.println("Holy Crap I'm about to hit something");
      client.print("About to hit something\n");
      info_sent = 1;
    }
    else if(distance > 15){
      obstacle_detected = false;  //obstacle is now no longer there
      client.print("obstacle_cleared\n"); //client.print sends stuff to the client, which is processing
    }
  }
  else {
    obstacle_detected = false;
    if (info_sent)
      client.print("obstacle_cleared\n");
  }
}

void Proceed(){
  BuggyActive = true; //now knows that the buggy is available to be moved
}

void Halt(){
  stop(); 
  BuggyActive = false;  //now knows that the buggy shouldnt be moved at all
  avg_speed = 0;  //speed and time between pulses are reset to 0 when halted
  Time_ENC_A = 0;
  Time_ENC_B = 0;
}

void adjustSpeed(){
  Input2 = UltraSonic.ping_cm();
  Setpoint2 = 15;
  Mode2.Compute();
  double error = Setpoint2-Input2;

  int adjustedSpeed = constrain(speed + Output2, 0, 255); // this line adjusts the speed depending on distance 
  if (error > 0){
 moveForward(adjustedSpeed);}
 }

void ActivateBuggy(){
  LEYE_current_state = digitalRead(LEYE);
  REYE_current_state = digitalRead(REYE); //re-read the ir sensor state each loop

  Input = -abs(LEYE_current_state - REYE_current_state);  //re-update the Input for every loop because the IR sensor values change

  MBS.Compute();
  IncrSpeed = speed + Output;


  obst_distance = UltraSonic.ping_cm(); //current distance in cm from US Sensor
  // if(obst_distance > 0 ){
  //   Serial.print("Distance: ");
  //   Serial.println(obst_distance); 
  //  }
  IncrSpeed = constrain(IncrSpeed, 0, 255); //ALlows speeds to remain within a certain boundary
  //IncrSpeedR = constrain(IncrSpeedR, 0, 255); //ALlows speeds to remain within a certain boundary

  obstacle_detection(obst_distance);
  if (obst_distance < 15){
 stop();}
 else if (obst_distance >= 15 && obst_distance <= 50){
 Mode2.compute();
 adjustSpeed(); }
 
 else if(LEYE_current_state && REYE_current_state){ 
    moveForward();
  }
  else if(!LEYE_current_state && REYE_current_state){ //!LEYE_current_state basically means if LEFT IR Sensor is off
    turnLeft(IncrSpeed);
    Serial.println(IncrSpeed);
  }
  else if(!REYE_current_state && LEYE_current_state){
    turnRight(IncrSpeed);
    Serial.println(IncrSpeed);
  }
  else if (!LEYE_current_state && !REYE_current_state){
    stop();  
  }
  //ending = micros();
  //Serial.println("loop " + String(ending - starting));
}
