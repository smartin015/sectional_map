#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define LED_PIN 6
#define LED_NUMPIXELS 16

struct LocationConfig {
  char* name;
  int idx;
};


const LocationConfig LOCATIONS[] = {
  {"KRNT", 0},
  {"KSEA", 1}, 
  {"KPLU", 2}, 
  {"KOLM", 3}, 
  {"KTIW", 4}, 
  {"KPWT", 5}, 
  {"KSHN", 6}, 
};

int    HTTP_PORT   = 80;
String HTTP_METHOD = "GET"; // or "POST"
char   HOST_NAME[] = "example.phpoc.com"; // hostname of web server:
String PATH_NAME   = "";

#define ssid "robotoverlords"
#define password "oakdale43"

String loc_csv = "";

void setup() {
  loc_csv = LOCATIONS[0].name;
  for (int i = 1; i < sizeof(LOCATIONS); i++) {
    loc_csv += ",";
    loc_csv += LOCATIONS[i].name;
  }
  Serial.println(loc_csv);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

struct METAR {
  char* name;
  char* vis;
  char* viscat;
  ceiling
  ceilingcat
};

METAR results[sizeof(LOCATIONS)];


Category get_category(char* metar, uint16_t len) {
  Category viz = INVALID;
  Category ceiling = INVALID;

  char* saveptr;
  char* ptr = strtok_r(metar, " ", &saveptr);
  while (ptr != NULL) {
    char* eptr = strchr(ptr, " ");

    // Visibility ends in "SM" (statute miles)
    if (strncmp(eptr-2, "SM", 2) == 0) {
      if (memchr(ptr, '/', eptr-ptr) != NULL) {
        // Fractional visibility is limited IFR
        viz = LIFR;
        continue;
      } 
      int sm = strtol(ptr, eptr-2, 10);       
      if (sm < 3) {
        viz = IFR;
      } else if (sm <= 5) {
        viz = MVFR;
      } else {
        viz = VFR;
      }
      continue;
    }

    // Cloud ceiling 

    ptr = strtok_r(NULL, " ", &saveptr);
  }
  components = metar.split(' ');
  // Get visibility by looking for entry ending in "SM" (statute miles)
  // Value can either be a fraction (e.g. "3/4SM") or a number ("5SM")
  match = re.search('( [0-9] )?([0-9]/?[0-9]?SM)', metar)
  if(match == None):
    return INVALID

  (g1, g2) = match.groups()
  if(g2 == None):
    return 'INVALID'
  if(g1 != None):
    return 'IFR'
  if '/' in g2:
    return 'LIFR'
  vis = int(re.sub('SM','',g2))
  if vis < 3:
    return 'IFR'
  if vis <=5 :
    return 'MVFR'
  return 'VFR'
  
  // Get ceiling
  minimum_ceiling = 10000
  for component in components:
    if 'BKN' in component or 'OVC' in component:
      ceiling = int(filter(str.isdigit,component)) * 100
      if(ceiling < minimum_ceiling):
        minimum_ceiling = ceiling
  return minimum_ceiling

  if ceiling < CEIL_LIFR:
    return 'LIFR'
  if ceiling < CEIL_IFR:
    return 'IFR'
  if ceiling < CEIL_MVFR:
    return 'MVFR'
  return 'VFR'

  if (ceiling == "INVALID") {
    return INVALID
  }
  if(vis == 'LIFR' or ceiling == 'LIFR'):
    return 'LIFR'
  if(vis == 'IFR' or ceiling == 'IFR'):
    return 'IFR'
  if(vis == 'MVFR' or ceiling == 'MVFR'):
    return 'MVFR'
  return 'VFR'

  return INVALID;
}

int get_metar(String airportCSV) {
  if(WiFi.status() != WL_CONNECTED){
    return -1;
  }

  HTTPClient http;
  http.begin("http://www.aviationweather.gov/metar/data?ids="+airportCSV+"&format=raw&hours=0&taf=off&layout=off&date=0");
  int rc = http.GET();
  if (rc>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(rc);
    String payload = http.getString();
    char* ptr;
    char* saveptr;
    char *cstr = new char[payload.length() + 1];
    strcpy(cstr, payload.c_str());
    ptr = strtok_r(cstr, "\n", &saveptr);
    while (ptr != NULL) {
      if (strncmp(ptr, "<code>", 6) == 0) {
        // e.g. <code>KSEA 062320Z 14006KT 10SM BKN013 OVC020 05/03 A3013 RMK AO2 T00500028</code><br/>
        Serial.println("Found code section");
      }
      ptr = strtok(NULL, "\n", &saveptr);
    }
  }
  else {
    Serial.print("Error code: ");
    Serial.println(rc);
  }
  // Free resources
  http.end();
}

void loop() {
  delay(5000);
}
