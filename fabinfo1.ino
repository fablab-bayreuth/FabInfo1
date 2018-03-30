#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

//#include "LedMatrix.h"
#include <ESP8266WiFi.h>

// WiFi SSID and Password
const char* ssid = "FabLab_Guest";
const char* password = "Arduino2018";
unsigned long ulReqcount;
unsigned long ulReconncount;
int x = 0;
WiFiServer server(80);
String dtext="http://";
String outtext="Arduino Day 2018 - FabLab Bayreuth";

// LED matrix Settings
int pinCS = 2; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )
int numberOfHorizontalDisplays = 8;
int numberOfVerticalDisplays = 1;
int wait = 40; // In milliseconds
int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);


void urldecode(String &input){
  input.replace("+", " ");
  while(input.indexOf('%') > -1){
    int start = input.indexOf('%');
    String src = input.substring(start, start + 3);
    src.toUpperCase();
    //   Serial.print("SRC:"+src);
    unsigned char char1 = src.charAt(1);
    unsigned char char2 = src.charAt(2);
    //   Serial.print(char1); Serial.print(":"); Serial.print(char2); Serial.print(":");
 // normalize A-F   
    if (char1 >= 'A') {char1 = char1 - 7; };
    if (char2 >= 'A') {char2 = char2 - 7; };   
    unsigned char dest = (char1 - 48) * 16 -48 + char2;
    //   Serial.print(char1); Serial.print(":"); Serial.print(char2); Serial.print(":"); Serial.println(dest);
    String destString = String((char)dest);
    input.replace(src, destString); 
  }
// we have latin1 and want cp437  
// transpose äöüÄÖÜß
  input.replace((char)0xE4, (char)0x84);
  input.replace((char)0xF6, (char)0x94);
  input.replace((char)0xFC, (char)0x81);
  input.replace((char)0xC4, (char)0x8E);
  input.replace((char)0xD6, (char)0x99);
  input.replace((char)0xDC, (char)0x9A);
  input.replace((char)0xDF, (char)0xE1);
}



void scrolltext(String scrt, int clk) {
  for ( int i = 0 ; i < width * scrt.length() + matrix.width() - spacer; i++ ) {
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2; // center the text vertically
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < scrt.length() ) {
        matrix.drawChar(x, y, scrt[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write(); // Send bitmap to display
    delay(clk);
   }
}



void WiFiStart()
{
  ulReconncount++;
  
 // Connect to WiFi network
  scrolltext("connect to: " + (String)ssid, 30);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  // Start the server
  server.begin();

  // Print the IP address
  IPAddress myAddr = WiFi.localIP();
  String ipAddress = (String)myAddr[0] + (String)"." + (String)myAddr[1] + (String)"." + 
   (String)myAddr[2] + (String)"." + (String)myAddr[3];
   
  scrolltext((String)"http://"+ ipAddress,20);
  
   dtext = dtext + ipAddress;
}


void setup() {
  matrix.setIntensity(15); // Use a value between 0 and 15 for brightness
  matrix.setRotation(0, 1);
  matrix.setRotation(1, 1);
  matrix.setRotation(2, 1);
  matrix.setRotation(3, 1);
  matrix.setRotation(4, 1);
  matrix.setRotation(5, 1);
  matrix.setRotation(6, 1);
  matrix.setRotation(7, 1);
  
  ulReqcount=0; 
  ulReconncount=0;
    
  WiFi.mode(WIFI_STA);
  WiFiStart();
}




void loop() {
  matrix.fillScreen(LOW);
 // scrolltext("LED MATRIX by JTL",20);      
  scrolltext(outtext,20);

  x=x+1;
  if (x == 1) {
     scrolltext(dtext,30); 
  }
    // check if WLAN is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiStart();
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) 
  {
    return;
  }
  // Wait until the client sends some data
  // A new client?
  unsigned long ultimeout = millis()+250;
  while(!client.available() && (millis()<ultimeout) )
  {
    delay(1);
  }
  if(millis()>ultimeout) 
  { 
  // Client connection time-out!
    return; 
  }
  
  // Read the first line of the request
  String sRequest = client.readStringUntil('\r');
  client.flush();
  
  // get path; end of path is either space or ?
  // Syntax is e.g. GET /?pin=MOTOR1STOP HTTP/1.1
  String sPath="",sParam="", sCmd="";
  String sGetstart="GET ";
  int iStart,iEndSpace,iEndQuest;
  iStart = sRequest.indexOf(sGetstart);
  if (iStart>=0)
  {
    iStart+=+sGetstart.length();
    iEndSpace = sRequest.indexOf(" ",iStart);
    iEndQuest = sRequest.indexOf("?",iStart);
    
    // are there parameters?
    if(iEndSpace>0)
    {
      if(iEndQuest>0)
      {
        // there are parameters
        sPath  = sRequest.substring(iStart,iEndQuest);
        sParam = sRequest.substring(iEndQuest,iEndSpace);
        //Parameters!
      }
      else
      {
        // NO parameters
        sPath  = sRequest.substring(iStart,iEndSpace);
        // No Parameters
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////
  // output parameters to serial, you may connect e.g. an Arduino and react on it
  ///////////////////////////////////////////////////////////////////////////////
  if(sParam.length()>0)
  {
    int iEqu=sParam.indexOf("=");
    if(iEqu>=0)
    {
      sCmd = sParam.substring(iEqu+1,sParam.length());
    }
  }
  if (sCmd.length() > 0) {
      urldecode(sCmd);
      outtext=sCmd;
  }
  
  ///////////////////////////
  // format the html response
  ///////////////////////////
  String sResponse,sHeader;
 
    sResponse="<style> input[type=text]{ -webkit-appearance: none; -moz-appearance: none; display: block; margin: 0; width: 100%; height: 40px; line-height: 40px; font-size: 17px; border: 1px solid #bbb; }</style><form action=\"/\" method=\"GET\">Text der FabInfo-Anzeige!<input type=\"text\" name=\"text\"><input type=\"submit\" value=\"Text Anzeigen!\"><form>";
    
    sHeader  = "HTTP/1.1 200 OK\r\n";
    sHeader += "Content-Length: ";
    sHeader += sResponse.length();
    sHeader += "\r\n";
    sHeader += "Content-Type: text/html\r\n";
    sHeader += "Connection: close\r\n";
    sHeader += "\r\n";
  
    client.print(sHeader);
    client.print(sResponse);
  
  // and stop the client
     client.stop();
  // Client disonnected
}
