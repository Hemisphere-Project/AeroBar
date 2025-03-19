#include <Arduino.h>
#include <Preferences.h>
#include <SPI.h>
#include <ArtnetEther.h>
#include "librmt/esp32_digital_led_lib.h"
#include "libfast/crgbw.h"
#include "network/K32_wifi.h"

// 
// CONFIGURATION
//

// Uncomment to set WARNING: for USB ONLY !!!
// #define _STRIP_POSITION 2

const int STRIP_SIZES[19] = {0, 528, 537, 543, 537, 534, 537, 540, 537, 537, 525, 525, 528, 531, 543, 540, 534, 528, 528};
int PIXEL_COUNT = 0;  // 660

const int PIN_LED = 26; // 26, 32
const int PIN_REF = 32; // 26, 32
const int PIN_ETH = 21; 

#define SCK  22
#define MISO 23
#define MOSI 33
#define CS   19

//
// VARIABLES
//

// Internal LEDS
bool dirty = false;
strand_t* strip;
pixelColor_t* buffer;
pixelColor_t* bufferOUT;
SemaphoreHandle_t bufferMutex;

// Internals
int STRIP_POSITION;
Preferences preferences;

// Tests
unsigned long lastChange = 0;
int color = 0;
int testMaster = 250;
bool testing = true;
int noNetwork = 0;

// Artnet stuff
ArtnetReceiver artnet;
IPAddress ip(10, 0, 0, 0);
uint8_t mac[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
int universeStart = 0;
int universeCount = 4;
const uint8_t dmxPixelSize = 3; // 4: RGBW, 3: RGB
int lastSequence = 0;

// Wifi
K32_wifi *wifi;

//
// LEDS
//
// drawTask
void drawTask(void *pvParameters) {
  while (true) {
    if (dirty) {
      xSemaphoreTake(bufferMutex, portMAX_DELAY);
      memcpy(&strip->pixels, &bufferOUT, sizeof(bufferOUT));
      xSemaphoreGive(bufferMutex);
      dirty = false;
      digitalLeds_updatePixels(strip);           // PUSH LEDS TO RMT
    }
    else delay(1);
  }
}

void draw() {
  xSemaphoreTake(bufferMutex, portMAX_DELAY);
  memcpy(bufferOUT, buffer, PIXEL_COUNT * sizeof(pixelColor_t));
  xSemaphoreGive(bufferMutex);
  dirty = true;
}

void clear() {
  memset(buffer, 0, PIXEL_COUNT * sizeof(pixelColor_t));
  draw();
}

void all(pixelColor_t color) {
  for (int i = 0; i < PIXEL_COUNT; i++) buffer[i] = color;
  draw();
}

void all(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  all(pixelFromRGBW(r, g, b, w));
}

void all(uint8_t r, uint8_t g, uint8_t b) {
  all(pixelFromRGBtoW(r, g, b));
}

void dumpArtnet(const uint8_t *data, uint16_t size, ArtNetRemoteInfo remote, ArtDmxMetadata metadata) 
{
  Serial.print("lambda : artnet data from ");
  Serial.print(remote.ip);
  Serial.print(":");
  Serial.print(remote.port);
  Serial.print(", universe = ");
  Serial.print(metadata.universe + 16*metadata.subnet + 256*metadata.net);
  Serial.print(", sequence = ");
  Serial.print(metadata.sequence);
  // Serial.print(", physical = ");
  // Serial.print(metadata.physical);
  // Serial.print(", net = ");
  // Serial.print(metadata.net);
  // Serial.print(", subnet = ");
  // Serial.print(metadata.subnet);
  Serial.print(", size = ");
  Serial.print(size);
  Serial.print(") : ");
  for (size_t i = 0; i < 8; ++i) {
      Serial.print(data[i]);
      Serial.print(",");
  }
  Serial.println();

}

void onArtnet(const uint8_t *data, uint16_t size, const ArtDmxMetadata &metadata, const ArtNetRemoteInfo &remote) {
  // STOP TESTING
  if (testing) {
    Serial.println("Stop testing, ArtNet received");
    testing = false;
    all(0, 0, 0, 0);
  }

  // if (metadata.sequence != lastSequence) {
  //   draw();
  //   lastSequence = metadata.sequence;
  // }

  // dumpArtnet(data, size, remote, metadata);
  int universe = metadata.universe + 16*metadata.subnet + 256*metadata.net;
  // Serial.printf("Universe: %d\n", universe);
  int pixStart = (universe - universeStart) * (512/dmxPixelSize);
  if (pixStart < 0) {
    Serial.println("ERROR: pixStart < 0");
    return;
  }
  int pixEnd = pixStart + (size / dmxPixelSize) - 1;
  if (pixEnd >= PIXEL_COUNT) pixEnd = PIXEL_COUNT-1;
  if (pixEnd < 0) {
    Serial.println("ERROR: pixStart < 0");
    return;
  }
  // Serial.printf("Setting pixels %d to %d\n", pixStart, pixEnd);
  for (int i = pixStart; i <= pixEnd; i++) {
    int ix = (i - pixStart) * dmxPixelSize;
    if (dmxPixelSize ==  4)     buffer[i] = pixelFromRGBW(data[ix], data[ix+1], data[ix+2], data[ix+3]);
    else if (dmxPixelSize == 3) buffer[i] = pixelFromRGBtoW(data[ix], data[ix+1], data[ix+2]);
  }

  // Last Universe: set dirty
  // if (metadata.universe == universeStart + universeCount) dirty = true;
  // if (metadata.universe == universeStart ) dirty = true;
}

void serviceLight() 
{
  // get value from 0.8 to 1.0 * testMaster
  // int testV = testMaster * (0.8 + 0.2 * sin(millis() / 1000.0));
  // all(testV, testV/1.1, testV/1.4);
  all(testMaster, testMaster/1.1, testMaster/1.4);
}


//
// SETUP
//
void setup() {

  // Reset ETH stack
  pinMode(PIN_ETH, OUTPUT);
  digitalWrite(PIN_ETH, LOW);
  delay(200);
  digitalWrite(PIN_ETH, HIGH);
  delay(100);

  // Init Serial
  Serial.begin(115200);
  delay(1000);

  // Set Pin REF to high
  pinMode(PIN_REF, OUTPUT);
  digitalWrite(PIN_REF, HIGH);

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

  // Set pixel count
  PIXEL_COUNT = STRIP_SIZES[STRIP_POSITION];
  Serial.printf("Pixel count: %d\n", PIXEL_COUNT);

  // Set STRIP
  digitalLeds_init();
  strip = digitalLeds_addStrand(
          {.rmtChannel = 0, .gpioNum = PIN_LED, .ledType = LED_SK6812W_V1, .brightLimit = 255, .numPixels = PIXEL_COUNT, .pixels = nullptr, ._stateVars = nullptr});
  
  buffer = (pixelColor_t*)malloc(PIXEL_COUNT * sizeof(pixelColor_t));
  bufferOUT = (pixelColor_t*)malloc(PIXEL_COUNT * sizeof(pixelColor_t));
  bufferMutex = xSemaphoreCreateMutex();

  // Init buffer 
  serviceLight();

  // Draw thread
  xTaskCreate(drawTask, "drawTask", 4096, NULL, 1, NULL);
  
  delay(1000);

  // Set up ethernet
  esp_efuse_mac_get_default(mac); 
  mac[0] = 0x02;
  ip[3] = STRIP_POSITION;
  SPI.begin(SCK, MISO, MOSI, -1);
  Ethernet.init(CS);
  delay(200);
  // print mac
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Ethernet.begin(mac, ip, IPAddress(10, 0, 0, 254), IPAddress(10, 0, 0, 254), IPAddress(255, 255, 255, 0));
  delay(200);
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());

  // Set up artnet
  // universeCount = ceil( PIXEL_COUNT * dmxPixelSize / 512 );
  universeStart = ((STRIP_POSITION - 1) * universeCount);
  artnet.begin();

  Serial.print("Starting Artnet on universes: ");
  for (int i = 0; i < universeCount; i++) {
    artnet.subscribeArtDmxUniverse(universeStart + i, onArtnet);
    Serial.printf(" %d", universeStart+i);
  }
  Serial.println();

  // ArtSync
  artnet.subscribeArtSync([](const ArtNetRemoteInfo &remote) { draw(); });

  // ArtPollReply
  String name = "AeroNode-"+String(STRIP_POSITION);
  artnet.setArtPollReplyConfigShortName(name);
  artnet.setArtPollReplyConfigLongName(name);

  // Set up wifi
  wifi = new K32_wifi(name);
  wifi->connect("hmsphr", "hemiproject");
 


  // Ready
  Serial.println("Ready.");
}

//
// LOOP
//
void loop() 
{
  // OTA
  if (wifi->otaState == ERROR) ESP.restart();
  else if (wifi->otaState > OFF) {
    all(0, 0, 50, 0);
    return;
  }

  artnet.parse();      // PARSE ARTNET

  // rotate between all red, green, blue and white every 5 seconds
  if (testing)
    if (millis() - lastChange > 1000) {
      lastChange = millis();
      
      serviceLight();
      noNetwork += 1;

      // No network -> Restart all
      if (noNetwork > 120) {
        Serial.println("No ArtNet for too long, restarting");
        ESP.restart();  
      }
    }
}




