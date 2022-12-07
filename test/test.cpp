#include <unity.h>
#include "test.h"

int main() {
  UNITY_BEGIN();
  run_metar_tests();
  run_config_tests();
  UNITY_END();
}
