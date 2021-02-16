/*! @file : sdk_hal_bme280.h
 * @author  Mayra Torres
 * @version 1.0.0
 * @date    28 ene. 2021
 * @brief   Driver para 
 * @details
 *
 */
#ifndef SDK_HAL_BME280_H_
#define SDK_HAL_BME280_H_
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include"fsl_common.h"


/*!
 * @addtogroup HAL
 * @{
 */
/*!
 * @addtogroup I2C
 * @{
 */
/*******************************************************************************
 * Public Definitions
 ******************************************************************************/

/*******************************************************************************
 * External vars
 ******************************************************************************/

/*******************************************************************************
 * Public vars
 ******************************************************************************/
int32_t t_fine;
int32_t t_fine_adjust = 0;
/*******************************************************************************
 * Public Prototypes
 ******************************************************************************/
/*--------------------------------------------*/
/*!

/*!
 *  @brief Register addresses
 */
#define BME280_I2C_DEVICE_ADDRESS	0x76
#define BME280_WHO_AM_I_MEMORY_ADDRESS  208

enum BME_Register {
  BME280_REGISTER_DIG_T1 = 0x88,
  BME280_REGISTER_DIG_T2 = 0x8A,
  BME280_REGISTER_DIG_T3 = 0x8C,

  BME280_REGISTER_DIG_P1 = 0x8E,
  BME280_REGISTER_DIG_P2 = 0x90,
  BME280_REGISTER_DIG_P3 = 0x92,
  BME280_REGISTER_DIG_P4 = 0x94,
  BME280_REGISTER_DIG_P5 = 0x96,
  BME280_REGISTER_DIG_P6 = 0x98,
  BME280_REGISTER_DIG_P7 = 0x9A,
  BME280_REGISTER_DIG_P8 = 0x9C,
  BME280_REGISTER_DIG_P9 = 0x9E,

  BME280_REGISTER_DIG_H1 = 0xA1,
  BME280_REGISTER_DIG_H2 = 0xE1,
  BME280_REGISTER_DIG_H3 = 0xE3,
  BME280_REGISTER_DIG_H4 = 0xE4,
  BME280_REGISTER_DIG_H5 = 0xE5,
  BME280_REGISTER_DIG_H6 = 0xE7,

  BME280_REGISTER_CHIPID = 0xD0,
  BME280_REGISTER_VERSION = 0xD1,
  BME280_REGISTER_SOFTRESET = 0xE0,

  BME280_REGISTER_CAL26 = 0xE1, // R calibration stored in 0xE1-0xF0

  BME280_REGISTER_CONTROLHUMID = 0xF2,
  BME280_REGISTER_STATUS = 0XF3,
  BME280_REGISTER_CONTROL = 0xF4,
  BME280_REGISTER_CONFIG = 0xF5,
  BME280_REGISTER_PRESSUREDATA = 0xF7,
  BME280_REGISTER_TEMPDATA = 0xFA,
  BME280_REGISTER_HUMIDDATA = 0xFD
};


typedef struct _bme280_calib_data
{
  uint16_t dig_T1; //temperature compensation value
  int16_t dig_T2;  //temperature compensation value
  int16_t dig_T3;  //temperature compensation value

  uint16_t dig_P1; //pressure compensation value
  int16_t dig_P2;  //pressure compensation value
  int16_t dig_P3;  // pressure compensation value
  int16_t dig_P4;  // pressure compensation value
  int16_t dig_P5;  //pressure compensation value
  int16_t dig_P6;  //pressure compensation value
  int16_t dig_P7;  // pressure compensation value
  int16_t dig_P8;  // pressure compensation value
  int16_t dig_P9;  // pressure compensation value

  uint8_t dig_H1; ///< humidity compensation value
  int16_t dig_H2; ///< humidity compensation value
  uint8_t dig_H3; ///< humidity compensation value
  int16_t dig_H4; ///< humidity compensation value
  int16_t dig_H5; ///< humidity compensation value
  int8_t dig_H6;  ///< humidity compensation value
} bme280_calib_data_t;

bme280_calib_data_t _bme280_calib;
enum sensor_sampling {
   SAMPLING_NONE,
   SAMPLING_X1,
   SAMPLING_X,
   SAMPLING_X4,
   SAMPLING_X8,
   SAMPLING_X16,
 };

 /**************************************************************************/
 /*!
     @brief  power modes
 */
 /**************************************************************************/
 /**************************************************************************/
 /*!
     @brief  filter values
 */
 /**************************************************************************/
enum sensor_filter {
   FILTER_OFF ,
   FILTER_X2,
   FILTER_X4,
   FILTER_X8,
   FILTER_X16,
 };

 /**************************************************************************/
 /*!
     @brief  standby duration in ms
 */
 /**************************************************************************/
 enum standby_duration {
   STANDBY_MS_0_5,
   STANDBY_MS_10,
   STANDBY_MS_20,
   STANDBY_MS_62_,
   STANDBY_MS_125,
   STANDBY_MS_250,
   STANDBY_MS_500,
   STANDBY_MS_1000,
 };

 enum sensor_mode {
  MODE_SLEEP,
  MODE_FORCED,
  MODE_NORMAL,
};

void waitTime(int32_t t);
bool bme280_Init(void);

uint8_t read8(int8_t reg);
uint16_t read16(int8_t reg);
uint16_t read16_LE(int8_t reg);
int16_t readS16 (int8_t reg);
int16_t readS16_LE (int8_t reg);

void write8(int8_t reg, int8_t value);

void readCoefficients(void);
bool ReadingCalibration(void);
void setSampling(uint8_t mode,
		uint8_t tempSampling,
		uint8_t pressSampling,
		uint8_t humSampling,
		uint8_t filter,
		uint8_t duration);

float readTemperature(void);
float readPressure(void);
float readHumidity(void);
float readAltitude(float seaLevel);
float getTemperatureCompensation(void);
void setTemperatureCompensation(float adjustment);


#endif /* SDK_HAL_BME280_H_*/
