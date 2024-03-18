#ifndef __SY7T609_REF_H__
#define __SY7T609_REF_H__

#define INVALID_DATA (0xFF)

typedef struct sy7t609_info {
    uint32_t power;
    uint32_t avg_power;
    uint32_t vrms;
    uint32_t irms;
    uint32_t pf;
    uint32_t eep_cnt;
    uint32_t eem_cnt;
} sy7t609_info_t;

void initSy7t609(void);
void sy7t609MeasurementProcess(void);
boolean sy7t609MeasurementFinish(void);

sy7t609_info_t getSy7t609Info(void);

#endif //__SY7T609_REF_H__
