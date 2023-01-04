#ifndef METAR_H
#define METAR_H

#include <stdint.h>

#define NAME_SZ 4
#define CEIL_MAX 10000
#define VIS_MAX 5
#define METAR_BEGIN "<code>"
#define METAR_END "</code>"

struct METAR {
  char name[NAME_SZ+1];
  int vis; // statute miles, fractional rounded down to 0
  int ceiling; // feet
};

enum Category {
  INVALID, 
  LIFR, 
  IFR, 
  MVFR, 
  VFR, 
  NUM_CATEGORIES
};

const char* category_str(Category c);

void parse_metar(const char* metar, uint16_t len, METAR &result);
Category metar_category(const METAR &m);

uint16_t extract_metar(char* html, uint16_t len, METAR* result, uint16_t rlen);

#endif // METAR_H
