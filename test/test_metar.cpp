#include <unity.h>
#include <stdio.h>
#include <string.h>

#include "metar.h"

void test_metar_overcast_intvis() {
  char* m = "KSEA 071453Z 16005KT 10SM SCT032 OVC046 04/02 A3014 RMK AO2 RAE50 SLP214 P0000 60001 T00390022 58004";
  METAR r;
  parse_metar(m, strlen(m), r);
  TEST_ASSERT_EQUAL(10, r.vis);
  TEST_ASSERT_EQUAL(4600, r.ceiling);
  TEST_ASSERT_EQUAL_STRING("KSEA", r.name);
}

void test_metar_broken_floatvis() {
  char* m = "KSEA 071453Z 16005KT 3/4SM+ SCT032 BKN046 04/02 A3014 RMK AO2 RAE50 SLP214 P0000 60001 T00390022 58004";
  METAR r;
  parse_metar(m, strlen(m), r);
  TEST_ASSERT_EQUAL(0, r.vis);
  TEST_ASSERT_EQUAL(4600, r.ceiling);
}

void test_vis_zero() {
  char* m = "KCLE 071551Z 33007KT 0SM -DZ BR OVC002 08/07 A3017 RMK AO2 SFC VIS 1 1/4 SLP234 P0000 T00780067";
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
  TEST_ASSERT_EQUAL(0, extract_metar("", 0, m, 1));
}

void test_extract_metar_basic() {
  METAR m[1];
  char* s = "asdfasdfasdf\n   <code>KSEA 10SM OVC046</code>";
  TEST_ASSERT_EQUAL(1, extract_metar(s, strlen(s), m, 1));
  TEST_ASSERT_EQUAL_STRING("KSEA", m[0].name);
  TEST_ASSERT_EQUAL(10, m[0].vis);
  TEST_ASSERT_EQUAL(4600, m[0].ceiling);
}

void test_extract_metar_multi() {
  METAR m[2];
  char* s = "asdfasdfasdf\n   <code>KSEA 10SM OVC046</code>\n   <code>TEST 5SM BKN030</code>\n\n";
  TEST_ASSERT_EQUAL(2, extract_metar(s, strlen(s), m, 2));
  TEST_ASSERT_EQUAL_STRING("KSEA", m[0].name);
  TEST_ASSERT_EQUAL(10, m[0].vis);
  TEST_ASSERT_EQUAL(4600, m[0].ceiling);
  TEST_ASSERT_EQUAL_STRING("TEST", m[1].name);
  TEST_ASSERT_EQUAL(5, m[1].vis);
  TEST_ASSERT_EQUAL(3000, m[1].ceiling);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_metar_overcast_intvis);
  RUN_TEST(test_metar_broken_floatvis);
  RUN_TEST(test_vis_zero);
  RUN_TEST(test_category_c_lifr);
  RUN_TEST(test_category_v_ifr);
  RUN_TEST(test_category_eq_mvfr);
  RUN_TEST(test_category_vfr);
  RUN_TEST(test_extract_metar_empty);
  RUN_TEST(test_extract_metar_basic);
  RUN_TEST(test_extract_metar_multi);
  UNITY_END();
}
