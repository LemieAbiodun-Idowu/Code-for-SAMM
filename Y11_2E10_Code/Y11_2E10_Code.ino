#include <Math.h>       //for exponent usage
#include <NewPing.h>    //used for US Sensor
#include <WiFiS3.h>     //library for wifi
#include <PID_v1.h>     
#include "HUSKYLENS.h"  //library for camera

char ssid[] = "Y11 SAMM"; 
char pass[] = "12345678"; 
//our arduino is a Wireless Acess Point, it basically acts as its own wifi
WiFiServer server(5200);  //the port number used
WiFiClient client;

HUSKYLENS huskylens; //HUSKYLENS green line >> SDA, blue line >> SCL
HUSKYLENSResult result;
const int close_height = 168; //value in pixels for a close tag
const int close_width = 168;  //value in pixels for a close tag (based on camera reading)
const double close_distance = 0.085;  //value in cm for a close tag (based on distance from camera)
const double pixel_conversion_to_cm = close_distance / (close_width * close_height);  
double acceleration;  //used as deceleration as the buggy speed will reduce due to a tag
int ID = 0; //Tag ID set as null initially

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

//PID Implementation for smooth turns
double Input_Turn, Setpoint_Turn, Output_Turn;
double Kp_Turn = 30, Ki_Turn = 0.5, Kd_Turn = 12;

PID MBS(&Input_Turn, &Output_Turn, &Setpoint_Turn, Kp_Turn, Ki_Turn, Kd_Turn, DIRECT);

//PID Implementation for Mode 1
double Input1, Setpoint1, Output1;
double Kp1 = 0.5, Ki1 = 0.4, Kd1 = 0.0;

PID Mode1(&Input1, &Output1, &Setpoint1, Kp1, Ki1, Kd1, DIRECT);

//PID Implementation for Mode 2
double Input2, Setpoint2, Output2;
double Kp2 = 6, Ki2 = 2.5, Kd2 = 0;

PID Mode2(&Input2, &Output2, &Setpoint2, Kp2, Ki2, Kd2, REVERSE);

//const values are NEVER changed
const int LEYE = 4;   //Eye = IR Sensors
const int REYE = 12;
int speed = 110;      //PWM value for speed of buggy
const int US_TRIG = 9;  //Sends the Ultrasonic Pulse
const int US_ECHO = 8;  //Receives the Ultrasonic Pulse

bool obstacle_detected = false;
bool obst_info_sent = false;
bool BuggyActive = false;
const double MIOH = 12750/37; //conversion between m/s and PWM values
bool BeginTurn = false; //denotes if a turn has begun at a junction
bool turningLeftatIntersection = false;
bool turningRightatIntersection = false;
int followingSpeed;     //speed at which buggy follows an obstacle
float slider_speed = 0; //slider used in mode 2

//Motor A pins
const int ENA = 5;    //EN = enable pins for the motor
const int IN1 = 16;
const int IN2 = 15;

//Motor B pins
const int ENB = 11;
const int IN3 = 14;
const int IN4 = 17;

//Encoder pins, the wheel encoders help determine the distance and speed travelled
//Encoder pins have to be 2 and 3.
const int ENC_A = 2;
const int ENC_B = 3;

int LEYE_current_state;
int REYE_current_state;

int mode = 0; //initialise to no active mode
// unsigned long starting;
// unsigned long ending;     //debugging stuff
int distance_maxxing = 50;  //maximum distance at which the us sensor detects objects
int IncrSpeed = speed + Output_Turn;


//NewPing library helps remove unnecessary delays for US Sensor
NewPing UltraSonic(US_TRIG, US_ECHO, distance_maxxing);

float obst_distance = UltraSonic.ping_cm(); //current distance in cm of obstacle from US Sensor
float avg_time = 0.0;     //this is the time taken between each pulse emitted by the wheels
float avg_speed = 0.0;  //this is the speed between each pulse emitted by the wheels
float avg_distance_travelled = 0.0; //the total distance travelle by buggy

void setup() {
  Serial.begin(115200); //115200 is the baud rate, this is the number of times the signal (messages) are sent per second
  while(!Serial) { delay(1000); }
  //increased from 9600 because some information wasnt being sent quick enough to the arduino
  WiFi.beginAP(ssid, pass);   //used beginAP because its a WAP
  delay(1000);  //Delay to ensure info is printed
  Serial.print("Connecting to WiFi...");
  Serial.println("Access Point started.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Wire.begin(); 
  Wire.setClock(400000);  //ensures swift I2C communication
  Serial.println("Huskylens connected succesfully!");
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
  //Interrupts pause the main program and then run this function, which is time_monitor
  //RISING means the function is run when the encoder pins (which are interrupted) go from LOW to HIGH
  attachInterrupt(digitalPinToInterrupt(ENC_A), time_monitor_A, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_B), time_monitor_B, RISING);
  Input_Turn = -abs(LEYE_current_state - REYE_current_state);  //re-update the Input for every loop because the IR sensor values change
  Input1 = avg_speed;
  Input2 = UltraSonic.ping_cm();
  //Setpoint represents the desired state  
  Setpoint_Turn = 0;  //desired state is 0 error
  Setpoint1 = slider_speed; //desired state is slider value for speed
  Setpoint2 = 15; //desired state is 15cm from object
  MBS.SetMode(AUTOMATIC);
  Mode1.SetMode(AUTOMATIC);
  Mode2.SetMode(AUTOMATIC);
  MBS.SetSampleTime(10);  //ensures PID is computed quickly
  Mode1.SetSampleTime(10);
  Mode2.SetSampleTime(10);
}

void loop() {
  //starting = micros();
  client = server.available();
  if (client.connected()){  //used instead of (client) because this checks for an inactive connection
    if(client.available()) {  //if there is data to be read
      String RequestFromClient = client.readStringUntil('\n');  // Read client request
      if(RequestFromClient.substring(0, 13) == "Mode lil bro:"){
        mode = RequestFromClient.substring(13).toInt();
          // if (mode == 1)
          //   speed = 120; //for overcoming initial inertia
          if (mode == 2) 
            followingSpeed = 100; // Default following speed
      }
      if (mode == 1){
        if (RequestFromClient.substring(0, 6) == "Speed:"){
          slider_speed = RequestFromClient.substring(6).toFloat();
        }
      }
      if (RequestFromClient == "Proceed lil bro")
        Proceed();
      else if (RequestFromClient == "Halt lil bro")
        Halt();
      else if (RequestFromClient == "Update lil bro"){  //this is mainly used to just ensure new speed and distance values are sent
      }
    }
  }
  Calc_avg_DST(); //obtain updated speed and distance value
  if (client.connected()){
    client.print(String(avg_speed, 2) + "," + String(avg_distance_travelled, 2) + "\n");
  }
  if(BuggyActive){
    ActivateBuggy();  //this runs the ir sensor and us sensor stuff
  }
}

void printResult(HUSKYLENSResult tag){
    if (result.command == COMMAND_RETURN_BLOCK){  //April tags are read as blocks
      Serial.println("April Tag");
      Serial.print("ID: "); 
      Serial.println(tag.ID);
      Serial.print("X Center: "); 
      Serial.println(tag.xCenter);
      Serial.print("Y Center: ");
      Serial.println(tag.yCenter);
      //Width and Height are read in pixels
      Serial.print("Width: "); 
      Serial.println(tag.width); 
      Serial.print("Height: ");       
      Serial.println(tag.height);
    }
    else Serial.println("Object unknown!");
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
  //Prevent Integral Accumulation Error in PID's
  resetTurnIntegral();
  resetM1Integral();
}

void JunctionMovement(){
  if(LEYE_current_state && REYE_current_state){
    if(BeginTurn){
      ID = 0;
      BeginTurn = false;
    }
    if(mode == 2 && obst_distance > 0 && obst_distance < 50){
      client.print(String(obst_distance, 2) + "\n");
      Mode2_Movement();
    }
    else moveForward();
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
    if(ID == 3 || ID == 4){
      if(turningLeftatIntersection){
        turnLeft(IncrSpeed);
        BeginTurn = true;
      }
      else if(turningRightatIntersection){
        turnRight(IncrSpeed);
        BeginTurn = true;
      }
    }       
    else {    
      moveForward();
      turningLeftatIntersection = false;
      turningRightatIntersection = false;
    }
  }
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
  if(avg_time > 0.0001 && BuggyActive) { // helps avoid division by very small numbers
    avg_speed = Circumference/(Pulse_pre_rev * avg_time); //remember this is the speed for each pulse
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
      obstcl_info_sent = true;
    }
    else if(distance > 15){
      obstacle_detected = false;  //obstacle is now no longer there
      if (obstcl_info_sent)
        client.print("obstacle_cleared\n"); //client.print sends stuff to the client, which is processing
    }
  }
  else {
    obstacle_detected = false;
    if (obstcl_info_sent)
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

void Slider_PID(){
  //int prev_Input1 = Input1;
  Input1 = avg_speed;
  // if(abs(prev_Input1 - Input1) > 0.05){
  //   resetM1Integral();
  // }
  if(slider_speed > 0 && avg_speed < 0.02) {
    //resetM1Integral();
    speed = 120; // Initial speed to overcome inertia
  }
  Setpoint1 = slider_speed;
  Mode1.Compute();
  double slided_speed = Output1;
  slided_speed = constrain(slided_speed, 0, 0.74);
  Serial.println(slided_speed);
  speed = slided_speed * MIOH;
  speed = constrain(speed, 0, 255);
  Serial.println("Target: " + String(slider_speed) + ", Actual: " + String(avg_speed) + ", Output: " + String(slided_speed) + ", Speed: " + String(speed));
}

void obstacle_following(){
  //int prev_Input2 = Input2;
  Input2 = UltraSonic.ping_cm();
  // if(prev_Input2 != Input2){

  // }
  Mode2.Compute();
  //double error = Setpoint2-Input2;
  followingSpeed = Output2;
  followingSpeed = constrain(followingSpeed, 60, 220); // this line adjusts the speed depending on distance
  if(Input2 < 10) { //slow down or stop
      followingSpeed = constrain(followingSpeed, 0, 80);
    } else if(Input2 > 20) { // go faster
      followingSpeed = constrain(followingSpeed, 120, 220);
    } else { // About right distance
      followingSpeed = constrain(followingSpeed, 60, 180);
    }
   
  // if (error > 0){hed
  //   moveForward(adjustedSpeed);
  // }
}

void Mode2_Movement(){
  analogWrite(ENA, followingSpeed);
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  analogWrite(ENB, followingSpeed);
  digitalWrite(IN4, HIGH);
  digitalWrite(IN3, LOW);
  Serial.println(followingSpeed);
}

void resetTurnIntegral() {
  //Prevents Output Buildup after buggy has been stopped.
  MBS.SetMode(MANUAL);  
  Output_Turn = 0;
  MBS.SetMode(AUTOMATIC);
}

void resetM1Integral(){
  Mode1.SetMode(MANUAL);
  Output1 = 0; // Constrain initial output
  Mode1.SetMode(AUTOMATIC);
}

void ActivateBuggy(){
  LEYE_current_state = digitalRead(LEYE);
  REYE_current_state = digitalRead(REYE); //re-read the ir sensor state each loop

  // First, check that we have the huskylens connected...
  if (!huskylens.request()) {
    Serial.println(F("Fail to request data from HUSKYLENS, recheck the connection!"));
    //delay(1000);
  }
  // then check that it's been trained on something...
  else if (!huskylens.isLearned()) {
    Serial.println(F("Nothing learned, press learn button on HUSKYLENS to learn one!"));
    //delay(1000);
  }
  // Then check whether there are any blocks visible at this exact moment...
  else if (!huskylens.available()) {
    Serial.println(F("No tag appears on the screen!"));
    //delay(1000);
  }
  else {
    // OK, we have some blocks to process. available() will return the number of blocks to work through.
    // fetch them using read(), one at a time, until there are none left. Each block gets given to
    // printResult() function to be printed out to the serial port.
    if (huskylens.available()) {
      result = huskylens.read();
      printResult(result);
    }
  }
  if (huskylens.available()){
    if(result.ID == 1){ //Set Speed to Max Value
      speed = 150;
      ID = 1;
    }
    else if(result.ID == 2){  //Follow a Speed Limit based on tag position
      if(result.width < (close_width) && result.height < (close_height)){
        int tag_distance = result.width*result.height*pixel_conversion_into_cm;
        acceleration = (pow((90/MIOH), 2) - pow((speed/MIOH), 2))/(2*tag_distance); 
        speed = speed + (MIOH*acceleration);  //v^2 = u^2 + 2as  area dim = 4.3 x 4.2 (upright) in cm
      }
      else speed = 90;
      speed = constrain(speed, 90, 255);
      ID = 2;
    }
    else if(result.ID == 3){  //turn left at the NEXT intersection only 
      turningLeftatIntersection = true;
      ID = 3;
    }
    else if(result.ID == 4){  //turn right at the NEXT intersection only
      turningRightatIntersection = true;
      ID = 4;
    }
  }
  int prev_input_turn = Input_Turn;
  Input_Turn = -abs(LEYE_current_state - REYE_current_state);  //re-update the Input for every loop because the IR sensor values change

  if(prev_input_turn != Input_Turn){
    resetTurnIntegral();
  }
  MBS.Compute();

  obst_distance = UltraSonic.ping_cm(); //current distance in cm from US Sensor
  // if(obst_distance > 0 ){
  //   Serial.print("Distance: ");
  //   Serial.println(obst_distance); 
  //  }
  if(mode == 1 && slider_speed > 0 && !(!LEYE_current_state && !REYE_current_state)){
    Slider_PID();
  }

  else if(mode == 2 && obst_distance > 0 && obst_distance <= 50){
    obstacle_following();
  }

  IncrSpeed = speed + Output_Turn;
  IncrSpeed = constrain(IncrSpeed, 0, 255); //ALlows speeds to remain within a certain boundary

  obstacle_detection(obst_distance);
  if (obstacle_detected){
    stop(); //stop if too close
  }                                   
  else if(turningLeftatIntersection || turningRightatIntersection){
    JunctionMovement();
  }
  else if(LEYE_current_state && REYE_current_state){
    if(mode == 2 && obst_distance > 0 && obst_distance < 50){
      client.print(String(obst_distance, 2) + "\n");
      Mode2_Movement();
    }
    else moveForward();
  }
  else if(!LEYE_current_state && REYE_current_state){ //!LEYE_current_state basically means if LEFT IR Sensor is off
    turnLeft(IncrSpeed);
    Serial.println(IncrSpeed);
  }
  else if(!REYE_current_state && LEYE_current_state){
    turnRight(IncrSpeed);
    Serial.println(IncrSpeed);
  }
  else stop();
  //ending = micros();
  //Serial.println("loop " + String(ending - starting));
}