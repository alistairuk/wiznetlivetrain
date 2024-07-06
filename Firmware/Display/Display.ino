/*
 * 
 * WizNet Live Train - Matrix display controller firmware
 * 
 * Alistair MacDonald 2024
 * 
 */

#include <LEDMatrixDriver.hpp>
#include "font6x8.h"

// Settings

#define LEDMATRIX_CS_PIN 10
#define MATRIXWIDTH 8

// Note the DIN goes to D11 and CLK to D13

LEDMatrixDriver lmd(MATRIXWIDTH, LEDMATRIX_CS_PIN);

// A function to update the display
void drawAll( String inString, int inOffset = 0 ) {
  int pos, offsetcol, val;

  // Loop though the colums of the display
  for( int col = 0; col < (MATRIXWIDTH*8); col++ ) {
    // Work out the charicter offset that we are sending
    offsetcol = col + inOffset;
    pos = (( inString[(offsetcol/6)%inString.length()]-32 )*6) + (offsetcol%6);
    // Loop though the rows of the display
    for( int row = 0; row < 8; row++ ) {
      // Work out the pixel state (checking for a space)
      val = ssd1306xled_font6x8[pos] & (0x01<<row);
      // Send the new pixel state to the deisplay driver
      lmd.setPixel( col, row, val );
    }
  }
}

// Set up the device
void setup() {
  // init the serial port
  Serial.begin(115200);
  Serial.setTimeout(80);
  // init the display
  lmd.setEnabled(true);
  lmd.setIntensity(0);   // 0 = low, 10 = high
  lmd.display();
}

// Globals
String lineBuffer = "Hello World!    ";
unsigned long timeOffset = 0;

// The main loop
void loop() {
  unsigned long now = millis();

  // Cherck for any new text
  if ( Serial.available() ) {
    // Rread and tidy the new sting
    lineBuffer = Serial.readString();
    lineBuffer.trim();
    // Padd out a short string
    if ( lineBuffer.length() <= 10 ) {
      // If is is a short string pad to 11
      lineBuffer = (lineBuffer+"           ").substring(0, 11);
    }
    else {
      // If it is long (more than 11 chars) then pad with a gap
      lineBuffer += "      ";
      // Reset the scrool position
      timeOffset = now;
    }
  }

  // Update the display
  if ( lineBuffer.length() <= 12 ) {
    drawAll( lineBuffer );
  }
  else {
    unsigned long offset = ( ( now - timeOffset ) / 128 ) % ( lineBuffer.length() * 6 );
    drawAll( lineBuffer, offset );
  }
  lmd.display();

  // Save a little power
  delay(40);
}
