import processing.net.*;
import controlP5.*;

Client myClient;
String distance = "0";
String speed = "0";
String warning = "";
ControlP5 cp5;
long prevUpdate = 0;
int mode = 0;
float obst_distance;
boolean following_obstacle = false;

void setup() {
  size(900, 900);
  background(50);
  myClient = new Client(this, "192.168.4.1", 5200);  //,atches 5200 from arduino code
  cp5 = new ControlP5(this);
  PFont buttonFont = createFont("Arial Bold", 38);  // Create a font with size 38

  cp5.addButton("Current mode")  //go button
     .setLabel("No Active Mode")
     //.getCaptionLabel().setSize(420)
     .setPosition(270, 700)
     .setSize(400, 80)
     .setColorBackground(color(227, 41, 215))
     .setColorActive(color(240, 104, 231))  
     .setFont(buttonFont)  // Set the button label font
     .onClick(new CallbackListener() {  //use event handlers for when buttons are pressed
       public void controlEvent(CallbackEvent theEvent) {
         ModebuttonClicked();
       }
     });
     
  // add a vertical slider
  cp5.addSlider("BuggySpeed in m/s")
     .setPosition(250, 305)
     .setSize(400, 75)
     .setRange(0, 0.74)
     .setValue(0)
     .onChange(new CallbackListener(){
       public void controlEvent(CallbackEvent theEvent){
         SliderValueChanged(theEvent.getController().getValue());
       }
     });
  
  cp5.addButton("GOButton")  //go button
     .setLabel("GO")
     //.getCaptionLabel().setSize(420)
     .setPosition(160,100)
     .setSize(200, 80)
     .setColorBackground(color(0, 255, 0))  // Set color to green
     .setColorActive(color(200, 255, 200))  
     .setFont(buttonFont)  // Set the button label font
     .onClick(new CallbackListener() {  //use event handlers for when buttons are pressed
       public void controlEvent(CallbackEvent theEvent) {
         GObuttonClicked();
       }
     });
  cp5.addButton("STOPButton")  //stop button
     .setLabel("STOP")
     .setPosition(500, 100)
     .setSize(200, 80)
     .setColorBackground(color(255, 0, 0))  //Set color to red
     .setColorActive(color(255, 255, 255)) 
     .setFont(buttonFont)
     .onClick(new CallbackListener() {
       public void controlEvent(CallbackEvent theEvent) {
         STOPbuttonClicked();
       }
     });
}

void draw() {
  background(50);
  drawButtonBorder(160, 100, 200, 80, color(0, 255, 0), 50); // Border with green color and thickness 5px
  drawButtonBorder(500, 100, 200, 80, color(255,0,0),50);
  textSize(50);
  if (millis() - prevUpdate > 100){    //Making sure values are updated regularly
    if(myClient.active()){
      myClient.write("Update lil bro\n");
      prevUpdate = millis();
    }
  }
  while (myClient.available() > 0) {  //Remember .available means there is data waiting to be read
    String data = myClient.readStringUntil('\n');  // Read incoming data
     if(data != null){
      data = data.trim();    //Removes unnecessary whitespaces
      if(data.equals("About to hit something")){
        warning = "OBSTACLE!!!";
      }
      else if(data.equals("obstacle_cleared")){
        warning = "Cleared :D";
      }
      else if(!data.contains(",")){
        obst_distance = float(data);
        following_obstacle = true;
      }
      else{  //if the data sent is not warning about an obstacle it will be sending distance and speed
        String[] DST =  data.split(",");  // Split speed and distance
        if (DST.length == 2) {
          speed = DST[0];
          distance = DST[1];
        }
      }
  }
 }
  float speedVal = float(speed); 
  if (speedVal > 0) {
    fill(0, 255, 0); // Green when moving
  } else {
    fill(255, 0, 0); // Red when stopped
  }
  text("Speed: " + speed + "m/s", 1, 600);
  fill(255);  //changes colour of the text to white
  text("Distance: " + distance + "m", 550, 600);
  if (!warning.isEmpty()) {    //if there is a warning
  if(warning.equals("OBSTACLE!!!")) {
        fill(255, 0, 0);  // Red for obstacle warning
    } else if(warning.equals("Cleared :D")) {
        fill(0, 255, 0);  // Green for all clear
    }
    textSize(70);
    text(warning, 250, 150);
    textSize(50);  // Reset text size
  }
  //if (warning.equals("Cleared :D")){
  // fill(255, 255, 255);
  // text("Not Following D:", 200, 850);
  //}
  if (following_obstacle){
    fill(255, 255, 255);
    text("Obstacle at " + obst_distance, 200, 850);
    //following_obstacle = false;
  }

}

//Event handlers
void GObuttonClicked() {
  myClient.write("Proceed lil bro\n");
}

void STOPbuttonClicked() {
  myClient.write("Halt lil bro\n");
}

void SliderValueChanged(float current_speed){
  myClient.write("Speed:" + Float.toString(current_speed) + "\n");
}

void ModebuttonClicked(){
  mode = mode + 1;
  if(mode > 2){
    mode = 0;
  }
  switch(mode){
    case 0:
      cp5.getController("Current mode").setLabel("No Active Mode");
      break;
    
    case 1:
      cp5.getController("Current mode").setLabel("Mode 1");
      break;
    
    case 2:
      cp5.getController("Current mode").setLabel("Mode 2");
      break;
     }
  myClient.write("Mode lil bro:" + Integer.toString(mode) + "\n");
}
void drawButtonBorder(float x, float y, float w, float h, color borderColor, float borderThickness) {
  noFill(); // No fill for the button, just the border
  stroke(borderColor);
  strokeWeight(borderThickness);
  rect(x, y, w, h, 20);  // Rounded rectangle with border
}
