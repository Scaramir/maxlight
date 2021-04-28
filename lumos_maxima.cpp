// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)
//
// Just use a WS2812b, a CH340c board and a compatible power supply.
// For led strips consuming <= 15W, solder the 5v wire of the usb cable directly
//  to the board and make use of power share (BIOS and Motherboard dependable)
// Costs: max.: 45€ for 5m 


#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the strip?
#define LED_PIN    6

// How many LEDs are attached to the Arduino?
#define LED_COUNT 30 

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

const int data_amount = 3 * LED_COUNT +2;
uint8_t buffer[data_amount]; //+2, 'cause "ml" is my signature
int index = 0;
// setup() function -- runs once at startup --------------------------------
void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.begin(9600);      // Check max. value for CH430 (~2M)
  Serial.print("ml");      // Send string to host //TODO: let host wait for it
}


// loop() function -- runs repeatedly as long as board is on ---------------
void loop() {
    if (Serial.available() > 0) {
        buffer[index++] = Serial.read();
        if (index >= data_amount) {
            if (buffer[0] == 'm' && buffer[1] == 'l') {
                for(uint16_t i=0; i < LED_COUNT; i++){ 
                    int led_index = i*3 + 2;           //+2, 'cause "ml" is the header
                    strip.setPixelColor(i, strip.Color(buffer[led_index], buffer[led_index+1], buffer[led_index+2]));
                }
                strip.show();
            }
        }
    }

}