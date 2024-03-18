#include <Arduino.h>

#include "sy7t609.h"
#include "sy7t609-uart-protocol.h"

#define CONTROL (0x048817)

#define VSCALE  (667000)
#define ISCALE  (31500)
#define PSCALE  (164145)

#define BUCKETH (0x1FD)
#define BUCKETL (0x28E9F8)

#define ACCUM   (670*5)

typedef enum process_state {
    PROCESS_STATE_READ_PF = 0,
    PROCESS_STATE_READ_VRMS,
    PROCESS_STATE_READ_IRMS,
    PROCESS_STATE_READ_POWER,
    PROCESS_STATE_READ_AVG_POWER,
    PROCESS_STATE_READ_EPPCNT,
    PROCESS_STATE_READ_EPMCNT,
	PROCESS_STATE_DELAY_1,
	PROCESS_STATE_DELAY_2,
    PROCESS_STATE_UPDATE_INFO
} process_state_t;

inline process_state_t& operator++(process_state_t& state) {
    state = static_cast<process_state_t>(state + 1);
    return state;
}

inline process_state_t& operator++(process_state_t& state, int) {
    process_state_t old = state;
    state = static_cast<process_state_t>(state + 1);
    return old;
}

static sy7t609_info_t s_info;

static process_state_t s_process_state = PROCESS_STATE_READ_PF;

static void updateInfo(sy7t609_info_t info);
static bool resetSy7t609(void);
static bool setControlRegister(void);
static bool setScale(void);
static bool setBucket(void);
static bool setAccum(void);
static bool clearEnergyCounters(void);
static uint32_t readPF(void);
static uint32_t readVRMS(void);
static uint32_t readIRMS(void);
static uint32_t readPOWER(void);
static uint32_t readAvgPower(void);
static uint32_t readEPPCNT(void);
static uint32_t readEPMCNT(void);

sy7t609_info_t getSy7t609Info(void)
{
	return s_info;
}

void initSy7t609(void)
{
	// emberAfAppPrintln("%s", __func__);
    resetSy7t609();
    delay(10);
    setBucket();
    clearEnergyCounters();
}

void sy7t609MeasurementProcess(void)
{
    static sy7t609_info_t tmp;

    switch(s_process_state) {
    case PROCESS_STATE_READ_PF:
    	tmp.pf = readPF();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_VRMS:
    	tmp.vrms = readVRMS();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_IRMS:
    	tmp.irms = readIRMS();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_POWER:
    	tmp.power = readPOWER();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_AVG_POWER:
    	tmp.avg_power = readAvgPower();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_EPPCNT:
    	tmp.eep_cnt = readEPPCNT();
        s_process_state++;
        break;
    case PROCESS_STATE_READ_EPMCNT:
    	tmp.eem_cnt = readEPMCNT();
        s_process_state++;
        break;
    case PROCESS_STATE_DELAY_1:
    	s_process_state++;
    	break;
    case PROCESS_STATE_DELAY_2:
    	s_process_state++;
    	break;
    case PROCESS_STATE_UPDATE_INFO:
        // sy7t609Callback(tmp);
        s_process_state = PROCESS_STATE_READ_PF;
        break;
    default :
        s_process_state = PROCESS_STATE_READ_PF;
        break;
    }
}

static uint32_t readPF(void)
{
    uint32_t data;

    if (readRegister(ADDR_PF, &data) == false) {
        data = INVALID_DATA;
    } else {
		if(data >= 0x800000)
		{
			data = 0x01000000 - data;
		}
    }

    return data;
}

static uint32_t readVRMS(void)
{
    uint32_t data;

    if (readRegister(ADDR_VRMS, &data) == false) {
        data = INVALID_DATA;
    }

    return data;
}

static uint32_t readIRMS(void)
{
    uint32_t data;

    if (readRegister(ADDR_IRMS, &data) == false) {
        data = INVALID_DATA;
    } else {
    	data = (uint32_t)(data/128);
    }

    return data;
}

static uint32_t readPOWER(void)
{
    uint32_t data;

    if (readRegister(ADDR_POWER, &data) == false) {
        data = INVALID_DATA;
    } else {
		if(data >= 0x800000) {
			data = 0x01000000 - data;
		}
    }

    return data;
}

static uint32_t readAvgPower(void)
{
    uint32_t data;

    if (readRegister(ADDR_AVG_POWER, &data) == false) {
        data = INVALID_DATA;
    }

    return data;
}

static uint32_t readEPPCNT(void)
{
    uint32_t data;

    if (readRegister(ADDR_EPPCNT, &data) == false) {
        data = INVALID_DATA;
    }

    return data;
}

static uint32_t readEPMCNT(void)
{
    uint32_t data;

    if (readRegister(ADDR_EPMCNT, &data) == false) {
        data = INVALID_DATA;
    }

    return data;
}

static void updateInfo(sy7t609_info_t info)
{
	memcpy(&s_info, &info, sizeof(info));
}

static bool setControlRegister(void)
{
    if (writeRegister(ADDR_CONTROL, CONTROL) == false) {
        return false;
    }

    return true;
}

static bool setScale(void)
{
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

static bool setBucket(void)
{
    if (writeRegister(ADDR_BUCKETL, BUCKETL) == false) {
        return false;
    }

    if (writeRegister(ADDR_BUCKETH, BUCKETH) == false) {
        return false;
    }

    return true;
}

static bool setAccum(void)
{
    if (writeRegister(ADDR_ACCUM, ACCUM) == false) {
        return false;
    }

    return true;
}

static bool clearEnergyCounters(void)
{
    if (writeRegister(ADDR_COMMAND, 
                      CMD_REG_CLEAR_ENGERGY_COUNTERS) == false) {
        return false;
    }

    return true;
}

static bool resetSy7t609(void)
{
    if (writeRegister(ADDR_COMMAND, 
                      CMD_REG_SOFT_RESET) == false) {
        return false;
    }

    return true;
}

boolean sy7t609MeasurementFinish(void)
{
    return (s_process_state == PROCESS_STATE_UPDATE_INFO);
}