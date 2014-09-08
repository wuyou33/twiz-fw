/**
   From : https://github.com/kriswiner/STM32F401
   TODO : see licensing restrictions and directives !
*/

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

#include "mpu9150.h"
#include "i2c_wrapper.h"
#include "nrf_delay.h"
#include "printf.h"
#include "nordic_common.h"
#include "timers.h"

// Define registers per MPU6050, Register Map and Descriptions, Rev 4.2, 08/19/2013 6 DOF Motion sensor fusion device
// Invensense Inc., www.invensense.com
// See also MPU-9150 Register Map and Descriptions, Revision 4.0, RM-MPU-9150A-00, 9/12/2012 for registers not listed in
// above document; the MPU6050 and MPU 9150 are virtually identical but the latter has an on-board magnetic sensor
//
//Magnetometer Registers
#define AK8975A_ADDRESS  0x0C

#define WHO_AM_I_AK8975A 0x00 // should return 0x48
#define INFO             0x01
#define AK8975A_ST1      0x02  // data ready status bit 0
#define AK8975A_XOUT_L   0x03  // data
#define AK8975A_XOUT_H   0x04
#define AK8975A_YOUT_L   0x05
#define AK8975A_YOUT_H   0x06
#define AK8975A_ZOUT_L   0x07
#define AK8975A_ZOUT_H   0x08
#define AK8975A_ST2      0x09  // Data overflow bit 3 and data read error status bit 2
#define AK8975A_CNTL     0x0A  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
#define AK8975A_ASTC     0x0C  // Self test control
#define AK8975A_ASAX     0x10  // Fuse ROM x-axis sensitivity adjustment value
#define AK8975A_ASAY     0x11  // Fuse ROM y-axis sensitivity adjustment value
#define AK8975A_ASAZ     0x12  // Fuse ROM z-axis sensitivity adjustment value

#define XGOFFS_TC        0x00 // Bit 7 PWR_MODE, bits 6:1 XG_OFFS_TC, bit 0 OTP_BNK_VLD
#define YGOFFS_TC        0x01
#define ZGOFFS_TC        0x02
#define X_FINE_GAIN      0x03 // [7:0] fine gain
#define Y_FINE_GAIN      0x04
#define Z_FINE_GAIN      0x05
#define XA_OFFSET_H      0x06 // User-defined trim values for accelerometer
#define XA_OFFSET_L_TC   0x07
#define YA_OFFSET_H      0x08
#define YA_OFFSET_L_TC   0x09
#define ZA_OFFSET_H      0x0A
#define ZA_OFFSET_L_TC   0x0B
#define SELF_TEST_X      0x0D
#define SELF_TEST_Y      0x0E
#define SELF_TEST_Z      0x0F
#define SELF_TEST_A      0x10
#define XG_OFFS_USRH     0x13  // User-defined trim values for gyroscope, populate with calibration routine
#define XG_OFFS_USRL     0x14
#define YG_OFFS_USRH     0x15
#define YG_OFFS_USRL     0x16
#define ZG_OFFS_USRH     0x17
#define ZG_OFFS_USRL     0x18
#define SMPLRT_DIV       0x19
#define CONFIG           0x1A
#define GYRO_CONFIG      0x1B
#define ACCEL_CONFIG     0x1C
#define FF_THR           0x1D  // Free-fall
#define FF_DUR           0x1E  // Free-fall
#define MOT_THR          0x1F  // Motion detection threshold bits [7:0]
#define MOT_DUR          0x20  // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define ZMOT_THR         0x21  // Zero-motion detection threshold bits [7:0]
#define ZRMOT_DUR        0x22  // Duration counter threshold for zero motion interrupt generation, 16 Hz rate, LSB = 64 ms
#define FIFO_EN          0x23
#define I2C_MST_CTRL     0x24
#define I2C_SLV0_ADDR    0x25
#define I2C_SLV0_REG     0x26
#define I2C_SLV0_CTRL    0x27
#define I2C_SLV1_ADDR    0x28
#define I2C_SLV1_REG     0x29
#define I2C_SLV1_CTRL    0x2A
#define I2C_SLV2_ADDR    0x2B
#define I2C_SLV2_REG     0x2C
#define I2C_SLV2_CTRL    0x2D
#define I2C_SLV3_ADDR    0x2E
#define I2C_SLV3_REG     0x2F
#define I2C_SLV3_CTRL    0x30
#define I2C_SLV4_ADDR    0x31
#define I2C_SLV4_REG     0x32
#define I2C_SLV4_DO      0x33
#define I2C_SLV4_CTRL    0x34
#define I2C_SLV4_DI      0x35
#define I2C_MST_STATUS   0x36
#define INT_PIN_CFG      0x37
#define INT_ENABLE       0x38
#define DMP_INT_STATUS   0x39  // Check DMP interrupt
#define INT_STATUS       0x3A
#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40
#define TEMP_OUT_H       0x41
#define TEMP_OUT_L       0x42
#define GYRO_XOUT_H      0x43
#define GYRO_XOUT_L      0x44
#define GYRO_YOUT_H      0x45
#define GYRO_YOUT_L      0x46
#define GYRO_ZOUT_H      0x47
#define GYRO_ZOUT_L      0x48
#define EXT_SENS_DATA_00 0x49
#define EXT_SENS_DATA_01 0x4A
#define EXT_SENS_DATA_02 0x4B
#define EXT_SENS_DATA_03 0x4C
#define EXT_SENS_DATA_04 0x4D
#define EXT_SENS_DATA_05 0x4E
#define EXT_SENS_DATA_06 0x4F
#define EXT_SENS_DATA_07 0x50
#define EXT_SENS_DATA_08 0x51
#define EXT_SENS_DATA_09 0x52
#define EXT_SENS_DATA_10 0x53
#define EXT_SENS_DATA_11 0x54
#define EXT_SENS_DATA_12 0x55
#define EXT_SENS_DATA_13 0x56
#define EXT_SENS_DATA_14 0x57
#define EXT_SENS_DATA_15 0x58
#define EXT_SENS_DATA_16 0x59
#define EXT_SENS_DATA_17 0x5A
#define EXT_SENS_DATA_18 0x5B
#define EXT_SENS_DATA_19 0x5C
#define EXT_SENS_DATA_20 0x5D
#define EXT_SENS_DATA_21 0x5E
#define EXT_SENS_DATA_22 0x5F
#define EXT_SENS_DATA_23 0x60
#define MOT_DETECT_STATUS 0x61
#define I2C_SLV0_DO      0x63
#define I2C_SLV1_DO      0x64
#define I2C_SLV2_DO      0x65
#define I2C_SLV3_DO      0x66
#define I2C_MST_DELAY_CTRL 0x67
#define SIGNAL_PATH_RESET  0x68
#define MOT_DETECT_CTRL   0x69
#define USER_CTRL        0x6A  // Bit 7 enable DMP, bit 3 reset DMP
#define PWR_MGMT_1       0x6B // Device defaults to the SLEEP mode
#define PWR_MGMT_2       0x6C
#define DMP_BANK         0x6D  // Activates a specific bank in the DMP
#define DMP_RW_PNT       0x6E  // Set read/write pointer to a specific start address in specified DMP bank
#define DMP_REG          0x6F  // Register in DMP from which to read or to which to write
#define DMP_REG_1        0x70
#define DMP_REG_2        0x71
#define FIFO_COUNTH      0x72
#define FIFO_COUNTL      0x73
#define FIFO_R_W         0x74
#define WHO_AM_I_MPU9150 0x75 // Should return 0x68


// On the TWI, ADO is set to 0
#define MPU9150_ADDRESS 0x68

// Set initial input parameters
#define AFS_2G  0
#define AFS_4G  1
#define AFS_8G  2
#define AFS_16G 3

#define  GFS_250DPS  0
#define  GFS_500DPS  1
#define  GFS_1000DPS 2
#define  GFS_2000DPS 3

static uint8_t Ascale = AFS_2G;     // AFS_2G, AFS_4G, AFS_8G, AFS_16G
static uint8_t Gscale = GFS_250DPS; // GFS_250DPS, GFS_500DPS, GFS_1000DPS, GFS_2000DPS
static bool calibrated = false;

// Pin definitions
// XXX FIXME int intPin = 12;  // These can be changed, 2 and 3 are the Arduinos ext int pins

static float magCalibration[3] = {0, 0, 0}, magBias[3] = {0, 0, 0};  // Factory mag calibration and mag bias
static float gyroBias[3] = {0, 0, 0}, accelBias[3] = {0, 0, 0}; // Bias corrections for gyro and accelerometer
static float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
static float temperature;

// parameters for 6 DoF sensor fusion calculations
static float PI = 3.14159265358979323846f;
// gyroscope measurement error in rads/s (start at 60 deg/s), then reduce after ~10 s to 3
// PI * (60.0f / 180.0f);
static float GyroMeasError = 1.0471975511965976f;
// compute beta = sqrt(3.0f / 4.0f) * GyroMeasError;
static float beta = 0.3068996821171088f; // XXX FIXME 0.9
// gyroscope measurement drift in rad/s/s (start at 0.0 deg/s/s)
// PI * (1.0f / 180.0f)
static float GyroMeasDrift = 0.017453292519943295f;
// compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value
// zeta = sqrt(3.0f / 4.0f) * GyroMeasDrift;
static float zeta = 0.015114994701951814f;
// these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback,
// Ki for integral
#define Kp (2.0f * 5.0f)
#define Ki (0.0f)

static float pitch, yaw, roll;
// integration interval for both filter schemes
static float deltat = 0.0f;

// vector to hold quaternion
static float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};
// vector to hold integral error for Mahony method
static float eInt[3] = {0.0f, 0.0f, 0.0f};

void ak8975a_init()
{
    // Check if magnometer is online
    uint8_t whoami = i2c_read_byte(AK8975A_ADDRESS, WHO_AM_I_AK8975A);
    printf("AK8975A : I am 0x%x\n\r", whoami);

    if (whoami != 0x48) {
        // WHO_AM_I should be 0x48
        printf("ERROR : I SHOULD BE 0x48\n\r");
        while(1) ;
    }
    printf("AK8975A is online...\n\r");

    // Power down mode
    i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x00);
    nrf_delay_ms(10);
}

static int ak8975a_read_raw_data(int16_t *data)
{
    static bool running = false;
    static uint8_t rawData[6];  // x/y/z gyro register data stored here
    static int ret = 0;

    if (running) {
        // If there is a data available
        if (i2c_read_byte(AK8975A_ADDRESS, AK8975A_ST1) & 0x01) {
            // If there is no overflow
            if((i2c_read_byte(AK8975A_ADDRESS, AK8975A_ST2) & 0x0C)==0) {
                // Read the six raw data registers sequentially into data array
                i2c_read_bytes(AK8975A_ADDRESS, AK8975A_XOUT_L, 6, rawData);
                // Turn the MSB and LSB into a signed 16-bit value
                data[0] = ((int16_t)rawData[1])*256 | rawData[0];
                data[1] = ((int16_t)rawData[3])*256 | rawData[2];
                data[2] = ((int16_t)rawData[5])*256 | rawData[4];
                ret = 0;
            }
            else
                ret = -1;
            // Launch a new acquisition
            i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x01);
            return ret;
        }
    }
    else {
        // Else launch the first acquisition
        i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x01);
        running = true;
    }
    return -1;
}

static void ak8975a_read_data(float *mx, float *my, float *mz)
{
    static int16_t data[3];
    while(ak8975a_read_raw_data(data) != 0) ;
    *mx = (data[0] - magBias[0])*magCalibration[0];
    *my = (data[1] - magBias[1])*magCalibration[1];
    *mz = (data[2] - magBias[2])*magCalibration[2];
}

void ak8975a_calibrate()
{
#if 1
    // Get and store factory trim values
    uint8_t rawData[3];
    // Power down
    i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x00);
    nrf_delay_ms(1);
    // Enter Fuse ROM access mode
    i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x0F);
    nrf_delay_ms(10);
    // Read the x-, y-, and z-axis calibration values
    i2c_read_bytes(AK8975A_ADDRESS, AK8975A_ASAX, 3, rawData);
    // Back to power down mode
    i2c_write_byte(AK8975A_ADDRESS, AK8975A_CNTL, 0x00);

    magCalibration[0] =  (float)(rawData[0] - 128)/256.0f + 1.0f; // Return x-axis sensitivity adjustment values
    magCalibration[1] =  (float)(rawData[1] - 128)/256.0f + 1.0f;
    magCalibration[2] =  (float)(rawData[2] - 128)/256.0f + 1.0f;

    // Calibrate for hard iron : for some times, user is asked to move the device in
    // all directions. We record the min / max values, then compute the halfsum which
    // will be our offset.

    int meas_count = 2000;
    static int16_t data[3];
    static int xmin = 32767, ymin =32767, zmin = 32767;
    static int xmax = -327678, ymax = -32768, zmax = -32768;

    // Read a lot a values, hoping that the users moves the TWI in all possible directions
    // Data include already the factory trim correction.
    for(int i=0; i<meas_count; i++) {
        if (ak8975a_read_raw_data(data) != 0) {
            i--;
            continue;
        }

#if 1
        for(int j=0; j<3; j++)
            printf("%d ", (int)data[j]);
        printf("\r\n");
#endif

        // Keep track of min and max along each axis
        xmin = MIN(xmin, data[0]);
        xmax = MAX(xmax, data[0]);

        ymin = MIN(ymin, data[1]);
        ymax = MAX(ymax, data[1]);

        zmin = MIN(zmin, data[2]);
        zmax = MAX(zmax, data[2]);
    }

#if 1
        printf("xmin=%d, xmax=%d, ymin=%d, ymax=%d, zmin=%d, zmax=%d\r\n", xmin, xmax, ymin, ymax, zmin, zmax);
#endif

        // Now calculate biases
        magBias[0] = (xmin + xmax)/2.;
        magBias[1] = (ymin + ymax)/2.;
        magBias[2] = (zmin + zmax)/2.;

        // And update calibration data so that max = 180
        magCalibration[0] = 360./(xmax - xmin);
        magCalibration[1] = 360./(ymax - ymin);
        magCalibration[2] = 360./(zmax - zmin);

#if 1
        printf("Mag calibration values :\r\n");
        printf("\txbias = %f, ybias = %f, zbias = %f\r\n", magBias[0], magBias[1], magBias[2]);
        printf("\txcal = %f, ycal = %f, zcal = %f\r\n", magCalibration[0], magCalibration[1], magCalibration[2]);
#endif

#else
        // Valeur de calibration sur la table de la A06 :)
        // xbias = 7.000000, ybias = 29.500000, zbias = -142.000000
        // xcal = 1.113281, ycal = 1.121094, zcal = 1.175781
        magBias[0] = 7.000000;
        magBias[1] = 29.500000;
        magBias[2] = -142.000000;
        magCalibration[0] = 1.113281;
        magCalibration[1] = 1.121094;
        magCalibration[2] = 1.175781;
#endif

    }

 static void mpu9150_reset() {
     // Write a one to bit 7 reset bit; toggle reset device
     i2c_write_byte(MPU9150_ADDRESS, PWR_MGMT_1, 0x80);
     while(i2c_read_byte(MPU9150_ADDRESS, PWR_MGMT_1) & 0x80) ;
     nrf_delay_ms(200);
 }

 void mpu9150_init()
 {
     // Take MPU9150 out of sleep
     i2c_write_byte(MPU9150_ADDRESS, PWR_MGMT_1, 0x00);
     // Delay 100ms for gyro startup
     nrf_delay_ms(100);

     // Set clock source to be PLL with x-axis gyroscope reference, bits 2:0 = 001
     i2c_write_byte(MPU9150_ADDRESS, PWR_MGMT_1, 0x01);
     i2c_write_byte(MPU9150_ADDRESS, PWR_MGMT_2, 0x00);
     nrf_delay_ms(100);

     // Disable I2C master mode, disable FIFO
     i2c_write_byte(MPU9150_ADDRESS, I2C_MST_CTRL, 0x00);
     i2c_write_byte(MPU9150_ADDRESS, FIFO_EN, 0x00);
     // Reset sensors PATH and registers and FIFO
     i2c_write_byte(MPU9150_ADDRESS, USER_CTRL, 0x5);
     while(i2c_read_byte(MPU9150_ADDRESS, USER_CTRL) & 0x05) ;

     // Configure Gyro and Accelerometer
     // Disable FSYNC and set accelerometer and gyro bandwidth to 44 and 42 Hz, respectively;
     // DLPF_CFG = bits 2:0 = 010; this sets the sample rate at 1 kHz for both
     // Maximum delay is 4.9 ms which is just over a 200 Hz maximum rate
     i2c_write_byte(MPU9150_ADDRESS, CONFIG, 0x01); // XXXX FIXME : 0x03

     // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
     i2c_write_byte(MPU9150_ADDRESS, SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; the same rate set in CONFIG above

     // Set gyroscope full scale range
     // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
     uint8_t c =  i2c_read_byte(MPU9150_ADDRESS, GYRO_CONFIG);
     i2c_write_byte(MPU9150_ADDRESS, GYRO_CONFIG, c & ~0xE0); // Clear self-test bits [7:5]
     i2c_write_byte(MPU9150_ADDRESS, GYRO_CONFIG, c & ~0x18); // Clear AFS bits [4:3]
     i2c_write_byte(MPU9150_ADDRESS, GYRO_CONFIG, c | Gscale << 3); // Set full scale range for the gyro

     // Set accelerometer configuration
     c =  i2c_read_byte(MPU9150_ADDRESS, ACCEL_CONFIG);
     i2c_write_byte(MPU9150_ADDRESS, ACCEL_CONFIG, c & ~0xE0); // Clear self-test bits [7:5]
     i2c_write_byte(MPU9150_ADDRESS, ACCEL_CONFIG, c & ~0x18); // Clear AFS bits [4:3]
     i2c_write_byte(MPU9150_ADDRESS, ACCEL_CONFIG, c | Ascale << 3); // Set full scale range for the accelerometer

     // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
     // but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

     // Configure Interrupts and Bypass Enable
     // Set interrupt pin active high, push-pull, latched and clear on read of INT_STATUS,
     // Enable I2C_BYPASS_EN so magnetometer can join the I2C bus and can be controlled
     // by the TWI as master
     i2c_write_byte(MPU9150_ADDRESS, INT_PIN_CFG, 0x22);
     i2c_write_byte(MPU9150_ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
 }

 // Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
 // of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
 void mpu9150_calibrate()
 {
     static uint8_t data[14]; // data array to hold accelerometer and gyro x, y, z, data
     int ii, meas_count;
     int32_t gyro_bias[3] = {0, 0, 0}, accel_bias[3] = {0, 0, 0};
     static int32_t gyro_temp[3];
     static int32_t accel_temp[3];

     printf("Calibrating MPU9150. Please don't move !\r\n");

     // If already calibrated, then reset chip to reset HW cal register to factory trim
     if(calibrated) {
         mpu9150_reset();
         mpu9150_init();
     }
     calibrated = true;

     // Configure MPU9150 gyro and accelerometer for bias calculation
     i2c_write_byte(MPU9150_ADDRESS, GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
     i2c_write_byte(MPU9150_ADDRESS, ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity
     nrf_delay_ms(200);

     uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
     uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

     // Accumulate 200 measures each 5 ms (200Hz sample rate)
     meas_count = 200;
     for (int i = 0; i<meas_count; i++) {
         nrf_delay_ms(5);

         // Burst read to ensure that accel, temp and gyro measurements are taken at the same time
         i2c_read_bytes(MPU9150_ADDRESS, ACCEL_XOUT_H, 14, data);

         // Debug
#if 1
         for (int j=0; j<14; j=j+2)
             printf("%02x%02x ", data[j], data[j+1]);
         printf("\r\n");
#endif
         // Form signed 16-bit integer for each data
         accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;
         accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
         accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;
         gyro_temp[0]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
         gyro_temp[1]  = (int16_t) (((int16_t)data[10] << 8) | data[11]  ) ;
         gyro_temp[2]  = (int16_t) (((int16_t)data[12] << 8) | data[13]) ;

         // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
         accel_bias[0] += (int32_t) accel_temp[0];
         accel_bias[1] += (int32_t) accel_temp[1];
         accel_bias[2] += (int32_t) accel_temp[2];
         gyro_bias[0]  += (int32_t) gyro_temp[0];
         gyro_bias[1]  += (int32_t) gyro_temp[1];
         gyro_bias[2]  += (int32_t) gyro_temp[2];

     }
     // Normalize sums to get average count biases
     accel_bias[0] /= (int32_t) meas_count;
     accel_bias[1] /= (int32_t) meas_count;
     accel_bias[2] /= (int32_t) meas_count;
     gyro_bias[0]  /= (int32_t) meas_count;
     gyro_bias[1]  /= (int32_t) meas_count;
     gyro_bias[2]  /= (int32_t) meas_count;

     // Remove gravity from the z-axis accelerometer bias calculation
     if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) accelsensitivity;}
     else {accel_bias[2] += (int32_t) accelsensitivity;}

     // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
     data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
     data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
     data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
     data[3] = (-gyro_bias[1]/4)       & 0xFF;
     data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
     data[5] = (-gyro_bias[2]/4)       & 0xFF;

     /// Push gyro biases to hardware registers
     i2c_write_byte(MPU9150_ADDRESS, XG_OFFS_USRH, data[0]);
     i2c_write_byte(MPU9150_ADDRESS, XG_OFFS_USRL, data[1]);
     i2c_write_byte(MPU9150_ADDRESS, YG_OFFS_USRH, data[2]);
     i2c_write_byte(MPU9150_ADDRESS, YG_OFFS_USRL, data[3]);
     i2c_write_byte(MPU9150_ADDRESS, ZG_OFFS_USRH, data[4]);
     i2c_write_byte(MPU9150_ADDRESS, ZG_OFFS_USRL, data[5]);

     // Construct gyro bias in deg/s for later manual subtraction
     gyroBias[0] = (float) gyro_bias[0]/(float) gyrosensitivity;
     gyroBias[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
     gyroBias[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

     // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
     // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
     // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
     // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
     // the accelerometer biases calculated above must be divided by 8.

     int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
     i2c_read_bytes(MPU9150_ADDRESS, XA_OFFSET_H, 2, data); // Read factory accelerometer trim values
     accel_bias_reg[0] = (int16_t) ((int16_t)data[0] << 8) | data[1];
     i2c_read_bytes(MPU9150_ADDRESS, YA_OFFSET_H, 2, data);
     accel_bias_reg[1] = (int16_t) ((int16_t)data[0] << 8) | data[1];
     i2c_read_bytes(MPU9150_ADDRESS, ZA_OFFSET_H, 2, data);
     accel_bias_reg[2] = (int16_t) ((int16_t)data[0] << 8) | data[1];

     uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
     uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

     for(ii = 0; ii < 3; ii++)
         // If temperature compensation bit is set, record that fact in mask_bit
         mask_bit[ii] = accel_bias_reg[ii] & mask;

     // Construct total accelerometer bias, including calculated average accelerometer bias from above
     accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
     accel_bias_reg[1] -= (accel_bias[1]/8);
     accel_bias_reg[2] -= (accel_bias[2]/8);

     data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
     data[1] = (accel_bias_reg[0])      & 0xFF;
     data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
     data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
     data[3] = (accel_bias_reg[1])      & 0xFF;
     data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
     data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
     data[5] = (accel_bias_reg[2])      & 0xFF;
     data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

     // Push accelerometer biases to hardware registers
     i2c_write_byte(MPU9150_ADDRESS, XA_OFFSET_H, data[0]);
     i2c_write_byte(MPU9150_ADDRESS, XA_OFFSET_L_TC, data[1]);
     i2c_write_byte(MPU9150_ADDRESS, YA_OFFSET_H, data[2]);
     i2c_write_byte(MPU9150_ADDRESS, YA_OFFSET_L_TC, data[3]);
     i2c_write_byte(MPU9150_ADDRESS, ZA_OFFSET_H, data[4]);
     i2c_write_byte(MPU9150_ADDRESS, ZA_OFFSET_L_TC, data[5]);

     // Output scaled accelerometer biases for manual subtraction in the main program
     accelBias[0] = (float)accel_bias[0]/(float)accelsensitivity;
     accelBias[1] = (float)accel_bias[1]/(float)accelsensitivity;
     accelBias[2] = (float)accel_bias[2]/(float)accelsensitivity;

     printf("x gyro bias = %f\n\r", gyroBias[0]);
     printf("y gyro bias = %f\n\r", gyroBias[1]);
     printf("z gyro bias = %f\n\r", gyroBias[2]);
     printf("x accel bias = %f\n\r", accelBias[0]);
     printf("y accel bias = %f\n\r", accelBias[1]);
     printf("z accel bias = %f\n\r", accelBias[2]);
     printf("\r\n");

     printf("Calibrating MPU9150 done.\r\n");
 }


 // Accelerometer and gyroscope self test; check calibration wrt factory settings
 // Should return percent deviation from factory trim values, +/- 14 or less deviation is a pass
 void mpu9150_selftest()
 {
     static uint8_t rawData[4] = {0, 0, 0, 0};
     static uint8_t selfTest[6];
     static float factoryTrim[6];

     // Configure the accelerometer for self-test
     i2c_write_byte(MPU9150_ADDRESS, ACCEL_CONFIG, 0xF0); // Enable self test on all three axes and set accelerometer range to +/- 8 g
     i2c_write_byte(MPU9150_ADDRESS, GYRO_CONFIG,  0xE0); // Enable self test on all three axes and set gyro range to +/- 250 degrees/s
     nrf_delay_ms(500);  // Delay a while to let the device execute the self-test
     rawData[0] = i2c_read_byte(MPU9150_ADDRESS, SELF_TEST_X); // X-axis self-test results
     rawData[1] = i2c_read_byte(MPU9150_ADDRESS, SELF_TEST_Y); // Y-axis self-test results
     rawData[2] = i2c_read_byte(MPU9150_ADDRESS, SELF_TEST_Z); // Z-axis self-test results
     rawData[3] = i2c_read_byte(MPU9150_ADDRESS, SELF_TEST_A); // Mixed-axis self-test results
     // Extract the acceleration test results first
     selfTest[0] = (rawData[0] >> 3) | (rawData[3] & 0x30) >> 4 ; // XA_TEST result is a five-bit unsigned integer
     selfTest[1] = (rawData[1] >> 3) | (rawData[3] & 0x0C) >> 4 ; // YA_TEST result is a five-bit unsigned integer
     selfTest[2] = (rawData[2] >> 3) | (rawData[3] & 0x03) >> 4 ; // ZA_TEST result is a five-bit unsigned integer
     // Extract the gyration test results first
     selfTest[3] = rawData[0]  & 0x1F ; // XG_TEST result is a five-bit unsigned integer
     selfTest[4] = rawData[1]  & 0x1F ; // YG_TEST result is a five-bit unsigned integer
     selfTest[5] = rawData[2]  & 0x1F ; // ZG_TEST result is a five-bit unsigned integer
     // Process results to allow final comparison with factory set values
     factoryTrim[0] = (4096.0f*0.34f)*(pow( (0.92f/0.34f) , ((selfTest[0] - 1.0f)/30.0f))); // FT[Xa] factory trim calculation
     factoryTrim[1] = (4096.0f*0.34f)*(pow( (0.92f/0.34f) , ((selfTest[1] - 1.0f)/30.0f))); // FT[Ya] factory trim calculation
     factoryTrim[2] = (4096.0f*0.34f)*(pow( (0.92f/0.34f) , ((selfTest[2] - 1.0f)/30.0f))); // FT[Za] factory trim calculation
     factoryTrim[3] =  ( 25.0f*131.0f)*(pow( 1.046f , (selfTest[3] - 1.0f) ));             // FT[Xg] factory trim calculation
     factoryTrim[4] =  (-25.0f*131.0f)*(pow( 1.046f , (selfTest[4] - 1.0f) ));             // FT[Yg] factory trim calculation
     factoryTrim[5] =  ( 25.0f*131.0f)*(pow( 1.046f , (selfTest[5] - 1.0f) ));             // FT[Zg] factory trim calculation

     // Report results as a ratio of (STR - FT)/FT; the change from Factory Trim of the Self-Test Response
     // To get to percent, must multiply by 100 and subtract result from 100
     for (int i = 0; i < 6; i++) {
         selfTest[i] = 100.0f + 100.0f*(selfTest[i] - factoryTrim[i])/factoryTrim[i]; // Report percent differences
     }

     printf("x-axis self test: acceleration trim within %2.2f%% of factory value\n\r", selfTest[0]);
     printf("y-axis self test: acceleration trim within %2.2f%% of factory value\n\r", selfTest[1]);
     printf("z-axis self test: acceleration trim within %2.2f%% of factory value\n\r", selfTest[2]);
     printf("x-axis self test: gyration trim within %2.2f%% of factory value\n\r", selfTest[3]);
     printf("y-axis self test: gyration trim within %2.2f%% of factory value\n\r", selfTest[4]);
     printf("z-axis self test: gyration trim within %2.2f%% of factory value\n\r", selfTest[5]);

     // Re-init MPU
     mpu9150_init();
 }

 // Read accel, temps and gyro values.
 static void mpu9150_read_data(int16_t * values)
 {
     static uint8_t data[14];

     // Burst read all sensors to ensure the same timestamp for everybody
     i2c_read_bytes(MPU9150_ADDRESS, ACCEL_XOUT_H, 14, data);

     // Convert each 2 byte into signed 16bit values
     for(int i=0; i<7; i++)
         values[i] = (int16_t)(((int16_t)data[2*i] << 8) | data[2*i+1]) ;

#if 0
     for (int j=0; j<7; j++)
         printf("%08x ", values[j]);
     printf("\r\n");
#endif
 }



 // Implementation of Sebastian Madgwick's "...efficient orientation filter for... inertial/magnetic sensor arrays"
 // (see http://www.x-io.co.uk/category/open-source/ for examples and more details)
 // which fuses acceleration, rotation rate, and magnetic moments to produce a quaternion-based estimate of absolute
 // device orientation -- which can be converted to yaw, pitch, and roll. Useful for stabilizing quadcopters, etc.
 // The performance of the orientation filter is at least as good as conventional Kalman-based filtering algorithms
 // but is much less computationally intensive---it can be performed on a 3.3 V Pro Mini operating at 8 MHz!
 void madgwick_quaternion_update(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
 {
     float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
     float norm;
     float hx, hy, _2bx, _2bz;
     float s1, s2, s3, s4;
     float qDot1, qDot2, qDot3, qDot4;

     // Auxiliary variables to avoid repeated arithmetic
     float _2q1mx;
     float _2q1my;
     float _2q1mz;
     float _2q2mx;
     float _4bx;
     float _4bz;
     float _2q1 = 2.0f * q1;
     float _2q2 = 2.0f * q2;
     float _2q3 = 2.0f * q3;
     float _2q4 = 2.0f * q4;
     float _2q1q3 = 2.0f * q1 * q3;
     float _2q3q4 = 2.0f * q3 * q4;
     float q1q1 = q1 * q1;
     float q1q2 = q1 * q2;
     float q1q3 = q1 * q3;
     float q1q4 = q1 * q4;
     float q2q2 = q2 * q2;
     float q2q3 = q2 * q3;
     float q2q4 = q2 * q4;
     float q3q3 = q3 * q3;
     float q3q4 = q3 * q4;
     float q4q4 = q4 * q4;

     // Normalise accelerometer measurement
     norm = sqrt(ax * ax + ay * ay + az * az);
     if (norm == 0.0f) return; // handle NaN
     norm = 1.0f/norm;
     ax *= norm;
     ay *= norm;
     az *= norm;

     // Normalise magnetometer measurement
     norm = sqrt(mx * mx + my * my + mz * mz);
     if (norm == 0.0f) return; // handle NaN
     norm = 1.0f/norm;
     mx *= norm;
     my *= norm;
     mz *= norm;

     // Reference direction of Earth's magnetic field
     _2q1mx = 2.0f * q1 * mx;
     _2q1my = 2.0f * q1 * my;
     _2q1mz = 2.0f * q1 * mz;
     _2q2mx = 2.0f * q2 * mx;
     hx = mx * q1q1 - _2q1my * q4 + _2q1mz * q3 + mx * q2q2 + _2q2 * my * q3 + _2q2 * mz * q4 - mx * q3q3 - mx * q4q4;
     hy = _2q1mx * q4 + my * q1q1 - _2q1mz * q2 + _2q2mx * q3 - my * q2q2 + my * q3q3 + _2q3 * mz * q4 - my * q4q4;
     _2bx = sqrt(hx * hx + hy * hy);
     _2bz = -_2q1mx * q3 + _2q1my * q2 + mz * q1q1 + _2q2mx * q4 - mz * q2q2 + _2q3 * my * q4 - mz * q3q3 + mz * q4q4;
     _4bx = 2.0f * _2bx;
     _4bz = 2.0f * _2bz;

     // Gradient decent algorithm corrective step
     s1 = -_2q3 * (2.0f * q2q4 - _2q1q3 - ax) + _2q2 * (2.0f * q1q2 + _2q3q4 - ay) - _2bz * q3 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q4 + _2bz * q2) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q3 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
     s2 = _2q4 * (2.0f * q2q4 - _2q1q3 - ax) + _2q1 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q2 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + _2bz * q4 * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q3 + _2bz * q1) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q4 - _4bz * q2) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
     s3 = -_2q1 * (2.0f * q2q4 - _2q1q3 - ax) + _2q4 * (2.0f * q1q2 + _2q3q4 - ay) - 4.0f * q3 * (1.0f - 2.0f * q2q2 - 2.0f * q3q3 - az) + (-_4bx * q3 - _2bz * q1) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (_2bx * q2 + _2bz * q4) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + (_2bx * q1 - _4bz * q3) * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
     s4 = _2q2 * (2.0f * q2q4 - _2q1q3 - ax) + _2q3 * (2.0f * q1q2 + _2q3q4 - ay) + (-_4bx * q4 + _2bz * q2) * (_2bx * (0.5f - q3q3 - q4q4) + _2bz * (q2q4 - q1q3) - mx) + (-_2bx * q1 + _2bz * q3) * (_2bx * (q2q3 - q1q4) + _2bz * (q1q2 + q3q4) - my) + _2bx * q2 * (_2bx * (q1q3 + q2q4) + _2bz * (0.5f - q2q2 - q3q3) - mz);
     norm = sqrt(s1 * s1 + s2 * s2 + s3 * s3 + s4 * s4);    // normalise step magnitude
     norm = 1.0f/norm;
     s1 *= norm;
     s2 *= norm;
     s3 *= norm;
     s4 *= norm;

     // Compute rate of change of quaternion
     qDot1 = 0.5f * (-q2 * gx - q3 * gy - q4 * gz) - beta * s1;
     qDot2 = 0.5f * (q1 * gx + q3 * gz - q4 * gy) - beta * s2;
     qDot3 = 0.5f * (q1 * gy - q2 * gz + q4 * gx) - beta * s3;
     qDot4 = 0.5f * (q1 * gz + q2 * gy - q3 * gx) - beta * s4;

     // Integrate to yield quaternion
     q1 += qDot1 * deltat;
     q2 += qDot2 * deltat;
     q3 += qDot3 * deltat;
     q4 += qDot4 * deltat;
     norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);    // normalise quaternion
     norm = 1.0f/norm;
     q[0] = q1 * norm;
     q[1] = q2 * norm;
     q[2] = q3 * norm;
     q[3] = q4 * norm;

 }



 // Similar to Madgwick scheme but uses proportional and integral filtering on the error between estimated reference vectors and
 // measured ones.
 void mahony_quaternion_update(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz)
 {
     float q1 = q[0], q2 = q[1], q3 = q[2], q4 = q[3];   // short name local variable for readability
     float norm;
     float hx, hy, bx, bz;
     float vx, vy, vz, wx, wy, wz;
     float ex, ey, ez;
     float pa, pb, pc;

     // Auxiliary variables to avoid repeated arithmetic
     float q1q1 = q1 * q1;
     float q1q2 = q1 * q2;
     float q1q3 = q1 * q3;
     float q1q4 = q1 * q4;
     float q2q2 = q2 * q2;
     float q2q3 = q2 * q3;
     float q2q4 = q2 * q4;
     float q3q3 = q3 * q3;
     float q3q4 = q3 * q4;
     float q4q4 = q4 * q4;

     // Normalise accelerometer measurement
     norm = sqrt(ax * ax + ay * ay + az * az);
     if (norm == 0.0f) return; // handle NaN
     norm = 1.0f / norm;        // use reciprocal for division
     ax *= norm;
     ay *= norm;
     az *= norm;

     // Normalise magnetometer measurement
     norm = sqrt(mx * mx + my * my + mz * mz);
     if (norm == 0.0f) return; // handle NaN
     norm = 1.0f / norm;        // use reciprocal for division
     mx *= norm;
     my *= norm;
     mz *= norm;

     // Reference direction of Earth's magnetic field
     hx = 2.0f * mx * (0.5f - q3q3 - q4q4) + 2.0f * my * (q2q3 - q1q4) + 2.0f * mz * (q2q4 + q1q3);
     hy = 2.0f * mx * (q2q3 + q1q4) + 2.0f * my * (0.5f - q2q2 - q4q4) + 2.0f * mz * (q3q4 - q1q2);
     bx = sqrt((hx * hx) + (hy * hy));
     bz = 2.0f * mx * (q2q4 - q1q3) + 2.0f * my * (q3q4 + q1q2) + 2.0f * mz * (0.5f - q2q2 - q3q3);

     // Estimated direction of gravity and magnetic field
     vx = 2.0f * (q2q4 - q1q3);
     vy = 2.0f * (q1q2 + q3q4);
     vz = q1q1 - q2q2 - q3q3 + q4q4;
     wx = 2.0f * bx * (0.5f - q3q3 - q4q4) + 2.0f * bz * (q2q4 - q1q3);
     wy = 2.0f * bx * (q2q3 - q1q4) + 2.0f * bz * (q1q2 + q3q4);
     wz = 2.0f * bx * (q1q3 + q2q4) + 2.0f * bz * (0.5f - q2q2 - q3q3);

     // Error is cross product between estimated direction and measured direction of gravity
     ex = (ay * vz - az * vy) + (my * wz - mz * wy);
     ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
     ez = (ax * vy - ay * vx) + (mx * wy - my * wx);
     if (Ki > 0.0f)
         {
             eInt[0] += ex;      // accumulate integral error
             eInt[1] += ey;
             eInt[2] += ez;
         }
     else
         {
             eInt[0] = 0.0f;     // prevent integral wind up
             eInt[1] = 0.0f;
             eInt[2] = 0.0f;
         }

     // Apply feedback terms
     gx = gx + Kp * ex + Ki * eInt[0];
     gy = gy + Kp * ey + Ki * eInt[1];
     gz = gz + Kp * ez + Ki * eInt[2];

     // Integrate rate of change of quaternion
     pa = q2;
     pb = q3;
     pc = q4;
     q1 = q1 + (-q2 * gx - q3 * gy - q4 * gz) * (0.5f * deltat);
     q2 = pa + (q1 * gx + pb * gz - pc * gy) * (0.5f * deltat);
     q3 = pb + (q1 * gy - pa * gz + pc * gx) * (0.5f * deltat);
     q4 = pc + (q1 * gz + pa * gy - pb * gx) * (0.5f * deltat);

     // Normalise quaternion
     norm = sqrt(q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4);
     norm = 1.0f / norm;
     q[0] = q1 * norm;
     q[1] = q2 * norm;
     q[2] = q3 * norm;
     q[3] = q4 * norm;

 }

 // XXX FIXME : this need to be split up inn several functions
 // and calibration data need to be stored in flash
 void mpu9150_mainloop()
 {
     // Check that MPU9150 is reponding
     mpu9150_reset();
     uint8_t whoami = i2c_read_byte(MPU9150_ADDRESS, WHO_AM_I_MPU9150);
     printf("MPU9150 : I am 0x%x\n\r", whoami);

     if (whoami != 0x68) {
         // WHO_AM_I should be 0x68
         printf("ERROR : I SHOULD BE 0x68\n\r");
         return;
     }
     printf("MPU9150 is online...\n\r");

     // Init MPU
     mpu9150_init();

#ifdef MPU9150_SELFTEST
     // Run self tests
     printf("Running MPU9150 self tests...\r\n");
     mpu9150_selftest(SelfTest);
#endif

     mpu9150_calibrate();
     while(1) {
         int16_t data[7];
         mpu9150_read_data(data);
         for(int i=0; i<3; i++)
             printf("%04x ", (uint16_t) data[i]);
         for(int i=4; i<7; i++)
             printf("%04x ", (uint16_t) data[i]);
         ak8975a_read_raw_data(data);
         for(int i=0; i<3; i++)
             printf("%04x ", (uint16_t) data[i]);
         printf("\r\n");
     }



     // Calibrate accel and gyro MPU9150
     mpu9150_calibrate();

     // OK for MPU9150
     printf("MPU9150 initialized for active data mode....\n\r");

     // Init and calibrate magnetometer
     ak8975a_init();
     ak8975a_calibrate();
     printf("AK8975 initialized for active data mode....\n\r");

#if 0
     while(1) {
         ak8975a_read_data(&mx, &my, &mz);
         printf("x=%04.2f, y=%04.2f, z=%04.2f\r\n", mx, my, mz);
     }
#endif



     // Main fusion loop
     int mcount = 0;
     // set magnetometer read rate in Hz; 10 to 100 (max) Hz are reasonable values
     uint8_t MagRate = 100;
     // Used to calculate integration interval
     static int lastUpdate = 0, Now = 0;
     // Used to display not so often
     static int lastDisplay = 0;

     while(1) {
         // If intPin goes high or NEW_DATA register is set, then all data registers have new data
         // XXX FIXME : should do this in interrupt service
         if (i2c_read_byte(MPU9150_ADDRESS, INT_STATUS) & 0x01) {
             static int16_t data[7];

             // Read accel, temp and gyro data
             mpu9150_read_data(data);

             // Biases are removed by hardware if calibrate routine has been called
             ax = (float)data[0];
             ay = (float)data[1];
             az = (float)data[2];

             // Biases are removed by hardware if calibrate routine has been called.
             // Gyro values need to be scaled. Hhere we are at 250dps.
             gx = (float)data[4]*250.0/32768.0*PI/180.0;
             gy = (float)data[5]*250.0/32768.0*PI/180.0;
             gz = (float)data[6]*250.0/32768.0*PI/180.0;

             // Temmperature (not used)
             temperature = data[4];

             mcount++;
             // 200Hz sample rate for accel / gyro, 10Hz sammple rate for mag
             // So read mag data every 200/10 times
             // XXX FIXME : should do this more cleanly
             if (mcount >= 200/MagRate) {
                 // Get mag data
                 ak8975a_read_data(&mx, &my, &mz);
                 mcount = 0;
             }
         }

         // Get integration time by time elapsed since last filter update
         Now = get_time();
         deltat = (float)((Now - lastUpdate)/1000000.0f) ;
         lastUpdate = Now;

         //    if(lastUpdate - firstUpdate > 10000000.0f) {
         //     beta = 0.04;  // decrease filter gain after stabilized
         //     zeta = 0.015; // increasey bias drift gain after stabilized
         //   }

         // Fusion. Pass gyro rate as rad/s
         // WARNING : on mpu9150, magnetometer axis x and y are swapped !!!
         //int t1, t2;
         //t1 = get_time();


         printf("ax=%04.2f, ay=%04.2f, az=%04.2f, gx=%04.2f,  \
gy=%04.2f, gz=%04.2f, mx=%04.2f, my=%04.2f, mz=%04.2f\r\n",
                ax, ay, az, gx, gy, gz, mx, my, mz);
         continue;

#if 1
         //mahony_quaternion_update(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f, my, mx, mz);
#else
         //madgwick_quaternion_update(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
#endif
         //t2 = get_time();
         //printf("dt=%d\r\n", t2-t1);

         // Display 10 times/s
         if((Now-lastDisplay) > 100000) {
             lastDisplay = Now;

             // Define output variables from updated quaternion---these are Tait-Bryan angles, commonly used in aircraft orientation.
             // In this coordinate system, the positive z-axis is down toward Earth.
             // Yaw is the angle between Sensor x-axis and Earth magnetic North (or true North if corrected for local declination, looking down on the sensor positive yaw is counterclockwise.
             // Pitch is angle between sensor x-axis and Earth ground plane, toward the Earth is positive, up toward the sky is negative.
             // Roll is angle between sensor y-axis and Earth ground plane, y-axis up is positive roll.
             // These arise from the definition of the homogeneous rotation matrix constructed from quaternions.
             // Tait-Bryan angles as well as Euler angles are non-commutative; that is, the get the correct orientation the rotations must be
             // applied in the correct order which for this configuration is yaw, pitch, and then roll.
             // For more see http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles which has additional links.
             yaw   = atan2(2.0f * (q[1] * q[2] + q[0] * q[3]), q[0] * q[0] + q[1] * q[1] - q[2] * q[2] - q[3] * q[3]);
             pitch = -asin(2.0f * (q[1] * q[3] - q[0] * q[2]));
             roll  = atan2(2.0f * (q[0] * q[1] + q[2] * q[3]), q[0] * q[0] - q[1] * q[1] - q[2] * q[2] + q[3] * q[3]);
             pitch *= 180.0f / PI;
             yaw   *= 180.0f / PI;
             yaw   -= 13.8f; // Declination at Danville, California is 13 degrees 48 minutes and 47 seconds on 2014-04-04
             roll  *= 180.0f / PI;

             printf("Yaw, Pitch, Roll: %3.2f %3.2f %3.2f\n\r", yaw, pitch, roll);
             //printf("average rate = %f\n\r", (float) sumCount/sum);
         }
     }
 }
