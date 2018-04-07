/*
========================================================================================
  FabInfo demo firmware
  Arduino Day 2018
  fablab-bayreuth.de
========================================================================================
*/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

#include "FabInfo.h"

//========================================================================================
// User configuration
//========================================================================================

#define DEFAULT_TEXT  "Arduino Day"  // Initial display text

//========================================================================================
// FabInfo display instance

FabInfoDisplay display;

//========================================================================================

void setup() 
{
  Serial.begin(19200);
  Serial.println();
  Serial.println("FabInfo");

  // Static display
  //display.print( DEFAULT_TEXT );
  //display.apply();

  // Special characters
  //display.print( convert_utf8_to_437("ÄÖÜäöüß") );
  //display.apply();

  // Scroll text
  display.scroll_text_async( "Arduino Day 2018 - Fablab Bayreuth", 30, 1 );
}

//========================================================================================

void loop() 
{
    display.scroll_text_step();  
}

//========================================================================================



