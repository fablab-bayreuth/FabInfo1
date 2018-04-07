/*
========================================================================================
  FabInfo demo firmware
  Arduino Day 2018
  JTL, Stephan Messlinger
========================================================================================
  Hardware: 
    * Wemos D1 mini (ESP-8266EX), 
    * 8 modules 8x8 LED Matrix (MAX7219 driver)
    * LDR (photo-resistor) with 1k reference resistor

  Pin map:
    D7 (MOSI)  DISPLAY_DIN
    D4         DISPLAY_CS
    D5 (CLK)   DISPLAY_CLK
    A0         LDR_IN
    D2         LDR_VCC  (LDR from here to LDR_IN)
    D3         LDR_GND  (1k Ohm from here to LDR_IN)
    
========================================================================================
 Board: Wemos D1 Mini
 (Add http://arduino.esp8266.com/stable/package_esp8266com_index.json to your moard manager path)
========================================================================================
 Libraries:
   Adafruit_GFX (download via library manager)
   Max72xxPanel (https://github.com/markruys/arduino-Max72xxPanel)
========================================================================================
 Notes for programming the ESP8266:
   There is a cooperative task scheduler running on this device. The user's loop() function
   is only one among other task. In order to give execution time to other tasks, return from
   the loop() function, or call delay(), eg. delay(0). If the user task blocks for too long,
   network connections may break off.
========================================================================================
*/

//========================================================================================
// User configuration
//========================================================================================

#define  WIFI_ENABLE   1            // Turn on Wifi (Station mode) and HTTP Server
#define  WIFI_SSID  "FabLab_Guest"  // Wifi Accesspoint SSID to connect to
#define  WIFI_PASS  "Arduino2018"   // Password for Wifi Accesspoint
#define  WIFI_AUTO_START    1       // Start Wifi connection automatically at startup
#define  WIFI_WAIT_CONNECT  1       // Wait until connected to Wifi
#define  HTTP_PORT 80               // Public port for HTTP server 

#define SERIAL_ENABLE  1            // Turn on USB-serial interface
#define SERIAL_BAUDRATE  19200      

#define DEFAULT_TEXT  "Arduino Day"  // Initial display text
#define DEFAULT_INTENSITY  7        // Default display intensity

//========================================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "FabInfo.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//========================================================================================
// FabInfo 
//========================================================================================

FabInfoDisplay display;
FabInfoLDR ldr;

//========================================================================================
// WIFI
//========================================================================================

ESP8266WebServer http_server ( HTTP_PORT );

bool wifi_on = WIFI_AUTO_START;

//========================================================================================

void wifi_init(void)
{
  if (WIFI_ENABLE) {  
    
    wifi_start();
  
    // HTTP server
    http_server.on ( "/", http_handler );
    http_server.on ( "/favicon.ico", http_handler_favicon );
    http_server.onNotFound ( http_handler );
    http_server.begin();
  }
}


//========================================================================================

void wifi_stop(void)
{
    WiFi.mode(WIFI_OFF);
}

//========================================================================================

void wifi_start()
{
  if (WIFI_ENABLE  &  wifi_on) {
  
    // Connect to WiFi network
    WiFi.mode(WIFI_STA);  // Act as station (not as access point)

    // Note: Only call Wifi.begin() once
    //  Wifi Connections will be handled automatically in the background.
    if (WiFi.status() == WL_IDLE_STATUS)  
      WiFi.begin( WIFI_SSID, WIFI_PASS );

    if (WIFI_WAIT_CONNECT)  {
      Serial.print("Wifi connect to: " + String(WIFI_SSID));
      display.scroll("Wifi connect to: " + String(WIFI_SSID));

      uint32_t t0 = millis();
      while (WiFi.status() != WL_CONNECTED) {   // Wait until connected
        display.task();  // Update display
        delay(0);        // Serve OS tasks
        if (millis() - t0 >= 500) { 
          Serial.print('.');
          t0 += 500;
        }
      }
      Serial.println();
    }
  
  }
}

//========================================================================================

void wifi_task(void)
{
  if (WIFI_ENABLE)  {
    
    const int wifi_status = WiFi.status();
    static int old_wifi_status = 0;

    // if Wifi is not connected, try to restart connection
    if (wifi_on  &&  (wifi_status != WL_CONNECTED))   
      wifi_start();
    else if (!wifi_on) 
      wifi_stop();

    // If we get a new Wifi connection: Print the new IP address
    if (old_wifi_status != WL_CONNECTED  &&  wifi_status == WL_CONNECTED)  {
      IPAddress myAddr = WiFi.localIP();
      Serial.println("IP: " + myAddr.toString());
      display.scroll("http://" + myAddr.toString(), 30, 2);
      display.scroll_wait();
    }
    old_wifi_status = wifi_status;

    // Note: The low level Wifi tasks are all handled by the underlying
    //   task scheduler. Just make sure to return from the loop function
    //   or call delay() periodically to provide enough execution time. 
  
    // HTTP server task
    if (wifi_on)
        http_server.handleClient();
  }
}

//========================================================================================
// HTTP handlers
//========================================================================================

#include "favicon.h"

void http_handler_favicon(void)
{
  http_print_request_info();
  http_server.send_P( 200, "image/png", favicon_png, sizeof(favicon_png) );
}

//========================================================================================

#include "html.h"

void http_handler(void)
{
  http_print_request_info();
  http_server.send( 200, "text/html", html );

  // The http server conveniently provides all paramters parsed from the url
  String text( http_server.arg("text") );

  if (text.length() > 0)  {
    Serial.print("text: ");
    Serial.print( text );
    Serial.println();
    convert_latin1_to_437( text );
    //convert_utf8_to_437( text );
    Serial.println("Via http: " + text);
    display.scroll( text, 20 );
  }
}

//========================================================================================

void http_print_request_info(void)
{
  Serial.print( http_server.client().remoteIP().toString());
  Serial.print( " " + http_server.uri() );
  Serial.println();
}


//========================================================================================
// Serial port
//========================================================================================

char serial_buf[1024];  // Serial input buffer
size_t serial_buf_put;  // Serial input buffer put index

//========================================================================================
void serial_init(void)
{
  if (SERIAL_ENABLE) {
    Serial.begin(19200);
    //Serial.setDebugOutput(1);
    Serial.println();
    Serial.println("FabInfo"); 
  }
}

//========================================================================================

void serial_task(void)
{
  if (SERIAL_ENABLE) {
    // Receive input characters from serial port and store to internal buffer
    while (Serial.available()) {
      char c = Serial.read();
      if ((c == '\r' || c == '\n') && serial_buf_put > 0)  {  // End command with linebreak
        convert_latin1_to_437( serial_buf );
        //convert_utf8_to_437( serial_buf );
        serial_eval( serial_buf );
        serial_buf[0] = 0;   // reset buffer
        serial_buf_put = 0;
      }
      else if (serial_buf_put < sizeof(serial_buf) - 1)  {  // Store input byte to buffer
        serial_buf[serial_buf_put++] = c;
        serial_buf[serial_buf_put] = 0;
      }
    }
  }
}

//========================================================================================
// Command evaluation helpers

// Test if string starts with a given command
bool is_cmd( const char * str, const char * cmd )
{
   return (strncasecmp(str, cmd, strlen(cmd)) == 0);
}

// Get the nth paramter from the command string, separated by ,;:= or blanks
const char * par_str( const char * str, int n )
{
  static const char delim[] = ",;:= \t";
  static const char *lstr=0, *p=0;
  static int ln=0, i=0;
  if ( str != lstr || n < ln )  { p = str;  i = 0; }  // from start of string
  p += strspn( p, delim );   // skip leading delim
  for ( ; i<n; i++) {  // continue from last position
    p += strcspn( p, delim );  // forward until delim
    p += strspn( p, delim );   // skip delim
  }
  return *p ? p : "";
}

// Get the nth integer paramter from the command string
// Returns 0 if the paramter is not present
long par_int( const char * str, int n )  
{
  return atol( par_str( str, n ) );  
}


//============================================================================================
// Serial com commands

void serial_eval( const char * input )
{
  if (input[0] == '!')  {
    input ++;
    static int scr_speed = 30, scr_repeat = 1;
    if      (is_cmd(input, "scroll")) display.scroll( par_str( input, 1 ), scr_speed, scr_repeat );
    else if (is_cmd(input, "speed"))  scr_speed = par_int( input, 1 );
    else if (is_cmd(input, "repeat")) scr_repeat = par_int( input, 1 );
    else if (is_cmd(input, "text"))   cmd_scroll( par_str( input, 1 ), par_int( input, 2 ), par_int( input, 3 ) );
    else if (is_cmd(input, "print"))  cmd_print( par_str( input, 1 ) );
    else if (is_cmd(input, "stop"))   display.scroll_stop();
    else if (is_cmd(input, "clear"))  display.clear();
    else if (is_cmd(input, "int"))    display.setIntensity( par_int( input, 1 ) );
    else if (is_cmd(input, "addr"))   cmd_ip();
    else if (is_cmd(input, "ip"))     cmd_ip();
    else if (is_cmd(input, "wifi"))   cmd_wifi( par_int( input, 1 ) );
    else if (is_cmd(input, "ldr"))    cmd_ldr();
  }
  else  {
    display.scroll( input );
  }
}

void cmd_scroll( const char * text, int speed, int repeat )
{
  display.scroll( text, speed, repeat );
}

void cmd_print( const char * text )
{ 
  display.clear(); 
  display.print( text ); 
  display.apply();  
}

void cmd_wifi( bool enable ) 
{
  wifi_on = enable;
  if (wifi_on) { display.clear(); display.print("Wifi on"); display.apply(); Serial.println("Wifi on"); }
  else         { display.clear(); display.print("Wifi off"); display.apply(); Serial.println("Wifi off"); }
}

void cmd_ip(void)
{
  String ip = WiFi.localIP().toString();
  Serial.println( ip );
  display.scroll( ip, 30, 1 );
}

void cmd_ldr(void)
{
  uint16_t a = ldr.read();
  Serial.println( a );
  display.clear(); 
  display.print( "LDR: " ); display.print(a);
  display.apply();
}

//========================================================================================
//========================================================================================

void setup() {
  display.clear();
  display.setIntensity( DEFAULT_INTENSITY );
  display.print( DEFAULT_TEXT );
  display.apply();
  serial_init();
  wifi_init();
}

//========================================================================================

void loop() {
  display.task();
  wifi_task();
  serial_task();
}

//========================================================================================



