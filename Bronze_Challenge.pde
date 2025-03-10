import processing.net.*;
import controlP5.*;

Client myClient;
String distance = "0";
String speed = "0";
String warning = "";
ControlP5 cp5;
long prevUpdate = 0;  

void setup() {
  size(900, 900);
  background(50);
  myClient = new Client(this, "192.168.4.1", 5200);  //,atches 5200 from arduino code
  cp5 = new ControlP5(this);
  PFont buttonFont = createFont("Arial", 38);  // Create a font with size 38
  cp5.addButton("GOButton")  //go button
     .setLabel("GO")
     //.getCaptionLabel().setSize(420)
     .setPosition(300,450)
     .setSize(200, 80)
     .setColorBackground(color(0, 255, 0))  // Set color to green
     .setColorActive(color(255, 255, 255))  
     .setFont(buttonFont)  // Set the button label font
     .onClick(new CallbackListener() {  //use event handlers for when buttons are pressed
       public void controlEvent(CallbackEvent theEvent) {
         GObuttonClicked();
       }
     });
  cp5.addButton("STOPButton")  //stop button
     .setLabel("STOP")
     .setPosition(600, 450)
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
}

//Event handlers
void GObuttonClicked() {
  myClient.write("Proceed lil bro\n");
}

void STOPbuttonClicked() {
  myClient.write("Halt lil bro\n");
}
