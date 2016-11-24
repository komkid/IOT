#include <ESP8266WiFi.h>
#include <time.h>

const char* ssid = "...";                  
const char* password = "...";          

int timezone = 7 * 3600;                    
int dst = 0;                                
struct tm* p_tm;

#include "Timer.h"
Timer t;
int timeEvent;

int updateInterval = 50000;//millisec.
unsigned int h;
unsigned int m;
unsigned int s;

#include <AntoIO.h>
// username of anto.io account
const char *user = "...";

// key of permission, generated on control panel anto.io
const char* key = "...";

// your default thing.
const char* thing = "...";

// create AntoIO object named anto.
// using constructor AntoIO(user, key, thing)
// or use AntoIO(user, key, thing, clientId)
// to generate client_id yourself.
AntoIO anto(user, key, thing);

bool bIsConnected = false;

int controlMode = 0;
int offTimeH = 1;
int offTimeM = 0;
int onTimeH = 18;
int onTimeM = 0;

#define RELAY01 12
#define RELAY02 13
#define LEDPIN 16
#define BUTTON1 2     // the number of the pushbutton pin
#define BUTTON2 0     // the number of the pushbutton pin

int state = 0;

void blinking(int qty=10){
  int n;
  for (n = 1; n < qty ; n++){
    digitalWrite(LEDPIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    delay(400);                      // Wait for a second
    digitalWrite(LEDPIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(400);                      // Wait for two seconds (to demonstrate the active low LED)
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

void updateIO(int relay, int state) {
  if (state == 1) {
    digitalWrite(relay, HIGH);
  } else {
    digitalWrite(relay, LOW);
  }
}

void setup()
{
  Serial.begin(115200);
 //Serial.setDebugOutput(true);

  WiFi.mode(WIFI_STA);                                        
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

  //port output
  pinMode(LEDPIN, OUTPUT);
  pinMode(RELAY01, OUTPUT);
  pinMode(RELAY02, OUTPUT);
      
  configTime(timezone, dst, "pool.ntp.org", "time.nist.gov");     
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
  printTimeNow();
  //if(h == 0 & m == 0 & s == 0) ESP.restart();
  if(controlMode == 1){
    if((h == 5 | h == 7 | h == 9 | h == 11 | h == 13 | h == 15 | h == 17) & m == 0){
      Serial.print("1:ON, 2:OFF");    
      anto.pub("Relay01", 1);
      updateIO(RELAY01, 1);
      delay(5000);
      anto.pub("Relay02", 0);
      updateIO(RELAY02, 0);
    }
    
    if((h == 6 | h == 8 | h == 10 | h == 12 | h == 14 | h == 16 | h == 18) & m == 0){
      Serial.print("1:OFF, 2:ON");    
      anto.pub("Relay01", 0);
      updateIO(RELAY01, 0);
      delay(5000);
      anto.pub("Relay02", 1);
      updateIO(RELAY02, 1);
    }
  }

  if(h > 18 & m == 0){
    anto.pub("Relay01", 0);
    updateIO(RELAY01, 0);
    delay(5000);
    anto.pub("Relay02", 0);
    updateIO(RELAY02, 0);
    delay(5000);
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
    
    anto.sub("ControlMode");
    anto.sub("Relay01");
    anto.sub("Relay02");
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

    if(topic.equals("ControlMode")){
      controlMode = msg.toInt();
      if(controlMode == 0)
        Serial.print("MODE : MANUAL");
      else
        Serial.print("MODE : AUTO");
    } else if(topic.equals("Relay01")){
      Serial.print("RELAY01 : ");
      if(controlMode == 0){
        state = msg.toInt();
        Serial.print(state);
        updateIO(RELAY01, state);
      }
    } else if(topic.equals("Relay02")){
      Serial.print("RELAY02 : ");
      if(controlMode == 0){
        state = msg.toInt();
        Serial.print(state);
        updateIO(RELAY02, state);
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
