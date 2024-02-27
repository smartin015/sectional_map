#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "metar.h"
#include "config.h"

Config cfg;
DisplayMode display_mode = WEATHER_MODE;
Adafruit_NeoPixel *pixels;
#define LED_PIN D1
#define MODE_SWITCH_PIN D6
#define ANALOG_BRIGHT_PIN A0

void panic_loop(const char* text) {
  Serial.println(text);
  while (1) {}
}

float brightness = 1.0;
int nled = 0;
uint32_t *pxbuf[NUM_DISPLAY_MODES];
void flush_pixels() {
  uint8_t bright = brightness * 255;
  for (int i = 0; i < nled; i++) {
    uint8_t a = (pxbuf[display_mode][i]) & 0xff;
    uint8_t b = (pxbuf[display_mode][i] >> 8) & 0xff;
    uint8_t c = (pxbuf[display_mode][i] >> 16) & 0xff;
    uint8_t d = (pxbuf[display_mode][i] >> 24) & 0xff;
    uint32_t adjusted = (
      ((a * bright) >> 8 )
      | ((b * bright) >> 8) << 8
      | ((c * bright) >> 8) << 16
      | ((d * bright) >> 8) << 24
    ); 
    pixels->setPixelColor(i, pixels->gamma32(adjusted));
  }
  pixels->show();
}
String loc_csv = "";

void do_update();

uint8_t mode_sw = HIGH;
// IRAM_ATTR is wemos d1 mini specific syntax, see
// https://arduino-esp8266.readthedocs.io/en/latest/reference.html#digital-io
IRAM_ATTR void mode_switch_isr() {
  mode_sw = digitalRead(MODE_SWITCH_PIN);
}

int status_px = 0;
void incr_status_px() {
  for (int i = 0; i < status_px; i++) {
    pixels->setPixelColor(i, pixels->Color(0, 150, 0));
  } 
  pixels->show();
  status_px++;
}

void setup() {
  pixels = new Adafruit_NeoPixel(20, LED_PIN, NEO_GRB + NEO_KHZ800);
  pixels->begin();
  pixels->clear();
  incr_status_px();

  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MODE_SWITCH_PIN), mode_switch_isr, CHANGE);

  Serial.begin(115200);
  Serial.println("Reading config\n");
  int status = read_config(cfg, "/config.txt");
  if (status != ERR_NONE) {
      for (int i = 0; i < status; i++) {
        pixels->setPixelColor(10+i, pixels->Color(255, 0, 0));
      }
      pixels->show();
      panic_loop("Config error");
  } 
  Serial.print("Done reading config. Num locations: ");
  Serial.println(cfg.num);
  incr_status_px();
  
  Serial.println("=== color overrides ===");
  for (int i = 0; i < cfg.num; i++) {
    if (cfg.locations[i].ovr != 0) {
      Serial.print(cfg.locations[i].name);
      Serial.print(" (idx ");
      Serial.print(cfg.locations[i].idx);
      Serial.print("): set to #");
      Serial.println(cfg.locations[i].ovr, HEX);
    }
  }
  Serial.println("=== end color ovrrides ===");

  Serial.println("Setting up LEDs\n");
  nled = 0;
  for (int i = 0; i < cfg.num; i++) {
    nled = max(nled, cfg.locations[i].idx+1);
  }
  free(pixels);
  pixels = new Adafruit_NeoPixel(nled, LED_PIN, NEO_GRB + NEO_KHZ800);
  incr_status_px();

  Serial.print("Connecting to SSID: ");
  Serial.print(cfg.ssid);
  Serial.print(", password: ");
  Serial.println(cfg.pass);
  WiFi.begin(cfg.ssid, cfg.pass);
  incr_status_px();
  int wifi_wait = 0;
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (wifi_wait++ == 40) {
      printf("Taking a long time to connect, throwing a color");
      pixels->setPixelColor(10, pixels->Color(255, 0, 0));
      pixels->show();
    }
  }
  incr_status_px();
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // TODO disable if not using hourly timing
  //Serial.println("Awaiting NTP server");
  //await_sync();
  //Serial.print("Time synced");
  //do_update();
  pxbuf[WEATHER_MODE] = (uint32_t*) malloc(nled * sizeof(uint32_t));
  pxbuf[WIND_MODE] = (uint32_t*) malloc(nled * sizeof(uint32_t));
  for (int i = 0; i < nled; i++) {
    pxbuf[WEATHER_MODE][i] = 0x440000;
    pxbuf[WIND_MODE][i] = 0x440000;
  }
  flush_pixels();
}

void write_loc_csv(int start_idx, int count) {
  loc_csv = "";
  for (int i = 0; i < count; i++) {
    auto& loc = cfg.locations[start_idx+i];
    if (loc.ovr != 0) {
      // Override color setting
      pxbuf[WEATHER_MODE][loc.idx] = loc.ovr;
      pxbuf[WIND_MODE][loc.idx] = loc.ovr;
      continue;
    }
    if (i > 0) {
      loc_csv += ",";
    }
    loc_csv += loc.name;
  }
}

#define BUFSZ 6000 
#define BATCH_SZ 3
char buf[BUFSZ];
METAR results[BATCH_SZ];
int get_metars(int start_idx, int count) {
  int num_extracted = 0;
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Not connected; skipping fetch");
    return num_extracted;
  }
  
  // Reset values to ensure no previous records
  for (int i = 0; i < BATCH_SZ; i++) {
    results[i].name[0] = '\0';
    results[i].vis = 0;
    results[i].ceiling = 0;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Allow insecure HTTPS fetches with no verification

  HTTPClient http;
  write_loc_csv(start_idx, count);
  String addr = "https://aviationweather.gov/cgi-bin/data/metar.php?ids=" + loc_csv;
  Serial.print("Connecting to ");
  Serial.println(addr);
  http.begin(client, addr);
  int rc = http.GET();
  if (rc == HTTP_CODE_OK) {
    size_t len = http.getString().length();
    if (len > BUFSZ) {
      Serial.print("ERROR ERROR ERROR: Buffer length exceeded - size of response is ");
      Serial.println(len);
    } else if (len > 0) {
      Serial.print("Received ");
      Serial.print(len);
      Serial.println(" bytes; extracting...");
      strncpy(buf, http.getString().c_str(), len);
      num_extracted = extract_metar(buf, len, results, count);
      Serial.print("Extracted ");
      Serial.print(num_extracted);
      Serial.println(" locations");
    } else {
      Serial.println("0 bytes received; skipping batch");
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
  Serial.print("ft\twind: ");
  Serial.print(m.wind_speed);
  Serial.print("kt\tgust: ");
  Serial.print(m.gusts);
  Serial.print("kt\tpixel_idx: ");
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

void render(int extracted, DisplayMode mode) {
  for (int i = 0; i < extracted; i++) {
    uint32_t c = pixels->Color(1, 0, 0);

    // Must declare out of switch statement
    int w = results[i].wind_speed;
    if (results[i].gusts > 0) {
      w = results[i].gusts;
    }

    switch (mode) {
      case WEATHER_MODE:
        switch (metar_category(results[i])) {
          case VFR:
            c = pixels->Color(0, 200, 0);
            break;
          case MVFR:
            c = pixels->Color(60, 0, 100);
            break;
          case IFR:
            c = pixels->Color(100, 0, 60);
            break;
          case LIFR:
            c = pixels->Color(128, 0, 20);
            break;
          default:
            break;
        }
        break;
      case WIND_MODE:
        if (w < 5) {
          c = pixels->Color(0, 255, 0);
        } else if (w < 10) {
          c = pixels->Color(0, 0, 255);
        } else if (w < 15) {
          c = pixels->Color(255, 0, 0);
        } else {
          c = pixels->Color(255, 0, 255);
        }
        break;

      default:
        break;
    }


    int loc = get_location(results[i].name);
    if (loc >= 0) {
      pxbuf[mode][loc] = c;
    }
  }
}

uint8_t next_hr = -1;
uint8_t next_minute = 5;
bool should_update_hourly() {
  time_t now = get_time();
  uint8_t now_hr = get_hour(now);
  uint8_t now_minute = get_minute(now);
  if (now_hr == next_hr && now_minute >= next_minute) {
    next_hr = (now_hr+1)%24;
    return true;
  }
  return false;
}

uint64_t next_update_ms = 0;
bool should_update_periodic(uint64_t period_ms) {
  uint64_t now = millis();
  uint64_t dt = next_update_ms - now;
  if (now > next_update_ms || dt > 10*period_ms) {
    next_update_ms = now + period_ms; // Not precise timing, but we don't really need to "catch up" from delays.
    return true;
  }
  return false;
}

int num_extracted = 0;
void do_update() {
    Serial.println("Updating...");
    num_extracted = 0;
    for (int i = 0; i < cfg.num; i += BATCH_SZ) {
      int count = min(BATCH_SZ, cfg.num-i);
      num_extracted = get_metars(i, count);
      for (int j = 0; j < num_extracted; j++) {
        printMETAR(results[j]);
      }
      render(num_extracted, WEATHER_MODE);
      render(num_extracted, WIND_MODE);
      Serial.println("Rendered weather and wind buffers");
    }
    flush_pixels();
    Serial.println("Updated");
}

uint8_t prev_mode_sw = HIGH;
uint64_t mode_sw_debounce = 0;
#define DEBOUNCE 100
void handle_mode() {
  uint64_t now = millis();
  if (mode_sw && mode_sw != prev_mode_sw && int64_t(now - mode_sw_debounce) > 0) {
    mode_sw_debounce = now+DEBOUNCE;
    display_mode = DisplayMode((display_mode + 1) % NUM_DISPLAY_MODES);
    Serial.print("Display mode is now ");
    Serial.println(DISPLAY_MODE_NAMES[display_mode]);
    flush_pixels();
  }
  prev_mode_sw = mode_sw;
}

void handle_brightness() {
  float new_bright = (2 * brightness + (float(analogRead(ANALOG_BRIGHT_PIN)) / 1024.0)) / 3;
  if ((new_bright - brightness) > 0.01 || (brightness - new_bright) > 0.01) {
    brightness = new_bright;
    Serial.print("Brightness ");
    Serial.println(brightness);
    flush_pixels();
  }
}

void loop() {
  handle_mode();
  // handle_brightness();

  //if (should_update_hourly()) {
  if (should_update_periodic(5*60*1000)) {
    do_update();
  }
  delay(50);
}
