/*
========================================================================================
  FabInfo demo firmware
  Arduino Day 2018
  JTL, Stephan Messlinger
========================================================================================
  Hardware: 
    * Wemos D1 mini (ESP-8266EX), 
    * 8 modules 8x8 LED Matrix (MAX7221 driver)
    * LDR (photo-resistor) with 10k pull-down

  Pin map:
    D7 (MOSI)  DIN-MAX7221
    D4         CS -MAX7221
    D5 (CLK)   CLK-MAX7221
    A0         LDR
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
   network connections may not work properly.
========================================================================================
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "FabInfo.h"

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//========================================================================================
// User configuration
//========================================================================================

#define  WIFI_ENABLE   0            // Turn on Wifi (Station mode) and HTTP Server
#define  WIFI_SSID  "FabLab_Guest"  // Wifi Accesspoint SSID to connect to
#define  WIFI_PASS  "Arduino2018"   // Password for Wifi Accesspoint
#define  WIFI_WAIT_CONNECT  0       // Wait until connected to Wifi
#define  HTTP_PORT 80               // Public port for HTTP server 

#define SERIAL_ENABLE  1            // Turn on USB-serial interface
#define SERIAL_BAUDRATE  19200      

#define DEFAULT_TEXT  "Arduino Day 2018 - FabLab Bayreuth"  // Initial display text

//========================================================================================
// FabInfo display
//========================================================================================

FabInfoDisplay display;

//========================================================================================
// WIFI
//========================================================================================

unsigned long ulReqcount;  // TODO: Do we need this?
unsigned long ulReconncount;

ESP8266WebServer http_server ( 8080 );

//========================================================================================

void wifi_init(void)
{
  if (WIFI_ENABLE) {
    ulReqcount = 0;
    ulReconncount = 0;
  
    // Wifi network
    WiFi.mode(WIFI_STA);
    wifi_start();
  
    // HTTP server
    http_server.on ( "/", http_handler );
    http_server.on ( "/favicon.ico", http_handler_favicon );
    http_server.onNotFound ( http_handler );
    http_server.begin();
  }
}


//========================================================================================

void wifi_task(void)
{
  if (WIFI_ENABLE)  {
    const int wifi_status = WiFi.status();
    static int old_wifi_status = 0;

    // if Wifi is not connected, try to restart connection
    if (wifi_status != WL_CONNECTED)   
      wifi_start();

    // If we get a new Wifi connection: Print the new IP address
    if (old_wifi_status != WL_CONNECTED  &&  
           wifi_status == WL_CONNECTED)  {
      IPAddress myAddr = WiFi.localIP();
      Serial.println("addr: " + myAddr.toString());
      display.scroll_text("http://" + myAddr.toString(), 20, 2);
      display.wait_scroll_text();
    }
    old_wifi_status = wifi_status;

    // Note: The low level Wifi tasks are all handled by the underlying
    //   task scheduler. Just make sure to return from the loop function
    //   or call delay() periodically to provide enough execution time. 
  
    // HTTP server task
    http_server.handleClient();
  }
}

//========================================================================================

void wifi_start()
{
  if (WIFI_ENABLE) {
    ulReconncount++;
  
    // Connect to WiFi network
    WiFi.begin( WIFI_SSID, WIFI_PASS );

    if (WIFI_WAIT_CONNECT)  {
      Serial.print("Wifi connect to: " + String(WIFI_SSID));
      display.scroll_text("Wifi connect to: " + String(WIFI_SSID));

      int i = 0;
      while (WiFi.status() != WL_CONNECTED) {   // Wait until connected
        display.task();
        delay(10);
        if (++i % 50 == 0)  Serial.print('.');
      }
      Serial.println();
    }
  
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
    convert_utf8_to_437( text );
    Serial.println("Via http: " + text);
    display.scroll_text( text, 20 );
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
        serial_eval( serial_buf );
        serial_buf[0] = 0;   // reset buffer
        serial_buf_put = 0;
      }
      else if (serial_buf_put < sizeof(serial_buf) - 1)  {  // Store input byte to our buffer
        serial_buf[serial_buf_put++] = c;
        serial_buf[serial_buf_put] = 0;
      }
    }
  }
}

//========================================================================================

void serial_eval( const char * input )
{
  Serial.println("Via serial: " + String(input));
  display.scroll_text( input );
}

//========================================================================================
//========================================================================================

void setup() {
  display.clear();
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



