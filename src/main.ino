#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "metar.h"
#include "config.h"

Config cfg;
Adafruit_NeoPixel *pixels;
#define LED_PIN D1

String loc_csv = "";

void do_update(int now_hr);

void setup() {
  Serial.begin(115200);
  Serial.println("Reading config\n");
  read_config(cfg, "/config.txt");
  Serial.print("Done reading config. Num locations: ");
  Serial.println(cfg.num);

  Serial.println("Setting up LEDs\n");
  pixels = new Adafruit_NeoPixel(cfg.num, LED_PIN, NEO_GRBW + NEO_KHZ800);
  pixels->begin();
  for (int i = 0; i < cfg.num; i++) {
    pixels->setPixelColor(i, pixels->Color(1, 0, 0, 0));
  }
  pixels->show();
  Serial.print("Connecting to SSID: ");
  Serial.print(cfg.ssid);
  Serial.print(", password: ");
  Serial.println(cfg.pass);

  WiFi.begin(cfg.ssid, cfg.pass);
  while(WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Awaiting NTP server");
  await_sync();
  Serial.print("Time synced");
  time_t now = get_time();
  uint8_t now_hr = get_hour(now);
  do_update(now_hr);
}

void write_loc_csv(int start_idx, int count) {
  loc_csv = cfg.locations[start_idx].name;
  for (int i = 1; i < count; i++) {
    auto& loc = cfg.locations[start_idx+i];
    if (loc.ovr != 0) {
      // Override color setting
      pixels->setPixelColor(loc.idx, loc.ovr);
      continue;
    }
    loc_csv += ",";
    loc_csv += loc.name;
  }
}

#define BUFSZ 8600 
#define BATCH_SZ 20
char buf[BUFSZ];
METAR results[BATCH_SZ];
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
  // Yes, this is inefficient. But num is small, so whatever
  for (int i = 0; i < cfg.num; i++) {
    if (strcmp(cfg.locations[i].name, name) == 0) {
      return cfg.locations[i].idx;
    }
  }
  return -1;
}

void render() {
  for (int i = 0; i < cfg.num; i++) {
    uint32_t c = pixels->Color(1, 0, 0, 0);
    switch (metar_category(results[i])) {
      case VFR:
        c = pixels->Color(0, 64, 128, 0);
        break;
      case MVFR:
        c = pixels->Color(60, 0, 100, 0);
        break;
      case IFR:
        c = pixels->Color(100, 0, 60, 0);
        break;
      case LIFR:
        c = pixels->Color(128, 0, 20, 0);
        break;
      default:
        break;
    }
    int loc = get_location(results[i].name);
    if (loc >= 0) {
      pixels->setPixelColor(loc, c);
    }
  }
  pixels->show();
}

uint8_t next_hr = -1;
uint8_t next_minute = 5;
void do_update(int now_hr) {
    Serial.println("Updating...");
    int extracted = 0;
    for (int i = 0; i < cfg.num; i += BATCH_SZ) {
      int count = min(BATCH_SZ, cfg.num-i);
      extracted = get_metars(i, count);
      for (int j = i; j < i+count; j++) {
        printMETAR(results[j]);
      }
    }
    if (extracted > 0) {
      render();
    }
    next_hr = (now_hr+1)%24;
    Serial.println("Updated");
}

void loop() {
  time_t now = get_time();
  uint8_t now_hr = get_hour(now);
  uint8_t now_minute = get_minute(now);
  if (now_hr == next_hr && now_minute >= next_minute) {
    do_update(now_hr);
  }
  delay(100);
}
