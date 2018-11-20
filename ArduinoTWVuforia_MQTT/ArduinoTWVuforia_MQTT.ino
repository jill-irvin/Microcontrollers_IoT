#include <ESP8266WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker

const int buttonPinStatus = D7; //pin to read from push button - input pin
const int ledOnOff = D8;  //pin to write to for LED - output pin
const int ledVoltage = D2;  //reads voltage on LED

//global to keep track if led on or off - initialize to zero
int LEDStatus;
//int TWStatus;

// MQTT broker setup
const char* mqtt_server = "192.168.1.81";  //this is on RPi
const char* mqtt_topicLED = "TWTestLED";  //this topic is also set on TW server

// The client id identifies the ESP8266 device
const char* clientID = "ESP8266";

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker

//function declarations
void toggleLED(char updatedFrom);

/*
 * This function will be called each time board receives a message.
 * It's a subscriber to the topic.  Payload is the message. Length is size of message
 */
void receivedMessage(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("] ");

  //Update the pin that controls the LED based on input message but only if it's a change
  //if the first character is a 1 then update the pin to be HIGH
  if (((char)payload[0] == '1') && (LEDStatus != 1)) {
    //update the LED to be HIGH
    digitalWrite(ledOnOff, HIGH);   
    
    //update the ledstatus variable
    LEDStatus = 1;
    
   } else if (((char)payload[0] == '0')&& (LEDStatus != 0)){
    //update the LED to be HIGH
    digitalWrite(ledOnOff, LOW);  
    LEDStatus = 0;
    
  } else{
    Serial.println("Invalid input or redundant input");
  }

} //end messageCallback function definition


//run at board startup
void setup() {
  //debuggging tool
  Serial.begin(9600);
  Serial.printf("Starting...\n");
  
  LEDStatus=0;

  //initialize the pins' modes
  pinMode(buttonPinStatus, INPUT);  //pin that is read for Button press status - D7

 /*attach interrupt to the button pint status so that change will take affect immediately
   *issue with this is the interrupt is two edges! rising and falling because button clicked then released
   so use the loop and hold the button (update once get button that holds the state)
  //pin, callback function, interrupt type/mode
  //attachInterrupt(buttonPinStatus, toggleLED, CHANGE);
*/

  pinMode(ledOnOff, OUTPUT);  //pin that either powers on/off LED - pin D8

  //set the led to be off to start
  digitalWrite(ledOnOff, LOW);

//  pinMode(ledVoltage, INPUT);  //reads led voltage - pin D8
  //attachInterrupt(ledVoltage, sendUpdate, CHANGE);

  //connect board to wifi call with 10 second timeout
  connectToWiFi(20);

  //subscribe to messages of a topic mqtt_topicLED (from TW)
  client.subscribe(mqtt_topicLED);
  
  //set the callBack function to message Received function to handled subscriptions
  client.setCallback(receivedMessage);

  // Connect to MQTT Broker
  //more input parameters for connect function if user name and password needed
  if (connectMQTT()) {
    Serial.println("Connected Successfully to MQTT Broker!");  
  }
  else {
    Serial.println("Connection Failed!");
  }

  /* Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  if (client.connect(clientID)) {
    Serial.println("Connected to MQTT Broker!");   
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }  
  */

} //end setup


void loop() {
  // If the connection is lost, try to connect again
  if (!client.connected()) {
    connectMQTT();
  }
 
  // client checks for messages
  client.loop();
  // Once it has done all it needs to do for this cycle, go back to checking if we are still connected.

delay(2000);

  //allow for board button press to toggle the current state of the LED and tell TW
  //since the board and TW are both publishers and subscribers...TW will update based on publish
  //TW will also resend a message but nothing will happen on the reception other than the printed result
  
  if (digitalRead(buttonPinStatus) == HIGH){  //this pin status is pulled to 0 with pull-down resistor - only goes high with held in button press
    //check current status of the pin
    //do the opposite of the current status
    if(LEDStatus == 1){
      //turn-off the led
       digitalWrite(ledOnOff, LOW);   
    
      //update the ledstatus variable
      LEDStatus = 0;

      //update TW with change
      client.publish(mqtt_topicLED, "0");
    }
    else{
      //turn-on the led
       digitalWrite(ledOnOff, HIGH);   
    
      //update the ledstatus variable
      LEDStatus = 1;

     //update TW with change
      client.publish(mqtt_topicLED, "1");
      }
} //end digitalRead if   

}  //end loop
/*
void sendUpdate(){
  int convert = digitalRead(ledVoltage);
  char result[5];
  sprintf(result, "%f", convert);
  Serial.println(result);
  //supplyVoltage and LED voltage
  client.publish(mqtt_topicLEDVoltage, result); 
  }
*/
/*
  Description:  Toggles the current state of the LED. 
  Inputs:  none
  Return:  none

void toggleLED(char updatedFrom){
  Serial.println("toggling led w/ LEDStatus");
  Serial.println(LEDStatus);

  //determine if this call was from TW or button press
  if(updatedFrom == 'T'){
    //toggle LED state and update the LED status
    }

  
  //button pin status is pulled to 0 with pull-down resistor - only goes high with held in button press
  if(LEDStatus == 1){    
    //set the led to be off
     digitalWrite(ledOnOff, LOW);

    //button press also uses this fn,
        client.publish(mqtt_topicLED, "0");
    //  Serial.println("sent off message to TW");
    }
    else{
      //set the led to be on
     digitalWrite(ledOnOff, HIGH);
     // Serial.println("Sending on message to TW");
    // delay(500);
      //turn-on the led
      client.publish(mqtt_topicLED, "1");
      //Serial.println("sent on message to TW");
      }  
  }//end toggleLED
*/


/*
  Description:  Attempts to connect to MQTT server/broker. 
  Inputs:  timeout length
  Return:  boolean
*/
bool connectMQTT() {
  // Connect to MQTT Server and subscribe to the topic
  if (client.connect(clientID)) {
      
      //also subscribe to topic
      client.subscribe(mqtt_topicLED);
      return true;
    }
    else {
      return false;
  }
}

/*
  Description:  Attempt to make a WiFi connection. Checks if connection has been made 
  once per second until timeout is reached returns TRUE if successful or FALSE if timed out
  Inputs:  timeout length
  Return:  boolean
*/
boolean connectToWiFi(int timeout) {
  delay(10);
  //set mode of wifi to STA (station mode) b/c default has an access point setup
  WiFi.mode(WIFI_STA);
  WiFi.begin("Why_Phi", "bcalphathrutauweretaken");

  Serial.println("Connecting...");

  // loop while WiFi is not connected waiting one second between checks
  uint8_t tries = 0; // counter for how many times we have checked

  // stop checking if connection has been made OR we have timed out
  while ((WiFi.status() != WL_CONNECTED) && (tries < timeout)) {
    tries++;
    Serial.print(".");  // print . for progress bar
    Serial.println(WiFi.status());
    delay(500);
  }

  //let user know if board connected
  if (WiFi.status() == WL_CONNECTED) { //check that WiFi is connected, print status and device IP address before returning
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else { //if WiFi is not connected we must have exceeded WiFi connection timeout
    return false;
  }

}
