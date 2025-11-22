#include "gps_parser.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "GPS_PARSER";
static gps_data_t gps_data = {0};

static double parse_coordinate(const char *coord_str, const char *direction) {
  if (!coord_str || strlen(coord_str) < 4)
    return 0.0;

  double coord = atof(coord_str);
  int degrees = (int)(coord / 100);
  double minutes = coord - (degrees * 100);
  double decimal_degrees = degrees + (minutes / 60.0);

  // Apply direction (N/E = positive, S/W = negative)
  if (direction && (*direction == 'S' || *direction == 'W')) {
    decimal_degrees = -decimal_degrees;
  }

  return decimal_degrees;
}

static void parse_gga(const char *sentence) {
  // $GPGGA,time,lat,N/S,lon,E/W,quality,num_sat,hdop,alt,M,alt_geoid,M,dgps_age,dgps_id*checksum
  char *tokens[15];
  char *str = strdup(sentence);
  char *token = strtok(str, ",");
  int i = 0;

  while (token && i < 15) {
    tokens[i++] = token;
    token = strtok(NULL, ",");
  }

  if (i >= 15) {
    // Parse latitude
    if (strlen(tokens[2]) > 0) {
      gps_data.latitude = parse_coordinate(tokens[2], tokens[3]);
    }

    // Parse longitude
    if (strlen(tokens[4]) > 0) {
      gps_data.longitude = parse_coordinate(tokens[4], tokens[5]);
    }

    // Parse quality (0=invalid, 1=GPS, 2=DGPS)
    int quality = atoi(tokens[6]);
    gps_data.valid = (quality > 0);

    // Parse number of satellites
    gps_data.satellites = atoi(tokens[7]);

    // Parse altitude
    if (strlen(tokens[9]) > 0) {
      gps_data.altitude = atof(tokens[9]);
    }

    // Parse timestamp
    if (strlen(tokens[1]) >= 6) {
      strncpy(gps_data.timestamp, tokens[1], 6);
      gps_data.timestamp[6] = '\0';
    }
  }

  free(str);
}

static void parse_rmc(const char *sentence) {
  // $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,mag_var,E/W*checksum
  char *tokens[13];
  char *str = strdup(sentence);
  char *token = strtok(str, ",");
  int i = 0;

  while (token && i < 13) {
    tokens[i++] = token;
    token = strtok(NULL, ",");
  }

  if (i >= 13) {
    // Parse status (A=active, V=void)
    gps_data.valid = (tokens[2][0] == 'A');

    // Parse speed (knots)
    if (strlen(tokens[7]) > 0) {
      gps_data.speed = atof(tokens[7]) * 1.852; // Convert knots to km/h
    }

    // Parse course
    if (strlen(tokens[8]) > 0) {
      gps_data.course = atof(tokens[8]);
    }

    // Parse date
    if (strlen(tokens[9]) >= 6) {
      strncpy(gps_data.date, tokens[9], 6);
      gps_data.date[6] = '\0';
    }
  }

  free(str);
}

void gps_parse_nmea(const char *nmea_sentence) {
  if (!nmea_sentence || strlen(nmea_sentence) < 6)
    return;

  // Check for valid NMEA sentence
  if (nmea_sentence[0] != '$')
    return;

  // Find checksum
  char *asterisk = strchr(nmea_sentence, '*');
  if (!asterisk)
    return;

  ESP_LOGD(TAG, "Parsing: %s", nmea_sentence);

  // Parse different sentence types
  if (strncmp(nmea_sentence, "$GPGGA", 6) == 0) {
    parse_gga(nmea_sentence);
  } else if (strncmp(nmea_sentence, "$GPRMC", 6) == 0) {
    parse_rmc(nmea_sentence);
  }
}

gps_data_t *gps_get_data(void) { return &gps_data; }

void gps_reset_data(void) { memset(&gps_data, 0, sizeof(gps_data)); }

bool gps_has_fix(void) { return gps_data.valid && gps_data.satellites >= 3; }

