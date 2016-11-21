#include <AntoIO.h>

// username of anto.io account
const char *user = "komkid";

// key of permission, generated on control panel anto.io
const char* key = "key";

// your default thing.
const char* thing = "ElectroDargonPlug01";

// create AntoIO object named anto.
// using constructor AntoIO(user, key, thing)
// or use AntoIO(user, key, thing, clientId)
// to generate client_id yourself.
AntoIO anto(user, key, thing);

bool bIsConnected = false;

int Led1,Led2,Led3 = 0;
int value = 0;
int controlMode = 0;


void setup() {
    // SSID and Password of your WiFi access point.
    const char* ssid = "SSID";
    const char* pwd  = "PASSWORD";
  
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
    Serial.print("Anto library version: ");
    Serial.println(anto.getVersion());


    // Connect to your WiFi access point
    if (!anto.begin(ssid, pwd)) {
        Serial.println("Connection failed!!");

        // Stop everything.
        while (1);
    }

    Serial.println();
    Serial.print("WiFi connected :");  
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
  //pinMode(D0,OUTPUT);
    //pinMode(D1,OUTPUT);
    pinMode(12,OUTPUT);
    pinMode(13,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
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
   
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(msg);

    if(topic.equals("ControlMode")){
      controlMode = msg.toInt();
      if(value == 1){
        Serial.println("AUTO");
      }
      else{
        Serial.println("MANUAL");
      }
    }
    else if(topic.equals("Relay01")){
     value = msg.toInt();
     if(value == 1){
        Serial.println("RELAY1 : ON");
        digitalWrite(12,HIGH);
      }
      else{
        Serial.println("RELAY1 : OFF");
        digitalWrite(12,LOW);
      }
    }
    else if(topic.equals("Relay02")){
      value = msg.toInt();
      if(value == 1){
        Serial.println("RELAY2 : ON");
        digitalWrite(13,HIGH);
      }
      else{
        Serial.println("RELAY2 : OFF");
        digitalWrite(13,LOW);
      }
    }
}

/*
* publishedCB(): a callback function called when the message is published.
*/
void publishedCB(void)
{
    Serial.println("published");
}
