#include <Adafruit_NeoPixel.h>
// #include <WiFi.h>
#include <ESP8266WiFi.h>
// #include <HTTPClient.h>
#include <ESP8266HTTPClient.h>
#include "metar.h"

struct LocationConfig {
  const char* name;
  int idx;
};

#define NUM_LOC 47
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
  {"KSEF", 25},
  {"KOBE", 26},
  {"KVRB", 27},
  {"KMLB", 28},
  {"KTIX", 30},
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


#define POLL_PD 10000
#define LED_PIN D1
#define NUMPIXELS NUM_LOC
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRBW + NEO_KHZ800);


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
    pixels.setPixelColor(i, pixels.Color(1, 0, 0, 0));
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
}

METAR results[NUM_LOC];

void write_loc_csv(int start_idx, int count) {
  loc_csv = LOCATIONS[start_idx].name;
  for (int i = 1; i < count; i++) {
    loc_csv += ",";
    loc_csv += LOCATIONS[start_idx + i].name;
  }
}

#define BUFSZ 8600 
char buf[BUFSZ];
int get_metars(int start_idx, int count) {
  int num_extracted = 0;
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Not connected; skipping fetch");
    return num_extracted;
  }
  
  // Reset values to ensure no previous records
  for (int i = start_idx; i < count; i++) {
    results[i].name[0] = '\0';
    results[i].vis = 0;
    results[i].ceiling = 0;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Allow insecure HTTPS fetches with no verification

  HTTPClient http;
  write_loc_csv(start_idx, count);
  String addr = "https://www.aviationweather.gov/metar/data?ids="+loc_csv+"&format=raw&hours=0&taf=off&layout=off&date=0";
  Serial.print("Connecting to ");
  Serial.println(addr);
  http.begin(client, addr);
  int rc = http.GET();
  if (rc == HTTP_CODE_OK) {
    size_t len = http.getString().length();
    if (len > BUFSZ) {
      Serial.print("ERROR ERROR ERROR: Buffer length exceeded - size of response is ");
      Serial.println(len);
    } else {
      Serial.print("Received ");
      Serial.print(len);
      Serial.println(" bytes; extracting...");
      strncpy(buf, http.getString().c_str(), len);
      num_extracted = extract_metar(buf, len, &(results[start_idx]), count);
      Serial.print("Extracted ");
      Serial.print(num_extracted);
      Serial.println(" locations");
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(rc);
  }
  // Free resources
  http.end();
  return num_extracted;
}

void printMETAR(const METAR& m) {
  Serial.print(m.name);
  Serial.print("\tvis: ");
  Serial.print(m.vis);
  Serial.print("sm\tceil: ");
  Serial.print(m.ceiling);
  Serial.print("ft\tpixel_idx: ");
  Serial.print(get_location(m.name));
  Serial.print("\tcategory: ");
  Serial.println(category_str(metar_category(m)));
}

int get_location(const char* name) {
  // Yes, this is inefficient. But NUM_LOC is small, so whatever
  for (int i = 0; i < NUM_LOC; i++) {
    if (strcmp(LOCATIONS[i].name, name) == 0) {
      return LOCATIONS[i].idx;
    }
  }
  return -1;
}

void render() {
  for (int i = 0; i < NUM_LOC; i++) {
    uint32_t c = pixels.Color(1, 0, 0, 0);
    switch (metar_category(results[i])) {
      case VFR:
        c = pixels.Color(0, 64, 128, 0);
        break;
      case MVFR:
        c = pixels.Color(60, 0, 100, 0);
        break;
      case IFR:
        c = pixels.Color(100, 0, 60, 0);
        break;
      case LIFR:
        c = pixels.Color(128, 0, 20, 0);
        break;
      default:
        break;
    }
    int loc = get_location(results[i].name);
    if (loc >= 0) {
      pixels.setPixelColor(loc, c);
    }
  }
  pixels.show();
}

uint64_t next = 0;

#define BATCH_SZ 20
void loop() {
  uint64_t now = millis();
  if (now > next || (next > 1000000 && now < 10000)) {
    Serial.println("Updating...");
    for (int i = 0; i < NUM_LOC; i += BATCH_SZ) {
      int count = min(BATCH_SZ, NUM_LOC-i);
      get_metars(i, count);
      for (int j = i; j < i+count; j++) {
        printMETAR(results[j]);
      }
    }
    render();
    next = now + POLL_PD;
    Serial.println("Updated");
  }
  delay(100);
}
