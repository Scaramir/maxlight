// "NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN *should* pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)"
//
// Just use a WS2812b, an arduino and a compatible power supply.
// For led strips consuming <= 15W, you can solder the 5v wire of the usb cable directly
//  to the board and make use of power share (BIOS and Motherboard dependable)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h>   // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the strip?
#define LED_PIN    6

// How many LEDs are attached to the Arduino?
#define LED_COUNT 150 

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

const int buffer_size = 5;
uint8_t buffer[buffer_size];         // +2, 'cause "mo" is the header
int index = 0;
int min_bright = 0;
//int parts = 7;


// setup() function -- runs once at startup --------------------------------
void setup() {
#if 1
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
  #endif
  // END of Trinket-specific code.
#endif
  strip.begin();                     // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.setBrightness(255);          // Set BRIGHTNESS to about 1/5 (max = 255)
  for (uint16_t i = 0; i < LED_COUNT; ++i) {
      strip.setPixelColor(i, strip.Color(150, 0, 50));
      strip.show();
      delay(10);
  }
  Serial.begin(2000000);             // Check baud rate value for your serial connection
  Serial.print("ml");                // Send string to host 
}


// loop() function -- runs repeatedly as long as board is on ---------------
void loop() {

    if (Serial.available() > 0) {
        buffer[index++] = Serial.read();
        if (index >= buffer_size) {
          index = 0;
            // buffer must begin with the correct header bytes
            if (buffer[0] == 'm' && buffer[1] == 'o') {
                min_bright = buffer[2]+buffer[3]+buffer[4] - 150;
                if (min_bright <= 1) {
                    if ( buffer[2] < 50 && buffer[3] < 50 && buffer[4] < 50)
                        strip.fill(strip.Color(buffer[2] << 1, buffer[3] << 1, buffer[4] << 1), 0, LED_COUNT/*i * LED_COUNT/parts, LED_COUNT/parts*/);
                    else 
                        strip.fill(strip.Color(buffer[2] + (buffer[2]>>1), buffer[3] + (buffer[3]>>1), buffer[4] + (buffer[4]>>1)), 0, LED_COUNT/*i * LED_COUNT/parts, LED_COUNT/parts*/);     
                } 
                else
                    strip.fill(strip.Color(buffer[2], buffer[3], buffer[4]), 0, LED_COUNT);
                
                strip.show();
            }
        }
    }

}
