#include <Adafruit_NeoPixel.h>
// #include <WiFi.h>
#include <ESP8266WiFi.h>
// #include <HTTPClient.h>
#include <ESP8266HTTPClient.h>
#include "metar.h"

#define POLL_PD 10000
#define LED_PIN D1
#define NUMPIXELS 16
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRBW + NEO_KHZ800);

struct LocationConfig {
  const char* name;
  int idx;
};

#define NUM_LOC 50
const LocationConfig LOCATIONS[] = {
  {"KSUA", 0},
  {"KF45", 1},
  {"KPBI", 2},
  {"KBCT", 3},
  {"KLNA", 4},
  {"K2IS", 5},
  {"KIMM", 6},
  {"KRSW", 7},
  {"KFMY", 8},
  {"KPGD", 9},
  {"KVNC", 10},
  {"KSRQ", 11},
  {"KSPG", 12},
  {"KPIE", 13},
  {"KCLW", 14},
  {"KTPA", 15},
  {"KTPF", 16},
  {"KVDF", 17},
  {"KPCM", 18},
  {"KZPH", 19},
  {"KLAL", 20},
  {"KBOW", 21},
  {"KGIF", 22},
  {"KX07", 23},
  {"KAVO", 24},
  {"KSEF", 25},
  {"KOBE", 26},
  {"KVRB", 27},
  {"KMLB", 28},
  {"KCOI", 29},
  {"KTIX", 30},
  {"KX21", 31},
  {"KEVB", 32},
  {"KDAB", 33},
  {"KOMN", 34},
  {"KFIN", 35},
  {"KDED", 36},
  {"KSFB", 37},
  {"KORL", 38},
  {"KMCO", 39},
  {"KISM", 40},
  {"KLEE", 41},
  {"KBKV", 42},
  {"KINF", 43},
  {"KCGC", 44},
  {"KOCF", 45},
  {"KX60", 46},
  {"KGNV", 47},
  {"K42J", 48},
  {"K28J", 49}
};

int    HTTP_PORT   = 80;
String HTTP_METHOD = "GET"; // or "POST"
char   HOST_NAME[] = "example.phpoc.com"; // hostname of web server:
String PATH_NAME   = "";

#define ssid "robotoverlords"
#define password "oakdale43"

String loc_csv = "";

void setup() {
  Serial.begin(115200);
  pixels.begin();
  for (int i = 0; i < NUM_LOC; i++) {
    pixels.setPixelColor(i, pixels.Color(10, 10, 10));
  }
  pixels.show();

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  loc_csv = LOCATIONS[0].name;

  for (int i = 1; i < NUM_LOC; i++) {
    loc_csv += ",";
    loc_csv += LOCATIONS[i].name;
  }
  Serial.println(loc_csv);

}

METAR results[NUM_LOC];
int get_metars() {
  int num_extracted = 0;
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Not connected; skipping fetch");
    return num_extracted;
  }

  for (int i = 0; i < NUM_LOC; i++) {
    results[i].name[0] = '\0';
    results[i].vis = 0;
    results[i].ceiling = 0;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Allow insecure HTTPS fetches with no verification
  HTTPClient http;
  String addr = "https://www.aviationweather.gov/metar/data?ids="+loc_csv+"&format=raw&hours=0&taf=off&layout=off&date=0";
  Serial.print("Connecting to ");
  Serial.println(addr);
  http.begin(client, addr);
  int rc = http.GET();
  if (rc == HTTP_CODE_OK) {
    String payload = http.getString();
    num_extracted = extract_metar(payload.c_str(), payload.length(), results, NUM_LOC);
    Serial.print("Extracted ");
    Serial.print(num_extracted);
    Serial.println(" locations");
  } else {
    Serial.print("Error code: ");
    Serial.println(rc);
  }
  // Free resources
  http.end();
  return num_extracted;
}

void render() {
  for (int i = 0; i < NUM_LOC; i++) {
    uint32_t c = pixels.Color(255, 0, 0);
    switch (metar_category(results[i])) {
      case VFR:
        c = pixels.Color(0, 255, 255);
        break;
      case MVFR:
        c = pixels.Color(0, 76, 153);
        break;
      case IFR:
        c = pixels.Color(76, 0, 153);
        break;
      case LIFR:
        c = pixels.Color(204, 0, 204);
        break;
      default:
        break;
    }
    pixels.setPixelColor(LOCATIONS[i].idx, c);
  }
  pixels.show();
}

uint64_t next = 0;

void loop() {
  uint64_t now = millis();
  if (now > next || (next > 1000000 && now < 10000)) {
    get_metars();
    render();
    next = now + POLL_PD;
    Serial.println("Updated");
  }
}
