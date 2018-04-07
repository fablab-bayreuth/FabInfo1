/*
========================================================================================
  FabInfo demo firmware
  Arduino Day 2018
  fablab-bayreuth.de
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

#define  WIFI_SSID  "FabLab_Guest"  // Wifi Accesspoint SSID to connect to
#define  WIFI_PASS  "Arduino2018"   // Password for Wifi Accesspoint
#define  HTTP_PORT 80               // Public port for HTTP server 

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

//========================================================================================

void wifi_init(void)
{    
    wifi_start();
  
    // HTTP server
    http_server.on ( "/", http_handler );
    http_server.on ( "/favicon.ico", http_handler_favicon );
    http_server.onNotFound ( http_handler );
    http_server.begin();
}


//========================================================================================

void wifi_stop(void)
{
    WiFi.mode(WIFI_OFF);
}

//========================================================================================

void wifi_start()
{
    // Connect to WiFi network
    WiFi.mode(WIFI_STA);  // Act as station (not as access point)

    // Note: Only call Wifi.begin() once.
    //  Wifi Connections will be handled automatically in the background.
    if (WiFi.status() == WL_IDLE_STATUS)  
      WiFi.begin( WIFI_SSID, WIFI_PASS );

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

//========================================================================================

void wifi_task(void)
{
    const int wifi_status = WiFi.status();
    static int old_wifi_status = 0;

    // if Wifi is not connected, try to restart connection
    if (wifi_status != WL_CONNECTED)
      wifi_start();

    // If we get a new Wifi connection: Print the new IP address
    if (old_wifi_status != WL_CONNECTED  &&  wifi_status == WL_CONNECTED)  {
      IPAddress myAddr = WiFi.localIP();
      Serial.println("IP: " + myAddr.toString());
      display.scroll("http://" + myAddr.toString(), 30, 2);
      //display.scroll_wait();
    }
    old_wifi_status = wifi_status;

    // Note: The low level Wifi tasks are all handled by the underlying
    //   task scheduler. Just make sure to return from the loop function
    //   or call delay() periodically to provide enough execution time. 
  
    // HTTP server task
    http_server.handleClient();
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
  
  // The http server conveniently provides all parameters parsed from the url
  String text = http_server.arg("text");
  long speed = http_server.arg("speed").toInt();
  long repeat = http_server.arg("repeat").toInt();
  if (speed <= 0)  speed = 50;
  if (repeat < 0)  repeat = 0;

  // Resend the form filled with the received paramters
  // At the first request, these will probably be empty
  String html_filled(html);
  html_filled.replace("%TEXT%", text);
  html_filled.replace("%SPEED%", String(speed) );
  html_filled.replace("%REPEAT%", String(repeat) );
  http_server.send( 200, "text/html", html_filled );

  if (text.length() > 0)  {
    convert_latin1_to_437( text );
    //convert_utf8_to_437( text );
    Serial.println("Text via http: " + text);
    Serial.println("Speed: " + String(speed));
    Serial.println("Repeat: " + String(repeat));
    display.scroll( text, speed, repeat );
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

void serial_init(void)
{
    Serial.begin(19200);
    //Serial.setDebugOutput(1);
    Serial.println();
    Serial.println("FabInfo"); 
}

//========================================================================================
//========================================================================================

void setup() {
  display.clear();
  display.setIntensity( DEFAULT_INTENSITY );
  display.scroll( DEFAULT_TEXT );
  display.apply();
  serial_init();
  wifi_init();
}

//========================================================================================

void loop() {
  display.task();
  wifi_task();
}

//========================================================================================
//========================================================================================



