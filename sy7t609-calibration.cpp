#include "sy7t609-calibration.h"

#include <Arduino.h>

#include "sy7t609-uart-protocol.h"
#include "sy7t609.h"

//===== nx custom value ======
#define VGAIN_MIN (1000000)
#define VGAIN_MAX (3000000)
#define VGAIN_AVG (2159919)
#define IGAIN_MIN (1000000)
#define IGAIN_MAX (3000000)
#define IGAIN_AVG (2192594)

#define CONTROL (0x048817)

#define BUCKETH (0x1FD)
#define BUCKETL (0x28E9F8)

#define ACCUM (670 * 5)

#define VSCALE (667000)
#define ISCALE (31500)
#define PSCALE (164145)
//============================

#define RETRY_COUNT (3)

typedef enum process_state {
  PROCESS_STATE_SET_CONTROL_REGISTER = 0,
  PROCESS_STATE_SET_SCALE,
  PROCESS_STATE_SET_ACCUM,
  PROCESS_STATE_CALIBRATION,
  PROCESS_STATE_WAIT,
  PROCESS_STATE_COMPLETE
} process_state_t;

process_state_t s_state;

void calibrationProcessHandler(void);

uint32_t vrms_target;
uint32_t irms_target;

void printInfo(void);
bool setControlRegisterAndVerify(void);
bool setScaleAndVerify(void);
bool setAccumAndVerify(void);
void setAvgGain(void);
bool isCalibrationComplete();
bool startCalibration(uint32_t v_target, uint32_t i_target);
bool saveToFlash(void);

void start(void);
void stop(void);

void printInfo(void) {
  // emberAfAppPrintln("sy7t609-calibration-info");
}

void calibrationProcessHandler(void) {
  bool result;

  switch (s_state) {
    case PROCESS_STATE_SET_CONTROL_REGISTER:
      result = setControlRegisterAndVerify();
      result = setScaleAndVerify();
      result = setAccumAndVerify();
      result = saveToFlash();

      if (result == false) {
        s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
        // emberEventControlSetInactive(calibration_event_control);
      } else {
        s_state = PROCESS_STATE_CALIBRATION;
        // emberEventControlSetDelayMS(calibration_event_control, 500);;
      }
      break;
    case PROCESS_STATE_CALIBRATION:
      result = startCalibration(vrms_target, irms_target);
      if (result == false) {
        s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
        // emberEventControlSetInactive(calibration_event_control);
      } else {
        s_state = PROCESS_STATE_WAIT;
        // emberEventControlSetActive(calibration_event_control);
      }
      break;
    case PROCESS_STATE_WAIT:
      result = isCalibrationComplete();

      if (result == false) {
        s_state = PROCESS_STATE_WAIT;
        // emberEventControlSetDelayMS(calibration_event_control, 1000);
      } else {
        s_state = PROCESS_STATE_COMPLETE;
        // emberEventControlSetActive(calibration_event_control);
      }
      break;
    case PROCESS_STATE_COMPLETE:
      saveToFlash();
      s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
      // emberEventControlSetInactive(calibration_event_control);
      break;
    default:
      s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
      // emberEventControlSetInactive(calibration_event_control);
      break;
  }
}

bool setControlRegisterAndVerify(void) {
  // emberAfAppPrintln("%s", __func__);
  uint32_t data;

  if (writeRegister(ADDR_CONTROL, CONTROL) == false) {
    // emberAfAppPrintln("[Fail] can not write Control Register");
    return false;
  }

  if (readRegister(ADDR_CONTROL, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read Control Register");
    return false;
  } else {
    if (data == CONTROL) {
      // emberAfAppPrintln("=== [1/1] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch Control Register");
      return false;
    }
  }

  // emberAfAppPrintln("[Success] %s", __func__);
  return true;
}

bool setScaleAndVerify(void) {
  // emberAfAppPrintln("%s", __func__);
  uint32_t data;

  if (writeRegister(ADDR_VSCALE, VSCALE) == false) {
    // emberAfAppPrintln("[Fail] can not write VSCALE");
    return false;
  }

  if (readRegister(ADDR_VSCALE, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read VSCALE");
    return false;
  } else {
    if (data == VSCALE) {
      // emberAfAppPrintln("=== [1/3] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch VSCALE");
      return false;
    }
  }

  if (writeRegister(ADDR_ISCALE, ISCALE) == false) {
    // emberAfAppPrintln("[Fail] can not write ISCALE");
    return false;
  }

  if (readRegister(ADDR_ISCALE, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read ISCALE");
    return false;
  } else {
    if (data == ISCALE) {
      // emberAfAppPrintln("=== [2/3] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch ISCALE");
      return false;
    }
  }

  if (writeRegister(ADDR_PSCALE, PSCALE) == false) {
    // emberAfAppPrintln("[Fail] can not write PSCALE");
    return false;
  }

  if (readRegister(ADDR_PSCALE, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read PSCALE");
    return false;
  } else {
    if (data == PSCALE) {
      // emberAfAppPrintln("=== [3/3] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch PSCALE");
      return false;
    }
  }

  // emberAfAppPrintln("[Success] %s", __func__);
  return true;
}

bool setAccumAndVerify(void) {
  // emberAfAppPrintln("%s", __func__);
  uint32_t data;

  if (writeRegister(ADDR_ACCUM, ACCUM) == false) {
    // emberAfAppPrintln("[Fail] can not write ACCUM");
    return false;
  }

  if (readRegister(ADDR_ACCUM, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read ACCUM");
    return false;
  } else {
    if (data == ACCUM) {
      // emberAfAppPrintln("=== [1/1] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch ACCUM");
      return false;
    }
  }

  // emberAfAppPrintln("[Success] %s", __func__);
  return true;
}

void setAvgGain(void) {
  // emberAfAppPrintln("%s", __func__);
  uint32_t data;

  if (writeRegister(ADDR_VGAIN, VGAIN_AVG) == false) {
    // emberAfAppPrintln("[Fail] can not write VGAIN_AVG");
    return;
  }

  if (readRegister(ADDR_VGAIN, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read VGAIN_AVG");
    return;
  } else {
    if (data == VGAIN_AVG) {
      // emberAfAppPrintln("=== [1/2] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch VGAIN_AVG");
      return;
    }
  }

  if (writeRegister(ADDR_IGAIN, IGAIN_AVG) == false) {
    // emberAfAppPrintln("[Fail] can not write IGAIN_AVG");
    return;
  }

  if (readRegister(ADDR_IGAIN, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read IGAIN_AVG");
    return;
  } else {
    if (data == IGAIN_AVG) {
      // emberAfAppPrintln("=== [2/2] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch IGAIN_AVG");
      return;
    }
  }

  // emberAfAppPrintln("[Success] %s", __func__);
  return;
}

bool isCalibrationComplete(void) {
  uint32_t data;

  if (readRegister(ADDR_COMMAND, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read ADDR_COMMAND");
    return false;
  } else {
    if ((data & COMMAND_REGISTER_CALIBRATION_MASK) == 0) {
      // emberAfAppPrintln("[success]");
    } else {
      // emberAfAppPrintln("[Fail] wait a second please...");
      return false;
    }
  }

  return true;
}

bool startCalibration(uint32_t mV, uint32_t mA) {
  // emberAfAppPrintln("%s", __func__);

  uint32_t data;
  uint32_t v_target = mV;
  uint32_t i_target = mA * 128;

  if (writeRegister(ADDR_VRMS_TARGET, v_target) == false) {
    // emberAfAppPrintln("[Fail] can not write VRMS_TARGET");
    return false;
  }

  if (readRegister(ADDR_VRMS_TARGET, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read VRMS_TARGET");
    return false;
  } else {
    if (data == v_target) {
      // emberAfAppPrintln("=== [1/3] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch VRMS_TARGET");
      return false;
    }
  }

  if (writeRegister(ADDR_IRMS_TARGET, i_target) == false) {
    // emberAfAppPrintln("[Fail] can not write IRMS_TARGET");
    return false;
  }

  if (readRegister(ADDR_IRMS_TARGET, &data) == false) {
    // emberAfAppPrintln("[Fail] can not read IRMS_TARGET");
    return false;
  } else {
    if (data == i_target) {
      // emberAfAppPrintln("=== [2/3] ===");
    } else {
      // emberAfAppPrintln("[Fail] mismatch IRMS_TARGET");
      return false;
    }
  }

  if (writeRegister(ADDR_COMMAND, CMD_REG_CALIBRATION_ALL) == false) {
    // emberAfAppPrintln("[Fail] can not request calibration");
    return false;
  }

  return true;
}

bool saveToFlash(void) {
  if (writeRegister(ADDR_COMMAND, CMD_REG_SAVE_TO_FLASH) == false) {
    // emberAfAppPrintln("[Fail] can not request save");
    return false;
  }

  // emberAfAppPrintln("[Success] %s", __func__);
  return true;
}

void start(void) {
  // emberEventControlSetActive(calibration_event_control);
  s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
}

void stop(void) {
  // emberEventControlSetInactive(calibration_event_control);
  s_state = PROCESS_STATE_SET_CONTROL_REGISTER;
}
