/*
 * sdk_mensaje.c
 *
 *  Created on: 5/03/2021
 *      Author: JAVIER CASALLAS
 */
#include "sdk_mensaje.h"
#include "sdk_pph_bme280.h"
#include "sdk_hal_i2c0.h"


//#include <stdio.h>
//#include "board.h"
//#include "peripherals.h"
//#include "pin_mux.h"
//#include "clock_config.h"


//bool leer=false;
void sdk_mens(){

//			printf("Temperatura,");
			float temp =(float)readTemperature();
//			printf(",Humedad,");
			double hum = readHumidity();
//			printf(",Presion,");
			double press =readPressure();
//			printf("%c",0x1A);
			//double alt = readAltitude(SEALEVELPRESSURE_HPA);

			printf("Temperatura, %f,Humedad, %f,Presion, %f%c",temp,hum,press,0x1A);
			//printf("Temperaturaaaaaaaa= %.4f  \n\r",temp);
			waitTime(1000);

}
