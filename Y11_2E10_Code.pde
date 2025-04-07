import processing.net.*;
import controlP5.*;    //for GUI
import org.gicentre.utils.stat.*;  //for vel/time graph

Client myClient;  //using processing as the client in this case
ControlP5 cp5;
//initialise global variables
String distance = "0";  
String speed = "0";
float speedVal = 0;
String warning = "";
long prevUpdate = 0;
int mode = 0;
float obst_distance = 0;
boolean following_obstacle = false;
String Tag_info = "";

void setup() {
  size(1700, 900);  //gives a big area for GUI
  background(50);
  myClient = new Client(this, "192.168.4.1", 5200);  //matches 5200 from arduino code
  cp5 = new ControlP5(this);
  PFont buttonFont = createFont("Arial Bold", 38);  // Create a font with size 38

  cp5.addButton("Current mode")  //Mode button
     .setLabel("No Active Mode")  //default mode
     .setPosition(250, 250)
     .setSize(400, 80)
     .setColorBackground(color(227, 41, 215))
     .setColorActive(color(240, 104, 231))  //Slight difference between active and default colour for standing out
     .setFont(buttonFont)  // Setting the button label font
     .onClick(new CallbackListener() {  //Use event handlers for when buttons are pressed
       public void controlEvent(CallbackEvent theEvent) {
         ModebuttonClicked();
       }
     });
     
  // add a horizontal slider
  cp5.addSlider("BuggySpeed in m/s")
     .setPosition(250, 400)
     .setSize(400, 75)  //x Size is bigger than y size, hence horizontal
     .setRange(0, 0.74)
     .setValue(0)  //start at 0
     .onChange(new CallbackListener(){  //Use event handlers for when slider is changed
       public void controlEvent(CallbackEvent theEvent){
         SliderValueChanged(theEvent.getController().getValue());
       }
     });
  
  cp5.addButton("GOButton")  //go button
     .setLabel("GO")
     .setPosition(150,100)
     .setSize(200, 80)
     .setColorBackground(color(0, 255, 0))  // Set color to green
     .setColorForeground(color(0, 255, 0))
     .setColorActive(color(0, 255, 0))  
     .setFont(buttonFont)  //Set the button label font
     .onClick(new CallbackListener() {
       public void controlEvent(CallbackEvent theEvent) {
         GObuttonClicked();
       }
     });
  cp5.addButton("STOPButton")  //stop button
     .setLabel("STOP")
     .setPosition(550, 100)
     .setSize(200, 80)
     .setColorBackground(color(255, 0, 0))  //Set color to red
     .setColorActive(color(255, 0, 0)) 
     .setColorForeground(color(255, 0, 0))
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
  drawButtonBorder(550, 100, 200, 80, color(255, 0, 0), 50);

  textSize(50);  //Set bigger size for text
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
      if(data.equals("About to hit something")){  //First, check for obstacle detection
        warning = "OBSTACLE!!!";
      }
      else if(data.equals("obstacle_cleared")){
        warning = "Cleared :D";
      }
      else if(data.substring(0, 3).equals("Tag")){  //Then check for Tag ID
        Tag_info = data;
      }
      else if(!data.contains(",")){                //Then check if obstacle is being followed
        obst_distance = float(data);
        following_obstacle = true;
      }
      else{  //if the data sent is not above criteria it will be sending distance and speed
        String[] DST =  data.split(",");  // Split speed and distance
        if (DST.length == 2) {
          speed = DST[0];
          distance = DST[1];
        }
      }
    }
  }
  speedVal = float(speed); 
  if (speedVal > 0) {
    fill(0, 255, 0); // Green when moving
  } else {
    fill(255, 0, 0); // Red when stopped
  }
  drawTable();  //Redraw table after values are updated
 // text("Speed: " + speed + "m/s", 1, 600);
  //fill(255);  //changes colour of the text to white
 // text("Distance: " + distance + "m", 550, 600);
  if (!warning.isEmpty()) {    //if there is a warning
    if(warning.equals("OBSTACLE!!!")) {
      fill(255, 0, 0);  // Red for obstacle warning
    } else if(warning.equals("Cleared :D")) {
        fill(0, 255, 0);  // Green for all clear
    }
    textSize(70);  
    text(warning, 400, 750);
    textSize(50);  // Reset text size
  }
  if (following_obstacle){
    fill(255, 255, 255);
    text("Obstacle at " + obst_distance, 600, 850);
    //following_obstacle = false;
  }

}

//Event handlers
void GObuttonClicked() {
  myClient.write("Proceed lil bro\n");  //Sending info to arduino
}

void STOPbuttonClicked() {
  myClient.write("Halt lil bro\n");
}

void SliderValueChanged(float current_speed){
  myClient.write("Speed:" + Float.toString(current_speed) + "\n");  //Ensure data is converted correctly
}

void ModebuttonClicked(){
  //Change mode whenever button is pressed to next mode
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
void drawTable() {
  float tableX = 100;
  float tableY = 550;
  float rowHeight = 30;
  float columnWidth = 700;
  
  // Table header
  fill(30);
  rect(tableX, tableY, columnWidth, rowHeight * 4);
  noStroke();
  
  // Row text settings
  fill(255);
  textSize(30);
  textAlign(LEFT, CENTER);
  
  // Row 1: Speed
  fill(50);
  rect(tableX, tableY, columnWidth, rowHeight);
  fill(255);
  text("Speed: ", tableX + 20, tableY + rowHeight / 2);
  textAlign(RIGHT, CENTER);
  text(speedVal + " m/s", tableX + 300 , tableY + rowHeight / 2);

  // Row 2: Distance
  fill(60);
  rect(tableX, tableY + rowHeight, columnWidth, rowHeight);
  fill(255);
  textAlign (LEFT, CENTER);
  text("Distance: ", tableX + 20, tableY + rowHeight * 1.5);
  textAlign(RIGHT, CENTER);
  text(distance + " m", tableX + 300, tableY + rowHeight * 1.5);

  // Row 3: Placeholder for another value
  fill(50);
  rect(tableX, tableY + rowHeight * 2, columnWidth, rowHeight);
  fill(255);
  textAlign (LEFT, CENTER);
  text("Latest Tag Read:", tableX + 20, tableY + rowHeight * 2.5);
  textAlign (RIGHT, CENTER);
  if(Tag_info.isEmpty() || Tag_info.length() < 3 || !Tag_info.substring(0, 3).equals("Tag")){ //default or invalid tag output
  text("...", tableX + 300, tableY + rowHeight * 2.5);
  }
  else text(Tag_info, tableX + 500, tableY + rowHeight * 2.5);  //valid tag output
  // Row 4: Placeholder for another value
  fill(60);
  rect(tableX, tableY + rowHeight * 3, columnWidth, rowHeight);
  fill(255);
  textAlign (LEFT, CENTER);
  text("Tag Info: ", tableX + 20, tableY + rowHeight * 3.5);
  textAlign (RIGHT, CENTER);
  switch(Tag_info){  //Switch-case for tag output
    case "Tag 1":
      text("Moving Quicker :0", tableX + 300, tableY + rowHeight * 3.5);
      break;
    
    case "Tag 2":
      text("Following Speed Limt :0", tableX + 300, tableY + rowHeight * 3.5);
      break;
    
    case "Tag 3":
      text("Turning Left at Junction :0", tableX + 300, tableY + rowHeight * 3.5);
      break;
    
    case "Tag 4":
      text("Turning Right at Junction :0", tableX + 300, tableY + rowHeight * 3.5);
      break;
   
    default:
      text("...", tableX + 300, tableY + rowHeight * 3.5);
  }
}
