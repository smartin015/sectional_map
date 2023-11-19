#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

#include "metar.h"

void test_to_wind_dir_n() {
  TEST_ASSERT_EQUAL(DIR_N, to_wind_dir(0));
}

void test_to_wind_dir_ne() {
  TEST_ASSERT_EQUAL(DIR_NE, to_wind_dir(45));
}

void test_to_wind_dir_nw() {
  TEST_ASSERT_EQUAL(DIR_NW, to_wind_dir(315));
}

void test_to_wind_dir_s() {
  TEST_ASSERT_EQUAL(DIR_S, to_wind_dir(180));
}

void test_to_wind_dir_se() {
  TEST_ASSERT_EQUAL(DIR_SE, to_wind_dir(135));
}

void test_to_wind_dir_sw() {
  TEST_ASSERT_EQUAL(DIR_SW, to_wind_dir(225));
}

void test_to_wind_dir_e() {
  TEST_ASSERT_EQUAL(DIR_E, to_wind_dir(90));
}

void test_to_wind_dir_w() {
  TEST_ASSERT_EQUAL(DIR_W, to_wind_dir(270));
}

void test_metar_overcast_intvis() {
  char* m = (char*)"KSEA 071453Z 16005KT 10SM SCT032 OVC046 04/02 A3014 RMK AO2 RAE50 SLP214 P0000 60001 T00390022 58004";
  METAR r;
  parse_metar(m, strlen(m), r);
  TEST_ASSERT_EQUAL(10, r.vis);
  TEST_ASSERT_EQUAL(4600, r.ceiling);
  TEST_ASSERT_EQUAL_STRING("KSEA", r.name);
}

void test_metar_broken_floatvis() {
  char* m = (char*)"KSEA 071453Z 16005KT 3/4SM+ SCT032 BKN046 04/02 A3014 RMK AO2 RAE50 SLP214 P0000 60001 T00390022 58004";
  METAR r;
  parse_metar(m, strlen(m), r);
  TEST_ASSERT_EQUAL(0, r.vis);
  TEST_ASSERT_EQUAL(4600, r.ceiling);
}

void test_vis_zero() {
  char* m = (char*)"KCLE 071551Z 33007KT 0SM -DZ BR OVC002 08/07 A3017 RMK AO2 SFC VIS 1 1/4 SLP234 P0000 T00780067";
  METAR r;
  parse_metar(m, strlen(m), r);
  TEST_ASSERT_EQUAL(0, r.vis);
  
}

void test_category_c_lifr() {
  METAR m = {"FOO", 10, 300};
  TEST_ASSERT_EQUAL(LIFR, metar_category(m));
}

void test_category_v_ifr() {
  METAR m = {"FOO", 2, 10000};
  TEST_ASSERT_EQUAL(IFR, metar_category(m));
}

void test_category_eq_mvfr() {
  METAR m = {"FOO", 5, 2999};
  TEST_ASSERT_EQUAL(MVFR, metar_category(m));
}

void test_category_vfr() {
  METAR m = {"FOO", 10, 100000};
  TEST_ASSERT_EQUAL(VFR, metar_category(m));
}

void test_extract_metar_empty() {
  METAR m[1];
  TEST_ASSERT_EQUAL(0, extract_metar((char*)"", 0, m, 1));
}

void test_extract_metar_basic() {
  METAR m[1];
  char* s = (char*)"KSEA 10SM VCTS OVC046 09010KT";
  TEST_ASSERT_EQUAL(1, extract_metar(s, strlen(s), m, 1));
  TEST_ASSERT_EQUAL_STRING("KSEA", m[0].name);
  TEST_ASSERT_EQUAL(10, m[0].vis);
  TEST_ASSERT_EQUAL(4600, m[0].ceiling);
  TEST_ASSERT_EQUAL(DIR_E, m[0].wind_dir);
  TEST_ASSERT_EQUAL(10, m[0].wind_speed);
  TEST_ASSERT_EQUAL(0, m[0].gusts);
  TEST_ASSERT_EQUAL(LIGHTNING_VICINITY, m[0].lightning);
}

void test_extract_metar_variable_winds_gusts() {
  METAR m[1];
  char* s = (char*)"KSEA TS VRB123G45KT";
  TEST_ASSERT_EQUAL(1, extract_metar(s, strlen(s), m, 1));
  TEST_ASSERT_EQUAL(DIR_VARIABLE, m[0].wind_dir);
  TEST_ASSERT_EQUAL(123, m[0].wind_speed);
  TEST_ASSERT_EQUAL(45, m[0].gusts);
  TEST_ASSERT_EQUAL(LIGHTNING_HERE, m[0].lightning);
}

void test_extract_metar_multi() {
  METAR m[2];
  char* s = (char*)"KSEA 10SM OVC046\nTEST 5SM LTG DSNT BKN030\n\n";
  TEST_ASSERT_EQUAL(2, extract_metar(s, strlen(s), m, 2));
  TEST_ASSERT_EQUAL_STRING("KSEA", m[0].name);
  TEST_ASSERT_EQUAL(10, m[0].vis);
  TEST_ASSERT_EQUAL(4600, m[0].ceiling);
  TEST_ASSERT_EQUAL_STRING("TEST", m[1].name);
  TEST_ASSERT_EQUAL(5, m[1].vis);
  TEST_ASSERT_EQUAL(3000, m[1].ceiling);
  TEST_ASSERT_EQUAL(LIGHTNING_DISTANT, m[1].lightning);
}

void run_metar_tests() {
  RUN_TEST(test_to_wind_dir_n);
  RUN_TEST(test_to_wind_dir_ne);
  RUN_TEST(test_to_wind_dir_nw);
  RUN_TEST(test_to_wind_dir_s);
  RUN_TEST(test_to_wind_dir_se);
  RUN_TEST(test_to_wind_dir_sw);
  RUN_TEST(test_to_wind_dir_e);
  RUN_TEST(test_to_wind_dir_w);
  RUN_TEST(test_metar_overcast_intvis);
  RUN_TEST(test_metar_broken_floatvis);
  RUN_TEST(test_vis_zero);
  RUN_TEST(test_category_c_lifr);
  RUN_TEST(test_category_v_ifr);
  RUN_TEST(test_category_eq_mvfr);
  RUN_TEST(test_category_vfr);
  RUN_TEST(test_extract_metar_empty);
  RUN_TEST(test_extract_metar_basic);
  RUN_TEST(test_extract_metar_variable_winds_gusts);
  RUN_TEST(test_extract_metar_multi);
}
