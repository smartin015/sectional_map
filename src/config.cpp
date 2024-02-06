#include "config.h"
#include <stdlib.h>     /* atoi */
#include <cstring>
#include <cstdio>
#include <ctype.h>

Config::Config() {
  memset(this->ssid, 0, MAX_WIFI_FIELD_SZ+1);
  memset(this->pass, 0, MAX_WIFI_FIELD_SZ+1);
  for (int i = 0; i < MAX_LOCATIONS; i++) {
    memset(this->locations[i].name, 0, LOCNAME_SZ+1);
    this->locations[i].idx = 0;
    this->locations[i].ovr = 0;
  }
  this->num = 0;
}

int replacechar(char *str, char orig, char rep) {
		// https://stackoverflow.com/a/32496876
    char *ix = str;
    int n = 0;
    while((ix = strchr(ix, orig)) != NULL) {
        *ix++ = rep;
        n++;
    }
    return n;
}

#define CFG_READ_BUFSZ 64

bool only_whitespace(char* ptr, size_t sz) {
  for (size_t i = 0; i < sz && ptr[i] != '\0'; i++) {
    if (!isspace(ptr[i])) {
      return false;
    }
  }
  return true;
}

ConfigErr read_config_line(Config& cfg, const char* line) {
  if (cfg.num >= MAX_LOCATIONS) {
    printf("ERROR: Too many locations\n");
    return ERR_TOO_MANY_LOCATIONS;
  }
  // split line, check / parse 
  char buf[CFG_READ_BUFSZ+1];
  strncpy(buf, line, CFG_READ_BUFSZ);
  buf[CFG_READ_BUFSZ]=0; // ensure null termination
  if (only_whitespace(buf, sizeof(CFG_READ_BUFSZ))) {
    printf("Whitespace");
    return ERR_NONE; // Ignore whtiespace
  }
	replacechar(buf, '\n', '\0');
	replacechar(buf, '\r', '\0');
  
  char* p1 = strtok(buf, "=");
  char* p2 = strtok(0, "=");
  if (p1 == NULL || p2 == NULL) {
    printf("Skipping unparseable line: \"%s\"\n", buf);
    return ERR_PARSE;
  }
  if (strcmp(p1, "SSID") == 0) {
    if (strlen(p2) > MAX_WIFI_FIELD_SZ) {
      printf("ERROR: Cannot set SSID; max length exceeded\n");
      return ERR_SSID_TOO_LONG;
    } else {
      strncpy(cfg.ssid, p2, MAX_WIFI_FIELD_SZ);
    }
  } else if (strcmp(p1, "PASS") == 0) {
    if (strlen(p2) > MAX_WIFI_FIELD_SZ) {
      printf("ERROR: Cannot set PASS; max length exceeded\n");
      return ERR_PASS_TOO_LONG;
    } else {
      strncpy(cfg.pass, p2, MAX_WIFI_FIELD_SZ);
    }
  } else {
    //printf("p2: %s\n", p2);
    if (strstr(p2, "#")) {
      char* p3 = strtok(p2, "#");
      p3 = strtok(0, "#");
      //printf("Found ovr: %s\n", p3);
      if (p3 != NULL) {
        char* eptr;
        cfg.locations[cfg.num].ovr = strtol(p3, &eptr, 16);
        if (*eptr != '\0') {
          printf("ERROR: Failed to parse color %s", p3);
          return ERR_PARSE_COLOR_INT;
        }
      }
    }
    if (strlen(p1) > LOCNAME_SZ) {
      printf("ERROR: Skipping location %s; location name too long\n", p1);
      return ERR_LOCNAME_TOO_LONG;
    } 
    /*else if (strspn(p2, "0123456789") != strlen(p2)) {
      printf("ERROR: Skipping location %s; LED index is not digit\n", p1);
    }*/ 
    else {
      strncpy(cfg.locations[cfg.num].name, p1, LOCNAME_SZ);
      char* eptr;
      cfg.locations[cfg.num].idx = strtol(p2, &eptr, 10); 
      if (*eptr != '\0') {
        printf("ERROR: Failed to parse location int value: %s", p2);
        return ERR_PARSE_LOC_INT;
      }
      cfg.num++;
    }
  }
  return ERR_NONE;
}


#ifdef ARDUINO
#include <Arduino.h>
#include "LittleFS.h"
#include <ezTime.h>
#define TZ_LOCATION "America/New York"
Timezone tz;

int get_hour(time_t t) {
  return hour(t);
}
int get_minute(time_t t) {
  return minute(t);
}


void await_sync() {
  waitForSync();
  tz.setLocation(TZ_LOCATION);
}

ConfigErr read_config(Config &cfg, const char* path) {
  if(!LittleFS.begin()){
    printf("Error while mounting the filesystem\n");
    return ERR_FS_MOUNT;
  }
  File file = LittleFS.open(path, "r");
  if(!file){
    printf("Failed to open path");
    return ERR_FILE_OPEN;
  }
  while (file.available()) {
    Serial.print(">");
    String line = file.readStringUntil('\n');
    Serial.println(line);
    ConfigErr status = read_config_line(cfg, line.c_str());
    if (status != ERR_NONE) {
      return status;
    }
  }
  file.close();
  return ERR_NONE;
}

time_t get_time() {
  return tz.now();
}
#else
#include <stdio.h>
#include <ctime>

ConfigErr read_config(Config &cfg, const char* path) {
	FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(path, "r");
  if (fp == NULL) {
    printf("Couldn't open file\n");
    return ERR_FILE_OPEN;
  }
  while ((read = getline(&line, &len, fp)) != -1) {
      ConfigErr status = read_config_line(cfg, line);
      if (status != ERR_NONE) {
        return status;
      }
  }
  fclose(fp);
  if (line) {
    free(line);
  }
  return ERR_NONE;
}

time_t get_time() {
  return time(0);
}

int get_hour(time_t t) {
  return 0; // TODO
}
int get_minute(time_t t) {
  return 0; // TODO
}

#endif // ARDUINO
