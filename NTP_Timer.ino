/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov

 This code is in the public domain.

 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "SSID";  //  your network SSID (name)
char pass[] = "PASSWORD";       // your network password


unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

#include "Timer.h"
Timer t;
int restEvent;
int ntpEvent;
int toggleEvent;

unsigned int h;
unsigned int m;

char state = 0;
int buttonState = 0;         // variable for reading the pushbutton status

const int RELAYPIN = D1;

#include <aREST.h>
#include <aREST_UI.h>
// Create aREST instance
aREST_UI rest = aREST_UI();
// The port to listen for incoming TCP connections 
#define LISTEN_PORT           80
// Create an instance of the server
WiFiServer server(LISTEN_PORT);


void setup()
{
  pinMode(RELAYPIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  rest.title("LIGHT");
  // Create button to control pin 5
  rest.button(RELAYPIN, "RELAY");
  // Give name and ID to device
  rest.set_id("1");
  rest.set_name("esp8266");
  // Start the server
  server.begin();
  Serial.println("REST server started");

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  
  restEvent = t.every(1000, restHandler);
  Serial.print("REST event started id=");
  Serial.println(restEvent);

  ntpEvent = t.every(10000, getNTP);
  Serial.print("ntp event started id=");
  Serial.println(ntpEvent);
  
}

void loop()
{
  t.update();
}

void restHandler()
{
  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
}

void getNTP()
{
  
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 

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
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    h = (epoch  % 86400L) / 3600;
    Serial.print(h); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    m = (epoch  % 3600) / 60;
    Serial.print(m); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
    t.stop(ntpEvent);
    Serial.print("ntp event stopped id=");
    Serial.println(ntpEvent);

    toggleEvent = t.every(60000, timeCheck);
    Serial.print("On/OFF event started id=");
    Serial.println(toggleEvent);
  
  }
}

void timeCheck()
{
  Serial.println("Checking...");
  m++;
  if(m == 60){
    m = 0;
    h++;
    if(h == 24) h = 0;
  }
  Serial.print("h = ");
  Serial.println(h);
  Serial.print("m = ");
  Serial.println(m);

  if(h == 18 && m == 30){
    state = 1;
    Serial.println("ON");
    updateIO();
  }

  if(h == 6 && m == 30){
    state = 0;
    Serial.println("OFF");
    updateIO();
  }
}

void updateIO() {
  if (state == 1) {
    digitalWrite(RELAYPIN, HIGH);
  } else {
    state = 0;
    digitalWrite(RELAYPIN, LOW);
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
