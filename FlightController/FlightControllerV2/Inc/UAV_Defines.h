#ifndef _UAV_DEFINES_
#define _UAV_DEFINES_

/*
 * This file defines global constants that all files need to use.
 */

/*
 * Defines
 */

#define UAV_Debug (0)

#define UAV_ENABLE_MOTORS (1)

// toggle whether the RC value controls attitude or acceleration
#define UAV_CMD_ATT_RATE (1) // in this mode, uesr directly control UAV's attitude rate
#define UAV_CMD_ATT (0) // in this mode, user directly controls UAV's attitude
#define UAV_CMD_ACC (0) // in this mode, user controls UAV's movement in xyz direction according to map coordinate

#define UAV_G (9.7760)
#define UAV_PI (3.1415926)
#define UAV_RADIANS_TO_DEGREE (57.2957805)
#define UAV_DEGREE_TO_RADIAN (0.01745329)

#define PID_ATT_KP_PITCH (0.0f)
#define PID_ATT_KD_PITCH (0.0f)
#define PID_ATT_KI_PITCH (0.0f)
#define PID_ATT_KP_ROLL (0.0f)
#define PID_ATT_KD_ROLL (0.0f)
#define PID_ATT_KI_ROLL (0.0f)
#define PID_ATT_RATE_KP_PITCH (0.35f)
#define PID_ATT_RATE_KD_PITCH (0.0f) // 0.015
#define PID_ATT_RATE_KI_PITCH (0.0f)
#define PID_ATT_RATE_KP_ROLL (0.35f)
#define PID_ATT_RATE_KD_ROLL (0.0f) // 0.015
#define PID_ATT_RATE_KI_ROLL (0.0f)
#define PID_ATT_RATE_KP_YAW (0.05f)
#define PID_ATT_RATE_KD_YAW (0.0f)
#define PID_ATT_RATE_KI_YAW (0.0f)
#define PID_VEL_KP (1.0f)
#define PID_VEL_KD (0.0f)
#define PID_VEL_KI (0.0f)
#define VEL_SETPOINT_TO_MOTOR_THRUST_KP (400)

#define UAV_MAX_ACC_X (0.5)
#define UAV_MAX_ACC_Y (0.5)
#define UAV_MAX_ACC_Z (0.5)
#define UAV_MAX_VEL_X (0.5)
#define UAV_MAX_VEL_Y (0.5)
#define UAV_MAX_VEL_Z (0.5)

#define UAV_PWM_HOVER_DUTYCYCLE (400) // To Confirm
//#define UAV_PWM_MIN_DUTYCYCLE (0) // to prevent propeller from stopping // To Confirm
//#define UAV_PWM_MAX_DUTYCYCLE (100) // to prevent propeller from stopping // To Confirm

// pwm output to motor. we want 1000 steps between 0ms to 1ms pulse width
#define UAV_MOTOR_MIN_DUTYCYCLE (0)
#define UAV_MOTOR_MAX_DUTYCYCLE (1000)

// unit m/s*2
#define CMD_ACC_MIN (-1.0f)
#define CMD_ACC_MAX (1.0f)

// uunit radian
#define CMD_PITCH_MIN (-20.0f)
#define CMD_PITCH_MAX (20.0f)
#define CMD_ROLL_MIN (-20.0f)
#define CMD_ROLL_MAX (20.0f)

// unit dps
#define CMD_ROLL_RATE_MIN (-45.0f)
#define CMD_ROLL_RATE_MAX (45.0f)
#define CMD_PITCH_RATE_MIN (-45.0f)
#define CMD_PITCH_RATE_MAX (45.0f)
#define CMD_YAW_RATE_MIN (-90.0f)
#define CMD_YAW_RATE_MAX (90.0f)

// thrust limit
#define UAV_THRUST_MIN (-125)
#define UAV_THRUST_MAX (125)

/*
 * Struct
 */

typedef struct {
   float yaw;
   float pitch;
   float roll;
} FCAttType;

typedef FCAttType FCAttRateType;

typedef struct {
   int motor1PWM;
   int motor2PWM;
   int motor3PWM;
   int motor4PWM;
} FCMotorPWMType;

typedef struct {
   float x;
   float y;
   float z;
} FCSensorDataType;

typedef FCSensorDataType FCAccDataType;
typedef FCSensorDataType FCVelDataType;

typedef struct {
   float q1;
   float q2;
   float q3;
   float q4;
} FCQuaternionType;

typedef struct {
    FCSensorDataType gyroData;
    FCSensorDataType accData;
    // FCSensorDataType magData;
} FCSensorMeasType;

typedef struct {
    FCAttType att;
    FCAttRateType attRate;
} FCStateType;

#if UAV_CMD_ATT
typedef struct {
    float desiredAccZ;
    float desiredPitchRate;
    float desiredRollRate;
    float desiredYawRate;
    bool toTunePID;
} FCCmdType;
#endif

#if UAV_CMD_ATT_RATE
typedef struct {
    float desiredAccZ;
    FCAttRateType desiredAttRate;
    bool toTunePID;
} FCCmdType;
#endif

#if UAV_CMD_ACC
typedef struct {
    FCAccDataType desiredAcc;
    float desiredYawRate;
    bool toTunePID;
} FCCmdType;
#endif

#endif