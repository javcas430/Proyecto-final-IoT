/*
 * sdk_mensaje.c
 *
 *  Created on: 5/03/2021
 *      Author: JAVIER CASALLAS
 */
#include "sdk_mensaje.h"
#include "sdk_pph_bme280.h"
#include "sdk_hal_i2c0.h"



void sdk_mens(){
			char vec[100];
			int temp =(float)readTemperature();
			int hum = readHumidity();
			int press =readPressure();
			//double alt = readAltitude(SEALEVELPRESSURE_HPA);


			sprintf((char *)(&vec[0]),"temperatura,%i,humedad,%i,presion,%i",temp,hum,press);
			printf("%s%c",vec,0x1A);
			waitTime(1000);

}
