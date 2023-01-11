#include <unity.h>
#include <stdio.h>
#include <string.h>
#include "test.h"

#include "config.h"

void test_basic_cfg() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("SSID=testssid\nPASS=testpass\nABCD=0\nDEFG=1",fp);
  fclose(fp);

  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(2, c.num);
  TEST_ASSERT_EQUAL(0, c.locations[0].idx);
  TEST_ASSERT_EQUAL(1, c.locations[1].idx);
  TEST_ASSERT_EQUAL_STRING("testssid", c.ssid);
  TEST_ASSERT_EQUAL_STRING("testpass", c.pass);
  TEST_ASSERT_EQUAL_STRING("ABCD", c.locations[0].name);
  TEST_ASSERT_EQUAL_STRING("DEFG", c.locations[1].name);
}

void test_cfg_empty() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("",fp);
  fclose(fp);

  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(0, c.num);
  TEST_ASSERT_EQUAL_STRING("", c.ssid);
  TEST_ASSERT_EQUAL_STRING("", c.pass);
}

void test_cfg_newlines() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("\n\n\n",fp);
  fclose(fp);

  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(0, c.num);
}

void test_cfg_garbage() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("ASDFJSDFKJ=FJKDFJKDFJKD",fp);
  fclose(fp);
  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(0, c.num);
}
  
void test_cfg_longwifi() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("SSID=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "\nPASS=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",fp);
  fclose(fp);
  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL_STRING("", c.ssid);
  TEST_ASSERT_EQUAL_STRING("", c.pass);
}

void test_cfg_nonnumeric() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("ASDF=banana",fp);
  fclose(fp);
  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(0, c.num);
}

void test_cfg_override_color() {
  char path[] = "/tmp/fileXXXXXX";
  int fd = mkstemp(path);
  FILE* fp = fdopen(fd, "w");
  fputs("ABCD=0#FF8800\nDEFG=100#FFAAAA",fp);
  fclose(fp);

  Config c;
  read_config(c, path);
  TEST_ASSERT_EQUAL(2, c.num);
  TEST_ASSERT_EQUAL_STRING("ABCD", c.locations[0].name);
  TEST_ASSERT_EQUAL(0, c.locations[0].idx);
  TEST_ASSERT_EQUAL(0xff8800, c.locations[0].ovr);
  TEST_ASSERT_EQUAL_STRING("DEFG", c.locations[1].name);
  TEST_ASSERT_EQUAL(100, c.locations[1].idx);
  TEST_ASSERT_EQUAL(0xffaaaa, c.locations[1].ovr);
}

void run_config_tests() {
  RUN_TEST(test_basic_cfg);
  RUN_TEST(test_cfg_empty);
  RUN_TEST(test_cfg_override_color);
  RUN_TEST(test_cfg_newlines);
  RUN_TEST(test_cfg_garbage);
  RUN_TEST(test_cfg_nonnumeric);
  RUN_TEST(test_cfg_longwifi);
}

