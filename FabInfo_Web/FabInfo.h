#ifndef FABINFO_H
#define FABINFO_H

//========================================================================================
// Fabinfo support library
// Arduino Day 2018
// fablab-bayreuth.de
//========================================================================================

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>

//========================================================================================
// LED matrix display (8 modules a 8x8 LED)
//========================================================================================

#define FABINFO_DISP_N_HOR     8   // Number of horizontal LED modules
#define FABINFO_DISP_N_VERT    1   // Number of vertical LED modules

#define FABINFO_DISP_DEF_INTENSITY  7   // Default display intensity (0..15)
#define FABINFO_FIX_ADA_CODPAGE  1      // Compensate bug in the Adafruit_GFX default codepage

//========================================================================================
// Pin configuration

#define FABINFO_PIN_DISP_CS    2   // Chip select pin for LED matrix driver
#define FABINFO_PIN_DOUT       7   // Data out (SPI MOSI) for led matrix (for reference only, will be used automatically)  
#define FABINFO_PIN_DOUT       5   // Clock out (SPI CLK) for led matrix (for reference only, will be used automatically)
#define FABINFO_PIN_LDR_IN     A0  // LDR analog input
#define FABINFO_PIN_LDR_VCC    D2  // LDR from here to LDR_IN. To be pulled high.
#define FABINFO_PIN_LDR_GND    D3  // Reference resistor from here to LDR_IN. To be pulled low. 

//========================================================================================
// Fabinfo light dtecting sensor 
//========================================================================================

class FabInfoLDR
{
  public:
    FabInfoLDR(void) 
    {
      init();
    }

    void init(void)
    {
      digitalWrite( FABINFO_PIN_LDR_GND, LOW );
      digitalWrite( FABINFO_PIN_LDR_VCC, HIGH );
      pinMode( FABINFO_PIN_LDR_GND, OUTPUT );
      pinMode( FABINFO_PIN_LDR_VCC, OUTPUT );
    }

    uint16_t read(void)
    {
      return analogRead( FABINFO_PIN_LDR_IN );
    }
};

//========================================================================================
// FabInfo LED matrix display
//========================================================================================

// The Max72xxPanel class is a specialization of an Adafruit Adadfruit_GFX display.
// Note: All gfx functions draw to an internal image buffer. To actually sent the image
//   to the led drivers, we need to call Max72xxPanel:write().

// We use a small wrapper to take care of the Fabinfo display arragement and
// to provide some convenient functions.

class FabInfoDisplay : public Max72xxPanel
{
  public:

    FabInfoDisplay(void) : Max72xxPanel( FABINFO_PIN_DISP_CS, FABINFO_DISP_N_HOR, FABINFO_DISP_N_VERT )
    {
      init();      
    }

    //-----------------------------------------------------------------------------------

    void init(void)
    {
      setIntensity( FABINFO_DISP_DEF_INTENSITY );
      for (int i=0; i<FABINFO_DISP_N_HOR; i++)
        setRotation( i, 1 );
        
      Adafruit_GFX::setTextWrap(0);
      Adafruit_GFX::cp437( FABINFO_FIX_ADA_CODPAGE ? 1 : 0 );
    }

    //-----------------------------------------------------------------------------------
    // Cooperative task handler, to be continuously called by the user
    
    void task(void)
    {
      scroll_step();
    }

    //-----------------------------------------------------------------------------------
    // Clear display

    void clear(void)
    {
      scroll_stop();
      fillScreen(0);
      setCursor(0,0);
      apply();
    }

    //-----------------------------------------------------------------------------------
    // Just an alias for write() (which seems to be a bad naming idea anyway since it is 
    // already used by the gfx library for character printing.

    void apply(void) { Max72xxPanel::write(); }    

    //-----------------------------------------------------------------------------------
    // Override the gfx libraries character write function to gain some additional space 
    // by shrinking whitespaces.
    // The write-function will be called by the various print-functions.

    size_t write(uint8_t c) 
    { 
      // For custom fonts use the existing Adafruit method
      if (Adafruit_GFX::gfxFont)  return Adafruit_GFX::write(c);
        
      // Default font (6x8 px)
      if (c == ' ') {
        Adafruit_GFX::cursor_x += 4;  // Slightly smaller whitespace to fit more charactes on the display
        return 1;
      }
      else {
        return Adafruit_GFX::write(c);
      }
    }

    //-------------------------------------------------------------------------------------
    // Scroll engine
    //-------------------------------------------------------------------------------------

  protected:
    struct Scr {
      char text[1024];        // currently scrolled text
      char new_text[1024];    // next text
      bool text_changed;
      uint32_t repeat;        // number remaining of remaining repetitions, 0=infinite
      uint32_t new_repeat;    // repetitions for next text, 0=infinite
      bool run;               // true if scroll active
      bool start;             // requests start of scroll
      uint32_t cnt;           // Overall counter, increased after scroll completes
      uint32_t new_delay_ms;  // delay in ms between pixel steps for next text
      Scr(void) : text_changed(0), new_repeat(0), run(0), start(0) { } 
    } scr;

  public:
  
    //-------------------------------------------------------------------------------------
    // Set new scroll text. If not already running, scroll starts immediately.
    // If running, new scroll text starts after the current scroll finishes.
    // The user must periodically call scroll_step() to keep the text scrolling.
    void scroll( const char * text, uint16_t speed = 50, uint32_t repeat = 0 )
    {
      strncpy( scr.new_text, text, sizeof(scr.new_text)-1 );
      scr.new_text[sizeof(scr.new_text)-1] = 0;
      scr.text_changed = 1;
      if (speed <= 0)  speed = 50;
      scr.new_delay_ms = 1000 / speed;  // Speed in px per second
      scr.new_repeat = repeat;
      if (!scr.run)  {
        scr.start = 1;    // Start only if not running, since this would reset the scroll
        scroll_step();  // Perform first scroll step immediately 
      }
    }

    void scroll( String text, uint16_t speed = 50, uint32_t repeat = 0 )
    {
      scroll( text.c_str(), speed, repeat );
    }

    //-------------------------------------------------------------------------------------
    // Start new scroll text immediately. If already running, the current scroll is aborted.
    
    void scroll_start(void)  {  scr.start = 1;  }

    //-------------------------------------------------------------------------------------
    // Query if the scroll engine is running 
    
    bool scroll_busy(void) {  return scr.run;  }

    //-------------------------------------------------------------------------------------
    // Stop scolling immediately
    
    void scroll_stop(void)  {  scr.run = 0;  }

    //-------------------------------------------------------------------------------------
    // Wait for rep scroll repetitions to complete, or until scroll stops (rep=0).
    // If scroll is set to infinite repetitions and rep==0, we only wait for one repetition.
    
    void scroll_wait( uint32_t rep = 0 )
    {
      if (!scroll_busy())  return;
      if (rep == 0  &&  scr.repeat == 0)  rep = 1;   // Do not block for inifinte repetitions
      
      if (rep == 0)  {  // Wait until scroll ends
        while (scr.repeat != 0  &&  scr.run)  {
          task();   // keep serving display tasks
          delay(0);  // keep serving OS tasks
        }
      }
      else {  // rep > 0: Wait for rep repetitions
        uint32_t cnt_start = scr.cnt;    // scr.cnt is increased after every complete scroll
        while (scr.cnt - cnt_start < rep  &&  scr.run)  {  
          task();    // keep serving display tasks
          delay(0);  // keep serving OS tasks
        }
      }
    }

    //-------------------------------------------------------------------------------------
    // Scroll text task. To be continuously called by the user.
    // The text speed is automatically kept independently of the call frequency.
    void scroll_step( void )
    {
      // During scroll, we work with internal copies of the changeable paramters
      static uint32_t last_time = 0;  // Time of last step
      static uint32_t delay_ms = 0;   // delay in ms between px steps
      static int pos_px = 0;          // current scroll position in px
      static size_t text_len = 0;   // string length of the current text
    
      if (scr.start)  {  // (re)start requested
        scr.run = 1;
        scr.start = 0;
        delay_ms = scr.new_delay_ms ? scr.new_delay_ms : 50;
        scr.repeat = scr.new_repeat;
        last_time = millis();
        pos_px = 0;
        if (scr.text_changed)  {
          strncpy( scr.text, scr.new_text, sizeof(scr.text)-1 );
          scr.text[sizeof(scr.text)-1] = 0;
          text_len = strlen( scr.text );
          scr.text_changed = 0;
        }
      }
      else if (!scr.run) {
        return;   // scroll inactive, nothing to do
      }
      else if (millis() - last_time < delay_ms)  {
        return;   // step not due, nothing to do
      }
      else {
        last_time += delay_ms;  
      }

      // Perform a text scroll step
      
      // Glyph sizes for Adafruit_GFX default charset
      const int glyph_space = 1;
      const int glyph_width = 5 + glyph_space; // Font glyph width = 5 px + 1 px space
      const int glyph_height = 7;
      const int text_width_px = glyph_width * text_len    // Width of text in px 
                                  + width() - glyph_space;  // (since we want to scroll completly out of the display)
                                  
      if ( pos_px < text_width_px )  {  // Keep on scrolling
        int last_vis_char = pos_px / glyph_width;         // Index of last visible character
        const int last_vis_px_in_char =  pos_px % glyph_width;  // Last visible pixel in last visible character
        int x = width() - 1 - last_vis_px_in_char;   // Start position of last visible character
    
        // Draw all visible character starting with last visible (i.e. the rightmost)
        while ( x + glyph_width - glyph_space >= 0  &&  last_vis_char >= 0 ) {
          if ( last_vis_char < text_len )  {
            const char c = scr.text[last_vis_char];
            Adafruit_GFX::drawChar( x, 0, c, 1, 0, 1 );
          }
          last_vis_char--;  // next character to the left
          x -= glyph_width;     // draw position on character width to the left
        }
    
        apply();
        pos_px++;
      }
      else {  // Scroll complete
        scr.cnt++;
        if (scr.text_changed)     // new text available? 
          scr.start = 1;          // --> keep running and load new text at next iteration
        else if ((scr.repeat == 0) ||   // infinite repetitions?
                  (--scr.repeat > 0))   // or more repetitions?
          pos_px = 0;                   // --> keep running and restart scroll
        else
          scr.run = 0;       
      }
    }

    //-------------------------------------------------------------------------------------
 
};  

// End class FabInfoDisplay
//========================================================================================


//========================================================================================
// Some convenient codepage conversions
//========================================================================================

// Codepage conversion table ISO-8859-1 (Latin-1) to 437 (used by Adafruit_GFX)
// Identical or not available entries are marked with zero.

static uint8_t codepage_8859_1_to_437[256] = { 
// ..0   ..1   ..2   ..3   ..4   ..5   ..6   ..7   ..8   ..9   ..A   ..B   ..C   ..D   ..E   ..F  
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 1..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 2..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 3..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 4..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 5..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 6..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 7..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 8..
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 9..
  0xFF, 0xAD, 0x9B, 0x9C, 0x00, 0x9D, 0x00, 0x15, 0x00, 0x00, 0xA6, 0xAE, 0xAA, 0x00, 0x00, 0x00,  // A..
  0xF8, 0xF1, 0xFD, 0x00, 0x00, 0xE6, 0x14, 0xFA, 0x00, 0x00, 0xA7, 0xAF, 0xAC, 0xAB, 0x00, 0xA8,  // B..
  0x00, 0x00, 0x00, 0x00, 0x8E, 0x8F, 0x92, 0x80, 0x00, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // C..
  0x00, 0xA5, 0x00, 0x00, 0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x00, 0x00, 0xE1,  // D..
  0x85, 0xA0, 0x83, 0x00, 0x84, 0x86, 0x91, 0x87, 0x8A, 0x82, 0x88, 0x89, 0x8D, 0xA1, 0x8C, 0x8B,  // E..
  0x00, 0xA4, 0x95, 0xA2, 0x93, 0x00, 0x94, 0xF6, 0x00, 0x97, 0xA3, 0x96, 0x81, 0x00, 0x00, 0x98   // F..
};


//========================================================================================
// Convert character codes from ISO 8859-1 (latin-1) to 437 (used by Adafruit_GFX)

uint8_t convert_latin1_to_437( uint8_t c )
{
  const char c1 = codepage_8859_1_to_437[c];
  return c1 != 0 ? c1 : c;   // Null entries are unchanged.      
}


// Convert complete C-string inplace
char* convert_latin1_to_437( char* str )
{
  while (*str) {
    *str = convert_latin1_to_437(*str);
    str++;
  }
  return str;
}

//========================================================================================
// Convert complete Arduino-string inplace

String& convert_latin1_to_437( String& str )
{
  for (int i=0; i<str.length(); i++)
    str[i] = convert_latin1_to_437( str[i] );
  return str;
}

//========================================================================================
// Convert character codes from UTF-8 ISO 8859-1.
// Unsupported (>0xFF) or skipped chars are returned as zeros.

uint8_t convert_utf8_to_latin1( uint8_t c )
{
  static uint8_t utf8_c1;  

  if (c < 0x80)  {   // single byte sequence = ASCII
    utf8_c1 = 0; 
    return c; 
  }
  else {
    const uint8_t c1 = utf8_c1;   // previous byte
    utf8_c1 = c;
    switch (c1)  {
      case 0xC2: return (c | 0x80);  break;   // 11000010 10xxxxxx --> ASCII 10xxxxxxxx
      case 0xC3: return (c | 0xC0);  break;   // 11000011 10xxxxxx --> ASCII 11xxxxxxxx
      // Higher codes not supported
      // We could handle the Euro symbol here as special case 
    }
    return 0;
  }
}


uint8_t convert_utf8_to_437( uint8_t c )
{
  c = convert_utf8_to_latin1(c);
  c = convert_latin1_to_437(c);
  return c;
}

//========================================================================================
// Convert complete C-String inplace
// Note: This may shorten the string length

char* convert_utf8_to_437( char* str )
{
  char * sput = str;  // string put pointer
  while (*str)  {
    const char c = convert_utf8_to_437( *str );
    if (c!=0)  *sput++ = c;  // write back to string, skip zeros
    str++;
  }
  *sput = 0;  // termination
  return str;
}

//========================================================================================
// Convert complete Arduino-String inplace
// Note: This may shorten the string length

String& convert_utf8_to_437( String& str )
{
  int j = 0;  // String put index
  for (int i=0; i<str.length(); i++)  {
    const char c = convert_utf8_to_437( str[i] );
    if (c!=0)   str[j++] = c;  // write back to string, skip zeros
  }
  str.remove(j);  // crop after index j
  return str;
}

//========================================================================================
#endif // FABINFO_H


