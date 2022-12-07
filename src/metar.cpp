#include "metar.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void parse_metar(const char* metar, uint16_t len, METAR &result) {
  char m[len];
  strncpy(m, metar, len);

  result.vis = VIS_MAX;
  result.ceiling = CEIL_MAX;
  strncpy(result.name, m, NAME_SZ);
  result.name[NAME_SZ] = '\0';

  char* saveptr;
  char* ptr = strtok_r(m, " ", &saveptr);
  while (ptr != NULL) {
    char* eptr = strchr(ptr, ' ');
    if (eptr == NULL) {
      eptr = strchr(ptr, 0);
    }
    // Visibility ends in "SM" (statute miles)
    if (strstr(ptr, "SM")) {
      if (strstr(ptr, "/")) {
        // Fractional visibility is rounded down to 0
        result.vis = 0;
      } else {
        result.vis = atoi(ptr);
      }
    }
    
    // Now check cloud ceiling
    if (strstr(ptr, "BKN") || strstr(ptr, "OVC")) {
      int c = atoi(ptr+3)*100;
      if (c < result.ceiling) {
        result.ceiling = c;
      }
    }
    ptr = strtok_r(NULL, " ", &saveptr);
  }
}

Category metar_category(const METAR &m) {
  Category c, v;

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

uint16_t extract_metar(const char* html, uint16_t len, METAR* result, uint16_t rlen) {
  char m[len+1];
  strncpy(m, html, len);
  m[len] = 0;
  METAR* mptr = result;

  char* ptr = strstr(m, METAR_BEGIN);
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
