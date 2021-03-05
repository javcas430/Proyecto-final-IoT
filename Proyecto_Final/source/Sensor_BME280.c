/*
 *			v0.1 dato recibido por puerto COM es contestado en forma de ECO
 *			v0.2 dato recibido por puerto COM realiza operaciones especiales
 *					A/a=invierte estado de LED conectado en PTB10
 *					v=apaga LED conectado en PTB7
 *					V=enciende LED conectado en PTB7
 *					r=apaga LED conectado en PTB6
 *			v0.3 nuevo comando por puerto serial para prueba de MMA8451Q
 *					M=detecta acelerometro MM8451Q en bus I2C0
 *
 *
 */
/*******************************************************************************

 * Includes
 ******************************************************************************/
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL02Z4.h"
#include "fsl_debug_console.h"

#include "sdk_hal_uart0.h"
#include "sdk_hal_gpio.h"
#include "sdk_hal_i2c0.h"

#include <sdk_pph_bme280.h>
#include "sdk_mdlw_leds.h"
#include "sdk_pph_mma8451Q.h"
#include "sdk_pph_ec25au.h"
#include "sdk_mensaje.h"

/***************************
 *
 * ****************************************************
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
//#define SEALEVELPRESSURE_HPA 1013.25
/*******************************************************************************
 * Private Source Code
 ******************************************************************************/
void waytTime(void) {
	uint32_t tiempo = 0xFFFFF;
	do {
		tiempo--;
	} while (tiempo != 0x0000);
}

void PrintValues(void) {

//	printf("Temperatura = %.2f C\n\r",temp );

//	printf("Presion = %.2f hPa\n\r",press);

//	printf("Altitud aproximada = %.2f m\n\r",altt);

//	printf("Humedad = %.2f %\n\r",hum );

	printf("\n\r");
}

/*
 * @brief   Application entry point.
 */
int main(void) {
	uint8_t mensaje[]="Temperatura, ,Humedad, ,Presion, ";//12-22-32
	uint8_t estado_actual_ec25;
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	BOARD_InitDebugConsole();
#endif

	(void) uart0Inicializar(115200);	//115200bps
	(void) i2c0MasterInit(100000);	//100kbps

	bool inicio = Bme280_Begin(0x76);
//	if (inicio){
//		printf("todo bien\n\r");
//		mensaje();
//	}

	//uint8_t letra;
	//status_t status;
	//bool leer=false;
   //LLamado a funcion que identifica modem conectado a puerto UART0

	if(detectarModemQuectel()==kStatus_Success){
		encenderLedAzul();
	}else{
		apagarLedAzul();
	}

/*while (1) {

		if (leer) {
			float temp =(float)readTemperature();
			double press =readPressure();
			double alt = readAltitude(SEALEVELPRESSURE_HPA);
			double hum = readHumidity();
			printf("Temperaturaaaaaaaa= %.4f  \n\r",temp);
			waitTime(1000);
		}
		if (uart0CuantosDatosHayEnBuffer() > 0) {
			status = uart0LeerByteDesdeBuffer(&letra);
			printf("Dato entrante: %c \r\n", letra);

			switch (letra) {

			case 'i':
				if (inicio) { //inicializa el sensor en la direccion i2c 0x76
					printf("Iniciando Medicion!!  \n\r");
					leer=true;
				} else {
					printf("Fallo al iniciar BME280 ='( ");
					waitTime(200);
					inicio = Bme280_Begin(0x76);
				}
				break;
			case 'I':
				leer=false;
				break;
			default:
				printf("Letra incorrecta :( \r\n");
				break;

			}

		}
	}
	*/
    //inicializa todas las funciones necesarias para trabajar con el modem EC25
    ec25Inicializacion();
   ec25EnviarMensajeDeTexto(&mensaje[0], sizeof(mensaje));

	//Ciclo infinito encendiendo y apagando led verde
	//inicia el SUPERLOOP
    while(1) {
    	waytTime();

    	estado_actual_ec25=ec25Polling();

  	switch(estado_actual_ec25){
    	case kFSM_RESULTADO_ERROR:
    		toggleLedRojo();
    		apagarLedVerde();
    		apagarLedAzul();
    		break;

    	case kFSM_RESULTADO_EXITOSO:
    		toggleLedVerde();
    		apagarLedAzul();
    		apagarLedRojo();
    		break;

    	default:
    		toggleLedAzul();
    		apagarLedVerde();
    		apagarLedRojo();
    		break;
    	}
    }


	return 0;
}
