#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <ctime>
#define LOCNAME_SZ 4

enum ConfigErr {
  ERR_NONE,
  ERR_PARSE,
  ERR_FS_MOUNT,
  ERR_FILE_OPEN,
  ERR_SSID_TOO_LONG,
  ERR_PASS_TOO_LONG,
  ERR_LOCNAME_TOO_LONG,
  ERR_TOO_MANY_LOCATIONS,
  ERR_PARSE_LOC_INT,
  ERR_PARSE_COLOR_INT,
};

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
ConfigErr read_config(Config &cfg, const char* path);
time_t get_time();
int get_hour(time_t t);
int get_minute(time_t t);

#endif
