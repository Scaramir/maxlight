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
// Costs: max.: 45€ for 5m

#include <Adafruit_NeoPixel.h> // ignore this error in vscode. The Arduino IDE will compile it and upload it to the board.
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the strip?
#define LED_PIN 6

// How many LEDs are attached to the Arduino?
#define LED_COUNT 150

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:

const int buffer_size = 8;
uint8_t buffer[buffer_size]; //+2, 'cause "mo" is the header
uint8_t buffer_old[buffer_size];
uint8_t buffer_tmp[buffer_size];
int index = 0;
int min_bright = 0;
bool usb_usage = 0;

// setup() function -- runs once at startup --------------------------------
void setup()
{
#if 1
// These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
// Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif
    // END of Trinket-specific code.
#endif

    strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    strip.setBrightness(255);

    strip.fill(strip.Color(255, 0, 0), 0, 6);
    for (uint16_t i = 5; i < LED_COUNT; ++i)
    {
        strip.setPixelColor(i, strip.Color(100, 1, 255));
        strip.setPixelColor(i - 1, strip.Color(150, 3, 200));
        strip.setPixelColor(i - 2, strip.Color(200, 7, 180));
        strip.setPixelColor(i - 3, strip.Color(255, 9, 100));
        strip.setPixelColor(i - 4, strip.Color(255, 11, 70));
        strip.setPixelColor(i - 5, strip.Color(255, 13, 0));
        strip.show();
        delay(15);
    }

    for (uint16_t i = 0; i <= 255; ++i)
    {
        strip.fill(strip.Color(255, i / 1.5, 2), 0, LED_COUNT);
        strip.show();
        delay(5);
    }

    Serial.begin(2000000); // Check max. value for your usb-port first (C340: 2M)
    Serial.print("ml");    // Send string to host
    strip.fill((strip.Color(0, 0, 0), 0, LED_COUNT));
    strip.clear();
    strip.show();

    while (index <= buffer_size)
    {
        if (Serial.available() > 0)
        {
            buffer[index++] = Serial.read();
            if (index == buffer_size)
            {
                index = 0;
                if (buffer[0] == 'm' && buffer[1] == 'o')
                {
                    usb_usage = 1;
                    break;
                }
                if (buffer[0] == 'r' && buffer[1] == 'b')
                {
                    usb_usage = 2;
                    break;
                }
            }
        }
    }
}

// loop() function -- runs repeatedly as long as board is on ---------------
void loop()
{
    if (usb_usage == 1)
    {
        maxlight(buffer);
    }
    if (usb_usage == 2)
    {
        rainbow(40);
    }
}

void maxlight(uint8_t buffer[8])
{
    if (Serial.available() > 0)
    {
        buffer[index++] = Serial.read();
        if (index >= buffer_size)
        {
            index = 0;
            if (buffer[0] == 'm' && buffer[1] == 'o')
            {
                strip.fill(strip.Color(0, 0, 0), 0, LED_COUNT);
                // Left side:
                strip.fill(strip.Color(buffer[2], buffer[3], buffer[4]), 0, 37);
                // Right side:
                strip.fill(strip.Color(buffer[5], buffer[6], buffer[7]), 37, 115);
                strip.fill(strip.Color(buffer[5], buffer[6], buffer[7]), 117, LED_COUNT - 116);
                // Turn two leds off, 'cause they'd shine right into my eyes:
                strip.setPixelColor(115, strip.Color(0, 0, 0));
                strip.setPixelColor(116, strip.Color(0, 0, 0));
                strip.show();
            }
        }
    }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait)
{
    // Hue of first pixel runs 5 complete loops through the color wheel.
    // Color wheel has a range of 65536 but it's OK if we roll over, so
    // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
    // means we'll make 5*65536/256 = 1280 passes through this outer loop:
    for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 100)
    { // 256
        for (int i = 0; i < strip.numPixels(); i++)
        { // For each pixel in strip...
            if (Serial.available() > 0)
            {
                return;
            }
            if (i == 115 || i == 116)
            {
                strip.setPixelColor(i, strip.Color(0, 0, 0));
                continue;
            }
            // Offset pixel hue by an amount to make one full revolution of the
            // color wheel (range of 65536) along the length of the strip
            // (strip.numPixels() steps):
            int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
            // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
            // optionally add saturation and value (brightness) (each 0 to 255).
            // Here we're using just the single-argument hue variant. The result
            // is passed through strip.gamma32() to provide 'truer' colors
            // before assigning to each pixel:
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
    }
}