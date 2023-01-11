#include "config.h"
#include <stdlib.h>     /* atoi */
#include <cstring>
#include <cstdio>

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

void read_config_line(Config& cfg, const char* line) {
  if (cfg.num >= MAX_LOCATIONS) {
    return;
  }
  // split line, check / parse 
  char buf[CFG_READ_BUFSZ+1];
  strncpy(buf, line, CFG_READ_BUFSZ);
  buf[CFG_READ_BUFSZ]=0; // ensure null termination

	replacechar(buf, '\n', '\0');
  char* p1 = strtok(buf, "=");
  char* p2 = strtok(0, "=");
  if (p1 == NULL || p2 == NULL) {
    printf("Skipping unparseable line: \"%s\"\n", buf);
    return;
  }
  if (strcmp(p1, "SSID") == 0) {
    if (strlen(p2) > MAX_WIFI_FIELD_SZ) {
      printf("ERROR: Cannot set SSID; max length exceeded\n");
    } else {
      strncpy(cfg.ssid, p2, MAX_WIFI_FIELD_SZ);
    }
  } else if (strcmp(p1, "PASS") == 0) {
    if (strlen(p2) > MAX_WIFI_FIELD_SZ) {
      printf("ERROR: Cannot set PASS; max length exceeded\n");
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
        cfg.locations[cfg.num].ovr = strtol(p3, NULL, 16);
      }
    }
    if (strlen(p1) > LOCNAME_SZ) {
      printf("ERROR: Skipping location %s; location name too long\n", p1);
    } else if (strspn(p2, "0123456789") != strlen(p2)) {
      printf("ERROR: Skipping location %s; LED index is not digit\n", p1);
    } else {
      strncpy(cfg.locations[cfg.num].name, p1, LOCNAME_SZ);
      cfg.locations[cfg.num].idx = atoi(p2);
      cfg.num++;
    }
  }
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


void panic_loop(const char* text) {
  Serial.println(text);
  while (1) {}
}

void await_sync() {
  waitForSync();
  tz.setLocation(TZ_LOCATION);
}

void read_config(Config &cfg, const char* path) {
  if(!LittleFS.begin()){
    panic_loop("Error while mounting the filesystem");
  }
  File file = LittleFS.open(path, "r");
  if(!file){
    panic_loop("Failed to open path");
  }
  while (file.available()) {
    Serial.print(">");
    String line = file.readStringUntil('\n');
    Serial.println(line);
    read_config_line(cfg, line.c_str());
  }
  file.close();
}

time_t get_time() {
  return tz.now();
}
#else
#include <stdio.h>
#include <ctime>

void panic_loop(const char* text) {
  printf(text);
  printf("\n");
  exit(EXIT_FAILURE);
}


void read_config(Config &cfg, const char* path) {
	FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(path, "r");
  if (fp == NULL) {
    panic_loop("Couldn't open file");
  }
  while ((read = getline(&line, &len, fp)) != -1) {
      read_config_line(cfg, line);
  }
  fclose(fp);
  if (line) {
    free(line);
  }
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
