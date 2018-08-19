#ifndef HAL_IMU_H_
#define HAL_IMU_H_

#include <UAV_Defines.h>

#include "MPU9250.h"

typedef void (*DataReadyCb)(void);

class IMU
{
private:
    MPU9250 mIMU;

    float gravity[3];
    float magConst[3];
    float gyroBias[3];
    float mx_centre;
    float my_centre;
    float mz_centre;

    // flags
    bool mReadyToStart;
    bool mSensorBiasCalibrateFlag;
    bool mGyroAccDataRdyFlag;
    bool mMagCalibrateFlag;

    //FCSensorDataType gyroData;
    //FCSensorDataType accData;
    //FCSensorDataType magData;
    //FCSensorDataType rawGyroData;
    //FCSensorDataType rawAccData;
    //FCSensorDataType rawMagData;
public:
    //cb
    DataReadyCb mDataReadyCb;

    IMU();

    void SetGyroAccDataReadyFlg();
    static void OnGyroAccDataReady(void* pIMU);
    bool Init();
    bool Start();
    bool SetDataReadyCb(DataReadyCb cb);
    void CalibrateSensorBias();
    void CalibrateMag();
    void GetGravityVector(float* pGravity);
    void GetMagConstVector(float* pMagConst);
    bool GetRawCompassData(FCSensorDataType* pMagData);
    bool GetCompassData(FCSensorDataType* pMagData);
    void GetAccelData(FCSensorDataType* pAccData);
    void GetRawGyroData(FCSensorDataType* pGyroData);
    void GetGyroData(FCSensorDataType* pGyroData);
};



#endif