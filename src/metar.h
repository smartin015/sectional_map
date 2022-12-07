#ifndef METAR_H
#define METAR_H

#include <stdint.h>

#define NAME_SZ 4
#define CEIL_MAX 10000
#define VIS_MAX 5
#define METAR_BEGIN "<code>"
#define METAR_END "</code>"

enum Lightning {
  LIGHTNING_NONE,
  LIGHTNING_DISTANT,
  LIGHTNING_VICINITY,
  LIGHTNING_HERE,
  NUM_LIGHTNING
};

enum Direction {
  DIR_INVALID,
  DIR_N,
  DIR_NE,
  DIR_E,
  DIR_SE,
  DIR_S,
  DIR_SW,
  DIR_W,
  DIR_NW,
  NUM_DIR,
  DIR_VARIABLE
};

struct METAR {
  char name[NAME_SZ+1];
  int vis; // statute miles, fractional rounded down to 0
  int ceiling; // feet
  Direction wind_dir;
  int wind_speed; // knots
  int gusts; // knots
  Lightning lightning;
};

enum Category {
  CATEGORY_INVALID, 
  LIFR, 
  IFR, 
  MVFR, 
  VFR, 
  NUM_CATEGORIES
};

const char* category_str(Category c);

void parse_metar(const char* metar, uint16_t len, METAR &result);
Category metar_category(const METAR &m);

Direction to_wind_dir(int deg);
int dir_to_angle(Direction);
uint16_t extract_metar(char* html, uint16_t len, METAR* result, uint16_t rlen);

#endif // METAR_H
