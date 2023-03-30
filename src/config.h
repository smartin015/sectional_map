#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <ctime>
#define LOCNAME_SZ 4

enum DisplayMode {
  WEATHER_MODE,
  WIND_MODE,
  NUM_DISPLAY_MODES
};
static const char* DISPLAY_MODE_NAMES[NUM_DISPLAY_MODES] = {"WEATHER", "WIND"};

struct LocationConfig {
  char name[LOCNAME_SZ+1];
  int idx;
  uint32_t ovr; // Color override, always sets this color
};

#define MAX_LOCATIONS 128
#define MAX_WIFI_FIELD_SZ 36
struct Config {
  char ssid[MAX_WIFI_FIELD_SZ+1];
  char pass[MAX_WIFI_FIELD_SZ+1];
  LocationConfig locations[MAX_LOCATIONS];
  int num;
  Config();
};

void await_sync();
void read_config(Config &cfg, const char* path);
time_t get_time();
int get_hour(time_t t);
int get_minute(time_t t);

#endif
