/*! @file : sdk_hal_bme280.c
 * @author  Mayra Torres
 * @version 1.0.0
 * @date    28 ene. 2021
 * @brief   Driver para 
 * @details
 *
*/
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "sdk_hal_bme280.h"
#include "sdk_hal_i2c0.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Private Prototypes
 ******************************************************************************/


/*******************************************************************************
 * External vars
 ******************************************************************************/


/*******************************************************************************
 * Local vars
 ******************************************************************************/
status_t status;

/*******************************************************************************
 * Private Source Code
 ******************************************************************************/

/*******************************************************************************
 * Public Source Code
 ******************************************************************************/
 /*--------------------------------------------*/
void waitTime(int32_t t) {
	uint32_t tiempo = ((0xFFFFF*t)/200);
	do {
		tiempo--;
	} while (tiempo != 0x0000);
}

bool bme280_Init(void){
	uint8_t id;
	status= i2c0MasterReadByte(&id, BME280_I2C_DEVICE_ADDRESS,BME280_WHO_AM_I_MEMORY_ADDRESS);
	if (id != 0x60){
		return(kStatus_Fail);
	}
	write8(BME280_REGISTER_SOFTRESET, 0xB6);
	waitTime(10);

	while(ReadingCalibration()){

		waitTime(10);

	}
	readCoefficients();
	setSampling(MODE_NORMAL,SAMPLING_X16,SAMPLING_X16,SAMPLING_X16, FILTER_OFF,STANDBY_MS_0_5);

	waitTime(100);

	return (true);
}


void readCoefficients(void) {
  _bme280_calib.dig_T1 = read16_LE(BME280_REGISTER_DIG_T1);
  _bme280_calib.dig_T2 = readS16_LE(BME280_REGISTER_DIG_T2);
  _bme280_calib.dig_T3 = readS16_LE(BME280_REGISTER_DIG_T3);

  _bme280_calib.dig_P1 = read16_LE(BME280_REGISTER_DIG_P1);
  _bme280_calib.dig_P2 = readS16_LE(BME280_REGISTER_DIG_P2);
  _bme280_calib.dig_P3 = readS16_LE(BME280_REGISTER_DIG_P3);
  _bme280_calib.dig_P4 = readS16_LE(BME280_REGISTER_DIG_P4);
  _bme280_calib.dig_P5 = readS16_LE(BME280_REGISTER_DIG_P5);
  _bme280_calib.dig_P6 = readS16_LE(BME280_REGISTER_DIG_P6);
  _bme280_calib.dig_P7 = readS16_LE(BME280_REGISTER_DIG_P7);
  _bme280_calib.dig_P8 = readS16_LE(BME280_REGISTER_DIG_P8);
  _bme280_calib.dig_P9 = readS16_LE(BME280_REGISTER_DIG_P9);

  _bme280_calib.dig_H1 = read8(BME280_REGISTER_DIG_H1);
  _bme280_calib.dig_H2 = readS16_LE(BME280_REGISTER_DIG_H2);
  _bme280_calib.dig_H3 = read8(BME280_REGISTER_DIG_H3);
  _bme280_calib.dig_H4 = ((int8_t)read8(BME280_REGISTER_DIG_H4) << 4) |
                         (read8(BME280_REGISTER_DIG_H4 + 1) & 0xF);
  _bme280_calib.dig_H5 = ((int8_t)read8(BME280_REGISTER_DIG_H5 + 1) << 4) |
                         (read8(BME280_REGISTER_DIG_H5) >> 4);
  _bme280_calib.dig_H6 = (int8_t)read8(BME280_REGISTER_DIG_H6);
}

uint8_t read8(int8_t reg){
	uint8_t value;
	i2c0MasterReadByte(&value, BME280_I2C_DEVICE_ADDRESS,reg);

	return value;
}

uint16_t read16(int8_t reg) {

	uint16_t value;
	uint16_t reg1=read8(reg);
	uint16_t reg2=read8(reg+1);

	value=(reg2<<8)|reg1;

	return value;

}

uint16_t read16_LE(int8_t reg){

	uint16_t  temp = read16(reg);

	return ((uint16_t)(temp>>8 | temp<<8));

}
int16_t readS16 (int8_t reg){

	return((int16_t)read16(reg));

}

int16_t readS16_LE (int8_t reg){

	return((int16_t)read16_LE(reg));

}

void write8(int8_t reg, int8_t value){

	i2c0MasterWriteByte(value,BME280_I2C_DEVICE_ADDRESS,reg);

}

bool ReadingCalibration(void){

	uint8_t rStatus;

	rStatus=read8(BME280_REGISTER_STATUS);

	return((rStatus & (1<<0))!=0);
}

void setSampling(uint8_t mode,
		uint8_t tempSampling,
		uint8_t pressSampling,
		uint8_t humSampling,
		uint8_t filter,
		uint8_t duration){

	 write8(BME280_REGISTER_CONTROL, MODE_SLEEP);

	 write8(BME280_REGISTER_CONTROLHUMID, humSampling);

	 write8(BME280_REGISTER_CONFIG, ((duration << 5) | (filter << 2) | 1));
	 write8(BME280_REGISTER_CONTROL, ( (tempSampling << 5) | (pressSampling << 2) | mode));



}

uint32_t read24(int8_t reg){

	uint32_t value;

	value = read8(reg);
	value<<=8;
	value |= read8(reg+1);
	value<<=8;
	value = read8(reg+2);

	return value;

}




/*!
 *   @brief  Returns the temperature from the sensor
 *   @returns the temperature read from the device
 */
float readTemperature(void) {
  int32_t var1, var2;

  int32_t adc_T = read24(BME280_REGISTER_TEMPDATA);
  if (adc_T == 0x800000) // value in case temp measurement was disabled
    return 0;
  adc_T >>= 4;

  var1 = ((((adc_T >> 3) - ((int32_t)_bme280_calib.dig_T1 << 1))) *
          ((int32_t)_bme280_calib.dig_T2)) >>11;

  var2 = (((((adc_T >> 4) - ((int32_t)_bme280_calib.dig_T1)) *
          ((adc_T >> 4) - ((int32_t)_bme280_calib.dig_T1))) >> 12) *
		  ((int32_t)_bme280_calib.dig_T3)) >> 14;

  t_fine = var1 + var2 + t_fine_adjust;

  float T = (t_fine * 5 + 128) >> 8;
  return T / 100;
}

/*!
 *   @brief  Returns the pressure from the sensor
 *   @returns the pressure value (in Pascal) read from the device
 */
float readPressure(void) {
  int64_t var1, var2, p;

  readTemperature(); // must be done first to get t_fine

  int32_t adc_P = read24(BME280_REGISTER_PRESSUREDATA);
  if (adc_P == 0x800000) // value in case pressure measurement was disabled
    return 0;
  adc_P >>= 4;

  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)_bme280_calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)_bme280_calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)_bme280_calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)_bme280_calib.dig_P3) >> 8) +
         ((var1 * (int64_t)_bme280_calib.dig_P2) << 12);
  var1 =(((((int64_t)1) << 47) + var1)) * ((int64_t)_bme280_calib.dig_P1) >> 33;

  if (var1 == 0) {
    return 0; // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)_bme280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)_bme280_calib.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)_bme280_calib.dig_P7) << 4);
  return (float)p / 256;
}

/*!
 *  @brief  Returns the humidity from the sensor
 *  @returns the humidity value read from the device
 */
float readHumidity(void) {
  readTemperature(); // must be done first to get t_fine

  int32_t adc_H = read16(BME280_REGISTER_HUMIDDATA);
  if (adc_H == 0x8000) // value in case humidity measurement was disabled
    return 0;

  int32_t v_x1_u32r;

  v_x1_u32r = (t_fine - ((int32_t)76800));

  v_x1_u32r = (((((adc_H << 14) - (((int32_t)_bme280_calib.dig_H4) << 20) -
              (((int32_t)_bme280_calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >>15) *
              (((((((v_x1_u32r * ((int32_t)_bme280_calib.dig_H6)) >> 10) *
              (((v_x1_u32r * ((int32_t)_bme280_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
              ((int32_t)2097152)) * ((int32_t)_bme280_calib.dig_H2) + 8192) >> 14));

  v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                             ((int32_t)_bme280_calib.dig_H1)) >> 4));

  v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
  v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
  float h = (v_x1_u32r >> 12);
  return h / 1024.0;
}

/*!
 *   Calculates the altitude (in meters) from the specified atmospheric
 *   pressure (in hPa), and sea-level pressure (in hPa).
 *   @param  seaLevel      Sea-level pressure in hPa
 *   @returns the altitude value read from the device
 */
float readAltitude(float seaLevel) {
  // Equation taken from BMP180 datasheet (page 16):
  //  http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

  // Note that using the equation from wikipedia can give bad results
  // at high altitude. See this thread for more information:
  //  http://forums.adafruit.com/viewtopic.php?f=22&t=58064

  float atmospheric = readPressure() / 100.0F;
  return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.1903));
}

/*!
 *   Returns the current temperature compensation value in degrees Celcius
 *   @returns the current temperature compensation value in degrees Celcius
 */
float getTemperatureCompensation(void) {
  return (float)(((t_fine_adjust * 5) >> 8) / 100);
};

/*!
 *  Sets a value to be added to each temperature reading. This adjusted
 *  temperature is used in pressure and humidity readings.
 *  @param  adjustment  Value to be added to each tempature reading in Celcius
 */
void setTemperatureCompensation(float adjustment) {
  // convert the value in C into and adjustment to t_fine
  t_fine_adjust = (((int32_t)(adjustment * 100) << 8)) / 5;
};






















