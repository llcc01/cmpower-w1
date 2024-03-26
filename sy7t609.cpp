#include "sy7t609.h"

#include <Arduino.h>

#include "sy7t609-uart-protocol.h"

#define CONTROL (0x048817)

#define VSCALE (667000)
#define ISCALE (31500)
#define PSCALE (164145)

#define BUCKETH (0x1FD)
#define BUCKETL (0x28E9F8)

#define ACCUM (670 * 5)

sy7t609_info_t s_info;

void updateInfo(sy7t609_info_t info);
void stopAutoReport(void);
bool resetSy7t609(void);
bool setControlRegister(void);
bool setScale(void);
bool setBucket(void);
bool setAccum(void);
bool clearEnergyCounters(void);
uint32_t readPF(void);
uint32_t readVRMS(void);
uint32_t readIRMS(void);
uint32_t readPOWER(void);
uint32_t readAvgPower(void);
uint32_t readEPPCNT(void);
uint32_t readEPMCNT(void);
uint32_t readFrequency(void);

sy7t609_info_t getSy7t609Info(void) { return s_info; }

uint8_t initSy7t609(void) {
  // emberAfAppPrintln("%s", __func__);
  Serial.setTimeout(50);
  if (!resetSy7t609()) {
    return 1;
  }

  delay(100);
  stopAutoReport();
  delay(100);

  if (!setBucket()) {
    return 2;
  }
  if (!clearEnergyCounters()) {
    return 3;
  }
  return 0;
}

void sy7t609MeasurementProcess(void) {
  sy7t609_info_t tmp;

  tmp.pf = readPF();
  tmp.vrms = readVRMS();
  tmp.irms = readIRMS();
  tmp.power = readPOWER();
  tmp.avg_power = readAvgPower();
  tmp.freq = readFrequency();
  tmp.epp_cnt = readEPPCNT();
  tmp.epm_cnt = readEPMCNT();
  updateInfo(tmp);
}

uint32_t readPF(void) {
  uint32_t data;

  if (readRegister(ADDR_PF, &data) == false) {
    data = INVALID_DATA;
  } else {
    if (data >= 0x800000) {
      data = 0x01000000 - data;
    }
  }

  return data;
}

uint32_t readVRMS(void) {
  uint32_t data;

  if (readRegister(ADDR_VRMS, &data) == false) {
    data = INVALID_DATA;
  }

  return data;
}

uint32_t readIRMS(void) {
  uint32_t data;

  if (readRegister(ADDR_IRMS, &data) == false) {
    data = INVALID_DATA;
  } else {
    // data = (uint32_t)(data / 128);
  }

  return data;
}

uint32_t readPOWER(void) {
  uint32_t data;

  if (readRegister(ADDR_POWER, &data) == false) {
    data = INVALID_DATA;
  } else {
    if (data >= 0x800000) {
      data = 0x01000000 - data;
    }
  }

  return data;
}

uint32_t readAvgPower(void) {
  uint32_t data;

  if (readRegister(ADDR_AVG_POWER, &data) == false) {
    data = INVALID_DATA;
  }

  return data;
}

uint32_t readEPPCNT(void) {
  uint32_t data;

  if (readRegister(ADDR_EPPCNT, &data) == false) {
    data = INVALID_DATA;
  }

  return data;
}

uint32_t readEPMCNT(void) {
  uint32_t data;

  if (readRegister(ADDR_EPMCNT, &data) == false) {
    data = INVALID_DATA;
  }

  return data;
}

uint32_t readFrequency(void) {
  uint32_t data;

  if (readRegister(ADDR_FREQUENCY, &data) == false) {
    data = INVALID_DATA;
  }

  return data;
}

void updateInfo(sy7t609_info_t info) { memcpy(&s_info, &info, sizeof(info)); }

bool setControlRegister(void) {
  if (writeRegister(ADDR_CONTROL, CONTROL) == false) {
    return false;
  }

  return true;
}

bool setScale(void) {
  if (writeRegister(ADDR_VSCALE, VSCALE) == false) {
    return false;
  }

  if (writeRegister(ADDR_ISCALE, ISCALE) == false) {
    return false;
  }

  if (writeRegister(ADDR_PSCALE, PSCALE) == false) {
    return false;
  }

  return true;
}

bool setBucket(void) {
  if (writeRegister(ADDR_BUCKETL, BUCKETL) == false) {
    return false;
  }

  if (writeRegister(ADDR_BUCKETH, BUCKETH) == false) {
    return false;
  }

  return true;
}

bool setAccum(void) {
  if (writeRegister(ADDR_ACCUM, ACCUM) == false) {
    return false;
  }

  return true;
}

bool clearEnergyCounters(void) {
  if (writeRegister(ADDR_COMMAND, CMD_REG_CLEAR_ENGERGY_COUNTERS) == false) {
    return false;
  }

  return true;
}

bool resetSy7t609(void) {
  if (writeRegister(ADDR_COMMAND, CMD_REG_SOFT_RESET) == false && 0) {
    return false;
  }

  return true;
}

void stopAutoReport(void) {
  writeRegister(ADDR_COMMAND, CMD_REF_AUTO_REPORTING_DISABLE);
}
