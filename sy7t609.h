#ifndef __SY7T609_REF_H__
#define __SY7T609_REF_H__

#include <Arduino.h>

#define INVALID_DATA (-1)

typedef struct sy7t609_info {
  uint32_t power;
  uint32_t avg_power;
  uint32_t vrms;
  uint32_t irms;
  uint32_t pf;
  uint32_t freq;
  uint32_t epp_cnt;
  uint32_t eem_cnt;
} sy7t609_info_t;

uint8_t initSy7t609(void);
bool clearEnergyCounters(void);
void sy7t609MeasurementProcess(void);

sy7t609_info_t getSy7t609Info(void);

void reportErr(String name, size_t errCode);

#endif  //__SY7T609_REF_H__
