#include "stm32f1xx_hal.h"
#include "main_app.h"

#include "logging.h"
#include "UAV_Defines.h"
#include "interrupt_mgr.h"
#include "state_estimator.h"
#include "cmd_listener.h"
#include "controller.h"
#include "sensor_reader.h"
#include "led.h"
#include "device_ctrl.h"

#define LOG_TAG ("MainApp")

/*
* Defines
*/

#define MAIN_APP_DEBUG (0)
#if MAIN_APP_DEBUG
#define LOG(...) LOGI(__VA_ARGS__)
#else
#define LOG(...)
#endif

// test result, read data 13ms
//              estimate state 13ms
//              controller  38ms
//              listen cmd  13ms
//

// TODO modulus
/*
* Constants
*/

#define CORE_TIMER_FREQ 1000
#define TIMER_CNT_MAX 5000

#define READ_SENSOR_CNT 20 // 1000/50hz
#define ESTIMATE_STATE_CNT 20 // 1000/50hz
#define CONTROLL_CNT 50 // 1000/20hz
#define LISTEN_CMD_CNT 500 // 1000/2hz

/*
*Static
*/

static int sTimerCnt = 0;
static int sReadSensorCnt = READ_SENSOR_CNT;
static int sEstimateStateCnt = ESTIMATE_STATE_CNT;
static int sControllerCnt = CONTROLL_CNT;
static int sListenCmdCnt = LISTEN_CMD_CNT;
static volatile bool sReadSensorFlag = false;
static volatile bool sListenCmdFlag = false;
static volatile bool sControllerFlag = false;
static volatile bool sEstimateStateFlag = false;

static FCSensorMeasType sMeas;

static bool sStarted = false;
static bool sArmed = false;
static bool sTunePID = false;

/*
* Code
*/

static bool ToArm(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MIN && cmd.desiredAcc.y == CMD_ACC_MIN
        && cmd.desiredYawRate == CMD_YAW_RATE_MAX && cmd.desiredAcc.z == CMD_ACC_MIN) {
            return true;
        }
    return false;
#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MIN && cmd.desiredRoll == CMD_ROLL_MIN
        && cmd.desiredYawRate == CMD_YAW_RATE_MAX && cmd.desiredAccZ == CMD_ACC_MIN) {
            return true;
        }
    return false;
#endif
}

static bool ToDisArm(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MIN && cmd.desiredAcc.y == CMD_ACC_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MIN && cmd.desiredAcc.z == CMD_ACC_MIN) {
            return true;
        }
    return false;
#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MIN && cmd.desiredRoll == CMD_ROLL_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MIN && cmd.desiredAccZ == CMD_ACC_MIN) {
            return true;
        }
    return false;
#endif
}

static bool ToTunePID(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MIN && cmd.desiredAcc.y == CMD_ACC_MIN
        && cmd.desiredYawRate == CMD_YAW_RATE_MIN && cmd.desiredAcc.z == CMD_ACC_MIN) {
            return true;
        }
    return false;
#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MIN && cmd.desiredRoll == CMD_ROLL_MIN
        && cmd.desiredYawRate == CMD_YAW_RATE_MIN && cmd.desiredAccZ == CMD_ACC_MIN) {
            return true;
        }
    return false;
#endif
}

static bool ToExitTunePID(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MIN && cmd.desiredAcc.y == CMD_ACC_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MAX && cmd.desiredAcc.z == CMD_ACC_MIN) {
            return true;
        }
    return false;
#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MIN && cmd.desiredRoll == CMD_ROLL_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MIN && cmd.desiredAccZ == CMD_ACC_MIN) {
            return true;
        }
    return false;
#endif
}

static bool ToCalibrateESC(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MAX && cmd.desiredAcc.y == CMD_ACC_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MAX && cmd.desiredAcc.z == CMD_ACC_MAX) {
            return true;
        }
    return false;
#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MAX && cmd.desiredRoll == CMD_ROLL_MAX
        && cmd.desiredYawRate == CMD_YAW_RATE_MAX && cmd.desiredAccZ == CMD_ACC_MAX) {
            return true;
        }
    return false;
#endif
}

void MainApp_OnCoreTimerTick(void)
{
    if (!sStarted) return;
    ++sTimerCnt;
    if (sTimerCnt >= sReadSensorCnt) {
        sReadSensorCnt += READ_SENSOR_CNT;
        sReadSensorFlag = true;
    }
    if (sTimerCnt >= sEstimateStateCnt) {
        sEstimateStateCnt += ESTIMATE_STATE_CNT;
        sEstimateStateFlag = true;
    }
    if (sTimerCnt >= sListenCmdCnt) {
        sListenCmdCnt += LISTEN_CMD_CNT;
        sListenCmdFlag = true;
    }
    if (sTimerCnt >= sControllerCnt) {
        sControllerCnt += CONTROLL_CNT;
        sControllerFlag = true;
    }
    if (sTimerCnt >= TIMER_CNT_MAX) {
        sTimerCnt = 0;
        sReadSensorCnt = READ_SENSOR_CNT;
        sEstimateStateCnt = ESTIMATE_STATE_CNT;
        sListenCmdCnt = LISTEN_CMD_CNT;
        sControllerCnt = CONTROLL_CNT;
    }
}

static bool TunePID(FCCmdType& cmd)
{
#if UAV_CMD_ACC
    if (cmd.desiredAcc.x == CMD_ACC_MIN) {
        float curKp = Controller::GetInstance().mAttRateController_pitch.GetKp();
        LOGI("TunePID: Kp to %f\r\n", curKp - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKp(curKp - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredAcc.x == CMD_ACC_MAX) {
        float curKp = Controller::GetInstance().mAttRateController_pitch.GetKp();
        LOGI("TunePID: Kp to %f\r\n", curKp + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKp(curKp + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredAcc.y == CMD_ACC_MIN) {
        float curKd = Controller::GetInstance().mAttRateController_pitch.GetKd();
        LOGI("TunePID: Kd to %f\r\n", curKd - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKd - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredAcc.y == CMD_ACC_MAX) {
        float curKd = Controller::GetInstance().mAttRateController_pitch.GetKd();
        LOGI("TunePID: Kd to %\r\n", curKd + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKd + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredYawRate == CMD_YAW_RATE_MIN) {
        float curKi = Controller::GetInstance().mAttRateController_pitch.GetKi();
        LOGI("TunePID: Ki to %f\r\n", curKi - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKi - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredYawRate == CMD_YAW_RATE_MAX) {
        float curKi = Controller::GetInstance().mAttRateController_pitch.GetKd();
        LOGI("TunePID: Ki to %f\r\n", curKi + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKi + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }

    return true;

#elif UAV_CMD_ATT
    if (cmd.desiredPitch == CMD_PITCH_MIN) {
        float curKp = Controller::GetInstance().mAttController_pitch.GetKp();
        LOGI("TunePID: Kp to %f\r\n", curKp - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKp(curKp - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredPitch == CMD_PITCH_MAX) {
        float curKp = Controller::GetInstance().mAttController_pitch.GetKp();
        LOGI("TunePID: Kp to %f\r\n", curKp + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKp(curKp + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredRoll == CMD_ROLL_MIN) {
        float curKd = Controller::GetInstance().mAttController_pitch.GetKd();
        LOGI("TunePID: Kd to %f\r\n", curKd - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKd - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredRoll == CMD_ROLL_MAX) {
        float curKd = Controller::GetInstance().mAttController_pitch.GetKd();
        LOGI("TunePID: Kd to %\r\n", curKd + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKd + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredYawRate == CMD_YAW_RATE_MIN) {
        float curKi = Controller::GetInstance().mAttController_pitch.GetKi();
        LOGI("TunePID: Ki to %f\r\n", curKi - 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKi - 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }
    else if (cmd.desiredYawRate == CMD_YAW_RATE_MAX) {
        float curKi = Controller::GetInstance().mAttController_pitch.GetKi();
        LOGI("TunePID: Ki to %f\r\n", curKi + 0.1);
        Controller::GetInstance().mAttRateController_pitch.SetKd(curKi + 0.1);
        LED_Blink(LED_ONBOARD, 4);
    }

    return true;
#endif
}

void MainApp()
{
    bool res = DeviceInit();
    if (!res) {
        LOGE("MainApp failed to init device, try again\r\n");
        HAL_Delay(1000);
        res = DeviceInit();
    }

    if (!res) {
        LOGE("MainApp failed to init device, abort\r\n");
        LED_SetOn(LED_ONBOARD, false);
        return;
    }

    // set controller period
    Controller::GetInstance().SetPeriodMs(CONTROLL_CNT);
    // everything ready. Let's go.
    LOGI("MainApp starts\r\n");
    sStarted = true;
    LED_Blink(LED_ONBOARD, 4);
    while (1) {
        if (sReadSensorFlag) {
            sReadSensorFlag = false;
            LOG("readsensor : sTimerCnt = %d\r\n", sTimerCnt);
            SensorReader::GetInstance().GetSensorMeas(sMeas);
            LOG("readsensor: sTimerCnt = %d\r\n", sTimerCnt);
            LOG("sensor meas: gyro: %f %f %f, acc: %f %f %f\r\n", sMeas.gyroData.x, sMeas.gyroData.y, sMeas.gyroData.z, sMeas.accData.x, sMeas.accData.y, sMeas.accData.z);
        }
        if (sListenCmdFlag) {
            sListenCmdFlag = false;
            LOG("listencmd: sTimerCnt = %d\r\n", sTimerCnt);
            FCCmdType cmd;
            ReceiverStatus status = CmdListener::GetInstance().GetCmd(cmd);
            if (status != RECEIVER_FAIL) {
#if UAV_CMD_ACC
                LOGI("Cmd: acc.x %f acc.y %f acc.z %f, yawRate %f\r\n", cmd.desiredAcc.x, cmd.desiredAcc.y, cmd.desiredAcc.z, cmd.desiredYawRate);
#elif UAV_CMD_ATT
                LOGI("Cmd: pitch %f roll %f acc.z %f, yawRate %f\r\n", cmd.desiredPitch, cmd.desiredRoll, cmd.desiredAccZ, cmd.desiredYawRate);
#endif
                // Controller::GetInstance().SetAccSetpoint(cmd.desiredVel);
                if (!sArmed && (ToArm(cmd) || ToCalibrateESC(cmd))) {
                    sArmed = true;
                    MotorCtrl::GetInstance().StartMotor();
                    LOGI("MainApp: Armed!!!");
                    LED_SetOn(LED_ONBOARD, true);
                }
                else if (!sArmed && ToTunePID(cmd)) {
                    sTunePID = true;
                    LOGI("MainApp: Tuning PID!!!");
                    LED_SetOn(LED_ONBOARD, true);
                }
                else if (sTunePID && ToExitTunePID(cmd)) {
                    sTunePID = false;
                    LOGI("MainApp: Exit Tuning PID!!!");
                    LED_SetOn(LED_ONBOARD, false);
                }
                else if (sArmed && ToDisArm(cmd)) {
                    sArmed = false;
                    MotorCtrl::GetInstance().StopMotor();
                    LOGI("MainApp: DisArmed!!!");
                    LED_SetOn(LED_ONBOARD, false);
                }

                else if (sArmed) {
#if UAV_CMD_ACC
                    Controller::GetInstance().SetAccSetpoint(cmd.desiredAcc);
                    Controller::GetInstance().SetYawRateSetpoint(cmd.desiredYawRate);
#elif UAV_CMD_ATT
                    FCAttType attSetpoint;
                    attSetpoint.roll = cmd.desiredRoll;
                    attSetpoint.pitch = cmd.desiredPitch;
                    attSetpoint.yaw = 0; // yaw angle control is not used.
                    Controller::GetInstance().SetAttSetpoint(attSetpoint);

                    FCAccDataType accSetpoint;
                    accSetpoint.x = 0; // not used.
                    accSetpoint.y = 0; // not used.
                    accSetpoint.z = cmd.desiredAccZ;
                    Controller::GetInstance().SetAccSetpoint(accSetpoint);
                    Controller::GetInstance().SetYawRateSetpoint(cmd.desiredYawRate);
#endif
                }
                else if (sTunePID) {
                    TunePID(cmd);
                }
            } else {
                LOGE("sCmdListener.GetCmd returns fail, skip\r\n");
            }
            LOG("listencmd: sTimerCnt = %d\r\n", sTimerCnt);
        }
        if (sEstimateStateFlag) {
            sEstimateStateFlag = false;
            LOG("estimateState: sTimerCnt = %d\r\n", sTimerCnt);
            StateEstimator::GetInstance().EstimateState(sMeas);
            LOGI("Estimated State: roll %f, pitch %f, yaw %f, rollRate %f, pitchRate %f, yawRate %f\r\n", StateEstimator::GetInstance().mState.att.roll, StateEstimator::GetInstance().mState.att.pitch,
                 StateEstimator::GetInstance().mState.att.yaw, StateEstimator::GetInstance().mState.attRate.roll, StateEstimator::GetInstance().mState.attRate.pitch, StateEstimator::GetInstance().mState.attRate.yaw);
            Controller::GetInstance().SetCurAtt(StateEstimator::GetInstance().mState.att);
            Controller::GetInstance().SetCurAttRate(StateEstimator::GetInstance().mState.attRate);
            LOG("estimateState: sTimerCnt = %d\r\n", sTimerCnt);
        }
        if (sControllerFlag) {
            LOG("controller: sTimerCnt = %d\r\n", sTimerCnt);
            sControllerFlag = false;
            if (sArmed) Controller::GetInstance().Run();
            LOG("controller: sTimerCnt = %d\r\n", sTimerCnt);
        }
    }
}

