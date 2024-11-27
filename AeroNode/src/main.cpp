#include <Arduino.h>
#include <Preferences.h>
#include <ArtnetEther.h>
#include "librmt/esp32_digital_led_lib.h"
#include "libfast/crgbw.h"

// 
// CONFIGURATION
//

// Uncomment to set
#define _STRIP_POSITION 1

const int PIXEL_COUNT = 660;
const int PIN_LED = 32;

//
// VARIABLES
//

// Internal LEDS
bool dirty = false;
strand_t* strip;
pixelColor_t* buffer;

// Internals
int STRIP_POSITION;
Preferences preferences;

// Artnet stuff
ArtnetReceiver artnet;
IPAddress ip(2, 0, 1, 0);
uint8_t mac[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
int universeStart = 0;
int universeCount = 0;

//
// LEDS
//
void clear() {
  memset(buffer, 0, PIXEL_COUNT * sizeof(pixelColor_t));
  dirty = true;
}

void all(pixelColor_t color) {
  for (int i = 0; i < PIXEL_COUNT; i++) buffer[i] = color;
  dirty = true;
}

void all(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  all(pixelFromRGBW(r, g, b, w));
}


//
// SETUP
//
void setup() {

  // Init Serial
  Serial.begin(115200);
  delay(1000);

  // ASCII Art Logo
  Serial.println("                                          ");
  Serial.println("                                          ");
  Serial.println(" ▗▄▖ ▗▞▀▚▖ ▄▄▄ ▄▄▄  ▗▖  ▗▖ ▄▄▄     ▐▌▗▞▀▚▖");
  Serial.println("▐▌ ▐▌▐▛▀▀▘█   █   █ ▐▛▚▖▐▌█   █    ▐▌▐▛▀▀▘");
  Serial.println("▐▛▀▜▌▝▚▄▄▖█   ▀▄▄▄▀ ▐▌ ▝▜▌▀▄▄▄▀ ▗▞▀▜▌▝▚▄▄▖");
  Serial.println("▐▌ ▐▌               ▐▌  ▐▌      ▝▚▄▟▌     ");
  Serial.println("                                          ");
  Serial.println("                                          ");
  Serial.println("                                          ");
  

  // Set preference
  preferences.begin("aero", true);
  #if defined(_STRIP_POSITION)
    if (preferences.getUInt("strip_position", -1) != _STRIP_POSITION) {
      preferences.end();
      preferences.begin("aero", false);
      preferences.putUInt("strip_position", _STRIP_POSITION);
      preferences.end();
      preferences.begin("aero", true);
    }
  #endif

  // Load preferences
  STRIP_POSITION = preferences.getUInt("strip_position", 0);

  while (STRIP_POSITION == 0) {
    Serial.println("Please set strip position in preferences");
    delay(1000);
  }
  Serial.printf("Strip position: %d\n", STRIP_POSITION);

  // Set up ethernet
  esp_efuse_mac_get_default(mac); mac[0] = 0x02;
  ip[3] = STRIP_POSITION;
  Ethernet.begin(mac, ip);

  // Set up artnet
  universeCount = ceil( PIXEL_COUNT * 4 / 512 );
  universeStart = ((STRIP_POSITION - 1) * universeCount) + 1;
  artnet.begin();

  Serial.print("Starting Artnet on universes: ");
  for (int i = 0; i <= universeCount; i++) {
    artnet.subscribeArtDmxUniverse(universeStart+i, 
      [&](const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote) {
          Serial.print("lambda : artnet data from ");
          Serial.print(remote.ip);
          Serial.print(":");
          Serial.print(remote.port);
          Serial.print(", universe = ");
          Serial.print(universeStart+i);
          Serial.print(", size = ");
          Serial.print(size);
          Serial.print(") :");
          for (size_t i = 0; i < size; ++i) {
              Serial.print(data[i]);
              Serial.print(",");
          }
          Serial.println();
      });
    Serial.printf(" %d", universeStart+i);
  }
  Serial.println();
 
  // Set STRIP
  digitalLeds_init();
  strip = digitalLeds_addStrand(
          {.rmtChannel = 0, .gpioNum = PIN_LED, .ledType = LED_SK6812W_V3, .brightLimit = 255, .numPixels = PIXEL_COUNT, .pixels = nullptr, ._stateVars = nullptr});
  
  buffer = (pixelColor_t*)malloc(PIXEL_COUNT * sizeof(pixelColor_t));

  // Init buffer 
  clear();

  // Set all pixels to low white
  all(0, 0, 0, 20);

  // Ready
  Serial.println("Ready.");
  
}

//
// LOOP
//
void loop() 
{

  // Push buffer to strip and draw
  if (dirty) {
    memcpy(&strip->pixels, &buffer, sizeof(buffer));
    digitalLeds_updatePixels(strip);           // PUSH LEDS TO RMT
    dirty = false;
  }
  delay(1);
}


