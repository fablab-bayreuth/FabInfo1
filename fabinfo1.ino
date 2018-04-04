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
   Adafruit_GFX
   Max72xxPanel (XXX Link? XXX)
========================================================================================
 Notes for programming the ESP8266:
   There is a cooperative task scheduler running on this device. The user's loop() function
   is only one among other task. In order to give execution time to other tasks, return from
   the loop() function, or call delay(), eg. delay(0). If the user task blocks for too long,
   network connections may not work properly.
========================================================================================
*/

//========================================================================================
// User configuration
//========================================================================================

#define  WIFI_ENABLE   1                  // Turn on Wifi (Station mode) and HTTP Server
//#define  WIFI_SSID  "FabLab_Guest"  // Wifi Accesspoint SSID to connect to
//#define  WIFI_PASS  "Arduino2018"   // Password for Wifi Accesspoint
#define  WIFI_SSID    "AndroidSM"
#define  WIFI_PASS    "frogggnn"
#define  HTTP_PORT 80                 // Public port for HTTP server 

#define SERIAL_ENABLE  1                 // Turn on USB-serial connection
#define SERIAL_BAUDRATE  19200      

const String DEFAULT_TEXT = "Arduino Day 2018 - FabLab Bayreuth";  // Initial display text

//========================================================================================
// Pin configuration
//========================================================================================

#define PIN_LED_CS    2   // Chip select pin for led matrix
#define PIN_LED_DOUT  7   // Data out (SPI MOSI) for led matrix (for reference only, will be used automatically)  
#define PIN_LED_CLK   5   // Clock out (SPI CLK) for led matrix (for reference only, will be used automatically)
#define PIN_LDR       A0  // LDR analog voltage

//========================================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

//#include "LedMatrix.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//========================================================================================
// LED matrix display (8 modules a 8x8 LED)
//========================================================================================

#define LED_MATRIX_N_HOR   8   // Number of horizontal LED modules
#define LED_MATRIX_N_VERT  1   // Number of vertical LED modules

Max72xxPanel led_matrix( PIN_LED_CS, LED_MATRIX_N_HOR, LED_MATRIX_N_VERT );

String outtext = DEFAULT_TEXT;
int wait = 40; // In milliseconds
int spacer = 1;
int width = 5 + spacer; // The font width is 5 pixels

//========================================================================================

void display_init(void)
{
  led_matrix.setIntensity(15); // Use a value between 0 and 15 for brightness
  for (int i=0; i<LED_MATRIX_N_HOR; i++)  {
    led_matrix.setRotation(i, 1);
  }
}

//========================================================================================

// TODO: make this non-blocking

void display_scrolltext(String scrt, int clk=40) {
  for ( int i = 0 ; i < width * scrt.length() + led_matrix.width() - spacer; i++ ) {
    int letter = i / width;
    int x = (led_matrix.width() - 1) - i % width;
    int y = (led_matrix.height() - 8) / 2; // center the text vertically
    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < scrt.length() ) {
        led_matrix.drawChar(x, y, scrt[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    led_matrix.write(); // Send bitmap to display
    delay(clk);
  }
}

//========================================================================================

void display_clear(void)
{
    led_matrix.fillScreen(0);
}

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

void wifi_start()
{
  if (WIFI_ENABLE) {
    ulReconncount++;
  
    // Connect to WiFi network
    Serial.print("WIFI Connect to: " + (String)WIFI_SSID);
    display_scrolltext("Connect to: " + (String)WIFI_SSID, 30);
    WiFi.begin( WIFI_SSID, WIFI_PASS );
  
    // Wait until connected
    // TODO: Make this non-blocking!
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print('.');
    }
    Serial.println();
  
    // Print the IP address
    IPAddress myAddr = WiFi.localIP();
    Serial.println("IP address: " + myAddr.toString());
    display_scrolltext("http://" + myAddr.toString(), 20);
  }
}

//========================================================================================
// favicon handler

#include "favicon.h"

void http_handler_favicon(void)
{
  http_print_request_info();
  http_server.send_P( 200, "image/png", favicon_png, sizeof(favicon_png) );
}

//========================================================================================
// HTML page handler

#include "html.h"


void http_handler(void)
{
  http_print_request_info();
  http_server.send( 200, "text/html", html );

  // The http server conveniently provides all paramters parsed from the url
  String text( http_server.arg("text") );

  if (text.length() > 0)  {
    Serial.print("text: ");
    Serial.print( http_server.arg("text") );
    Serial.println();
    convert_codepage( text );
    display_scrolltext( text, 20 );
  }
}

//========================================================================================
// Helper functions

void http_print_request_info(void)
{
  Serial.print( http_server.client().remoteIP().toString());
  Serial.print( " " + http_server.uri() );
  Serial.println();
}

void convert_codepage( String &str )
{
  // we have latin1 and want cp437
  // transpose äöüÄÖÜß
  str.replace((char)0xE4, (char)0x84);
  str.replace((char)0xF6, (char)0x94);
  str.replace((char)0xFC, (char)0x81);
  str.replace((char)0xC4, (char)0x8E);
  str.replace((char)0xD6, (char)0x99);
  str.replace((char)0xDC, (char)0x9A);
  str.replace((char)0xDF, (char)0xE1);
}


//========================================================================================

void wifi_task(void)
{
  if (WIFI_ENABLE)  {
    // if WIFI is not connected, try to restart connection
    if (WiFi.status() != WL_CONNECTED)  {
      wifi_start();
    }
  
    // HTTP server
    http_server.handleClient();
  }
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
    Serial.println("Hello, World!"); 
  }
}

//========================================================================================

void serial_task(void)
{
  if (SERIAL_ENABLE) {
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
  display_scrolltext( input );
}

//========================================================================================

void setup() {
  display_init();
  serial_init();
  wifi_init();
}

//========================================================================================

void loop() {
  display_clear();
  //  scrolltext(outtext,20);
  wifi_task();
  serial_task();
}

//========================================================================================



