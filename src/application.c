#include "application.h"

float computeTemperature(uint8_t first, uint8_t second){
  int TemperatureSum = (first << 8 | second) >> 4;
  return (float)TemperatureSum*0.0625;
}