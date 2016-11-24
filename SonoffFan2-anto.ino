#include <ESP8266WiFi.h>
#include <time.h>

const char* ssid = "...";                  //ใส่ชื่อ SSID Wifi
const char* password = "...";          //ใส่รหัสผ่าน

int timezone = 7 * 3600;                    //ตั้งค่า TimeZone ตามเวลาประเทศไทย
int dst = 0;                                //กำหนดค่า Date Swing Time
struct tm* p_tm;

#include "Timer.h"
Timer t;
int timeEvent;
int inputEvent;

int updateInterval = 5000;//millisec.
unsigned int h;
unsigned int m;
unsigned int s;

#include <AntoIO.h>
// username of anto.io account
const char *user = "...";

// key of permission, generated on control panel anto.io
const char* key = "...";

// your default thing.
const char* thing = "SonoffFan2";//AIO

// create AntoIO object named anto.
// using constructor AntoIO(user, key, thing)
// or use AntoIO(user, key, thing, clientId)
// to generate client_id yourself.
AntoIO anto(user, key, thing);

bool bIsConnected = false;

int value = 0;
int controlMode = 0;

#define RELAYPIN 12
#define LEDPIN 13
#define BUTTONPIN 0     // the number of the pushbutton pin
int state = 0;
int buttonState = 0;         // variable for reading the pushbutton status

void blinking(int qty=10){
  int n;
  for (n = 1; n < qty ; n++){
    digitalWrite(LEDPIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    delay(200);                      // Wait for a second
    digitalWrite(LEDPIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(200);                      // Wait for two seconds (to demonstrate the active low LED)
  }
}

void printTimeNow(){
  Serial.println();
  Serial.print(h); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if ( m < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print(m); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ( s < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print(s); // print the second
  Serial.print(" >> "); // print the second
}

void updateIO() {
  anto.pub("relay", state);
  if (state == 1) {
    digitalWrite(RELAYPIN, HIGH);
    #ifdef LEDPIN
      digitalWrite(LEDPIN, LOW);
    #endif
  }
  else {
    state = 0;
    digitalWrite(RELAYPIN, LOW);
    #ifdef LEDPIN
      digitalWrite(LEDPIN, HIGH);
    #endif
  }
}

void readInput()
{
  // read the state of the pushbutton value:
  buttonState = digitalRead(BUTTONPIN);

  // check if the pushbutton is pressed.
  // if it is, the buttonState is HIGH:
  if (buttonState == LOW) {
    printTimeNow();
    Serial.println("Button pressed");
    state = (state == 0) ? 1 : 0;
    updateIO();
//    delay(1000);
  }
}

void setup()
{
  Serial.begin(115200);
 //Serial.setDebugOutput(true);

  inputEvent = t.every(1000, readInput);
//  Serial.print("input event started id=");
//  Serial.println(inputEvent);

  WiFi.mode(WIFI_STA);                                        //เชื่อมต่อ Wifi
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
     Serial.print(",");
     delay(1000);
  }

  delay(10);

  Serial.print("Anto library version: ");
  Serial.println(anto.getVersion());

  // Connect to your WiFi access point
  if (!anto.begin(ssid, password)) {
      Serial.println("Connection failed!!");

      // Stop everything.
      while (1);
  }

  Serial.println();
  Serial.print("WiFi connected : ");  
  Serial.println(WiFi.localIP());
  
  Serial.println("Connecting to MQTT broker");

  // register callback functions
  anto.mqtt.onConnected(connectedCB);
  anto.mqtt.onDisconnected(disconnectedCB);
  anto.mqtt.onData(dataCB);
  anto.mqtt.onPublished(publishedCB);

  // Connect to Anto.io MQTT broker
  anto.mqtt.connect();

  pinMode(BUTTONPIN, INPUT);
  pinMode(RELAYPIN, OUTPUT);
  #ifdef LEDPIN
    pinMode(LEDPIN, OUTPUT);
  #endif

  state = 1;
  updateIO();
      
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");     //ดึงเวลาจาก Server
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  time_t now = time(nullptr);
  p_tm = localtime(&now);
  h = p_tm->tm_hour;
  m = p_tm->tm_min;
  s = p_tm->tm_sec;

  printTimeNow();
  Serial.println("Now");
  
  blinking(10);
  
  timeEvent = t.every(updateInterval, tikTok);
  //Serial.print("timeEvent event started id = ");
  //Serial.println(timeEvent);
}
        
void loop()
{
  t.update();
}

void tikTok(){
  s = s + (updateInterval / 1000);
  if((s / 60) > 0) {
    s = s % 60; 
    m++;
    if(m == 60){
      m = 0;
      h++;
      if(h == 24) h = 0;
    }
  }
  //printTimeNow();
  if(h == 0 & m == 0 & s == 0) ESP.restart();

  if(controlMode == 1){
    if(h == 8 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 9 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 9 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 10 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 10 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 11 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 11 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 12 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 12 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 13 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 13 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 14 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 14 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 15 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 15 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 16 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  
    if(h == 16 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 17 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
    
    if(h == 17 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 18 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
    
    if(h == 18 && m == 30){
      state = 1;
      Serial.println("ON");
      updateIO();
    }
    if(h == 19 && m == 20){
      state = 0;
      Serial.println("OFF");
      updateIO();
    }
  } 
}

/*
* connectedCB(): a callback function called when the connection to the MQTT broker is establised.
*/
void connectedCB()
{   
    // If the connection is establised, subscribe channels.

    bIsConnected = true;
    Serial.println("Connected to MQTT Broker");
    
    anto.sub("controlMode");
    anto.sub("relay");
}

/*
* disconnectedCB(): a callback function called when the connection to the MQTT broker is broken.
*/
void disconnectedCB()
{   
    bIsConnected = false;
    Serial.println("Disconnected to MQTT Broker");
}

/*
* msgArrvCB(): a callback function called when there a message from the subscribed channel.
*/
void dataCB(String& topic, String& msg)
{
    uint8_t index = topic.indexOf('/');

    index = topic.indexOf('/', index + 1);
    index = topic.indexOf('/', index + 1);

    topic.remove(0, index + 1);
   
    printTimeNow();
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(msg);

    if(topic.equals("controlMode")){
      controlMode = msg.toInt();
      if(controlMode == 1)
        Serial.print("MODE : AUTO");
      else
        Serial.print("MODE : MANUAL");
    } else if(topic.equals("relay")){
      Serial.print("RELAY : ");
      if(controlMode == 0){
        state = msg.toInt();
        Serial.print(state);
        updateIO();
      }
    } else Serial.println("Unknown topic!");

}

/*
* publishedCB(): a callback function called when the message is published.
*/
void publishedCB(void)
{
    Serial.println("published");
}
