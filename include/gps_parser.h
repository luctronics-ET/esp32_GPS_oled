#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  bool valid;
  double latitude;
  double longitude;
  float altitude;
  uint8_t satellites;
  float speed;
  float course;
  char timestamp[10]; // HHMMSS
  char date[7];       // DDMMYY
} gps_data_t;

// Function prototypes
void gps_parse_nmea(const char *nmea_sentence);
gps_data_t *gps_get_data(void);
void gps_reset_data(void);
bool gps_has_fix(void);

