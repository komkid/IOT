#include <AntoIO.h>
#include <WiFiUdp.h>

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
IPAddress timeServerIP(192, 168, 0, 2); // time.nist.gov NTP server
//IPAddress timeServerIP; // time.nist.gov NTP server address
//const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

#include "Timer.h"
Timer t;
int ntpEvent;
unsigned int h;
unsigned int m;
unsigned int s;

// username of anto.io account
const char *user = "komkid";

// key of permission, generated on control panel anto.io
const char* key = "xxxxxxxxxxxxxx";

// your default thing.
const char* thing = "ElectroDargonPlug01";

// create AntoIO object named anto.
// using constructor AntoIO(user, key, thing)
// or use AntoIO(user, key, thing, clientId)
// to generate client_id yourself.
AntoIO anto(user, key, thing);

bool bIsConnected = false;

int value = 0;
int controlMode = 0;

void blinking(int n=10){
  int i;
  for (i = 1; i < n ; i++){
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    delay(200);                      // Wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    delay(200);                      // Wait for two seconds (to demonstrate the active low LED)
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void getNTP()
{
  //get a random server from the pool
  //WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears + (7 * 60 * 60);
    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The BKK time is ");       // UTC is the time at Greenwich Meridian (GMT)
    h = (epoch  % 86400L) / 3600;
    Serial.print(h); // print the hour (86400 equals secs per day)
    Serial.print(':');
    m = (epoch  % 3600) / 60;
    if ( m < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(m); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    s = (epoch  % 60);
    if ( s < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(s); // print the second

    t.stop(ntpEvent);
    Serial.print("ntp event stopped id=");
    Serial.println(ntpEvent);

    blinking(20);
  }
}

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
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  t.update();
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

    int ntpEvent = t.every(10000, getNTP);
    Serial.print("ntp event started id = ");
    Serial.println(ntpEvent);
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
      if(controlMode == 1)
        Serial.print("MODE : AUTO");
      else
        Serial.print("MODE : MANUAL");
    } else if(topic.equals("Relay01")){
      Serial.print("RELAY1 : ");
      if(controlMode == 1){
        value = msg.toInt();
        if(value == 1){
          Serial.print("ON");
          digitalWrite(12, HIGH);
        }else{
          Serial.print("OFF");
          digitalWrite(12, LOW);
        }
      }
    } else if(topic.equals("Relay02")){
      Serial.print("RELAY2 : ");
      if(controlMode == 1){
        value = msg.toInt();
        if(value == 1){
          Serial.print("ON");
          digitalWrite(13, HIGH);
        }
        else{
          Serial.print("OFF");
          digitalWrite(13, LOW);
        }
      }
    } else Serial.println("Unknown topic!");

    Serial.println("");
    Serial.print(h);
    Serial.print(":");
    Serial.print(m);
    Serial.print(":");
    Serial.println(s);
}

/*
* publishedCB(): a callback function called when the message is published.
*/
void publishedCB(void)
{
    Serial.println("published");
}
