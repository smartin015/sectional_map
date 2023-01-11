#include "metar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char* CATEGORY_STR[] = {
  "INVALID",
  "LIFR",
  "IFR",
  "MVFR",
  "VFR",
  "NUM_CATEGORIES"
};

const char* category_str(Category c) {
  if (c > NUM_CATEGORIES || c < 0) {
    c = CATEGORY_INVALID;
  }
  return CATEGORY_STR[c];
}

int DIR_COMPASS[] = {10000, 0, 45, 90, 135, 180, 225, 270, 315, 0};
int dir_to_angle(Direction dir) {
  return DIR_COMPASS[dir];
}

Direction to_wind_dir(int deg) {
  Direction result = DIR_INVALID;
  int best = dir_to_angle(result);
  for (int i = DIR_N; i < NUM_DIR; i++) {
    Direction d = Direction(i);
    int delta = abs(deg - dir_to_angle(d));
    // printf("%d - %d = %d vs %d\n", deg, dir_to_angle(d), delta, best);
    if (delta < best) {
      // printf("new best dir %d\n", d);
      result = d;
      best = delta;
    }
  }
  return result;
}

#define METAR_SEPS " <"

void parse_metar(const char* metar, uint16_t len, METAR &result) {
  char m[len+1];
  strncpy(m, metar, len);
  m[len] = 0;


  result.vis = VIS_MAX;
  result.ceiling = CEIL_MAX;
  result.wind_dir = DIR_INVALID;
  result.lightning = LIGHTNING_NONE;
  result.wind_speed = 0;
  result.gusts = 0;
  strncpy(result.name, m, NAME_SZ);
  result.name[NAME_SZ] = '\0';

  char* saveptr;
  char* ptr = strtok_r(m, METAR_SEPS, &saveptr);
  // Skip over the first part of the string - airport code may have trigger characters in it
  ptr = strtok_r(NULL, METAR_SEPS, &saveptr);
  while (ptr != NULL) {
    if (strstr(ptr, "SM")) {
      // Visibility ends in "SM" (statute miles), e.g. 10SM
      if (strstr(ptr, "/")) {
        // Fractional visibility is rounded down to 0
        result.vis = 0;
      } else {
        result.vis = atoi(ptr);
      }
    } else if (strstr(ptr, "BKN") || strstr(ptr, "OVC")) {
      // Cloud ceiling, e.g. OVC150, BKN220
      int c = atoi(ptr+3)*100;
      if (c < result.ceiling) {
        result.ceiling = c;
      }
    } else if (strstr(ptr, "KT")) {
      // Wind direction and speed
      // e.g. 15011G17KT, 25004KT, VRB12KT
      char buf[] = {ptr[0], ptr[1], ptr[2], 0};
      if (strcmp(buf, "VRB")==0) {
        result.wind_dir = DIR_VARIABLE;
      } else {
        int deg = atoi(buf);
        result.wind_dir = to_wind_dir(deg);
      }
      result.wind_speed = atoi(ptr+3);
      char* gust_ptr = strstr(ptr, "G");
      if (gust_ptr) {
        result.gusts = atoi(gust_ptr+1);
      }
    } else if (strstr(ptr, "TS") || strstr(ptr, "DSNT")) {
      // Lightning / thunderstorm info, e.g.
      // `LTG DSNT N-E`, `TS`, `VCTS`
      if (strstr(ptr, "DSNT")) {
        result.lightning = LIGHTNING_DISTANT;
      } else if (strstr(ptr, "VCTS")) {
        result.lightning = LIGHTNING_VICINITY;
      } else if (strstr(ptr, "TS")) {
        result.lightning = LIGHTNING_HERE;
      }
    }
    ptr = strtok_r(NULL, METAR_SEPS, &saveptr);
  }
}

Category metar_category(const METAR &m) {
  Category c, v;

  if (m.name[0] == 0) {
    return CATEGORY_INVALID;
  }

  if (m.ceiling < 500) {
    c = LIFR;
  } else if (m.ceiling < 1000) {
    c = IFR;
  } else if (m.ceiling < 3000) {
    c = MVFR;
  } else {
    c = VFR;
  }
  
  if (m.vis < 1) {
    v = LIFR;
  } else if (m.vis < 3) {
    v = IFR;
  } else if (m.vis <= 5) {
    v = MVFR;
  } else {
    v = VFR;
  }

  return (c < v) ? c : v;
}

uint16_t extract_metar(char* html, uint16_t len, METAR* result, uint16_t rlen) {
  METAR* mptr = result;
  char* ptr = strstr(html, METAR_BEGIN);
  while (ptr != NULL) {
    // printf("ptr = %s\n", ptr);
    char *start = ptr + strlen(METAR_BEGIN); 
    char *end = strstr(ptr, METAR_END); 
    if (start == NULL || end == NULL) {
      // printf("start or end is null: %x %x", start, end);
      return 0; // Shouldn't happen, but oh well
    }
    parse_metar(start, end-start, *mptr);
    if ((++mptr - result) >= rlen) {
      // printf("mptr - result = %d\n", mptr - result);
      return rlen;
    }
    ptr = strstr(end, METAR_BEGIN);
  }
  return (mptr - result);
}
