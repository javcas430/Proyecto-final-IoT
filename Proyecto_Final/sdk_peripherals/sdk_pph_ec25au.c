/*! @file : sdk_pph_ec25au.c
 * @author  Ezequiel De La Hoz - Mayra Torres - Javier Casallas
 * @version 1.0.0
 * @date    23/01/2021
 * @brief   Driver para modem EC25AU
 * @details
 *
*/
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "stdio.h"
#include "sdk_pph_ec25au.h"
#include "sdk_mdlw_leds.h"
#include "sdk_mensaje.h"
#include "sdk_hal_lptmr0.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/


typedef struct _estado_fsm{
	uint8_t anterior;
	uint8_t actual;
}estado_fsm_t;

enum _ec25_lista_ordendes{
	kORDEN_NO_HAY_EN_EJECUCION=0,
	kORDEN_ENVIAR_MENSAJE_DE_TEXTO,
};

#define EC25_BYTES_EN_BUFFER	100
#define EC25_TIEMPO_MAXIMO_ESPERA	1

#define HABILITAR_TLPTMR0   1
/*******************************************************************************
 * Private Prototypes
 ******************************************************************************/
void ec25BorrarBufferRX(void);

/*******************************************************************************
 * External vars
 ******************************************************************************/


/*******************************************************************************
 * Local vars
 ******************************************************************************/
uint8_t ec25_buffer_tx[EC25_BYTES_EN_BUFFER];		//almacena los datos a enviar al modem
uint8_t ec25_buffer_rx[EC25_BYTES_EN_BUFFER];		//almacena las datos que provienen del modem
uint8_t ec25_index_buffer_rx = 0;				//apuntador de buffer de datos

estado_fsm_t ec25_fsm;	//almacena estado actual de la maquina de estados modem EC25

uint32_t ec25_timeout;	//almacena las veces que han llamado la fsm del EC25 y estimar tiempo

uint8_t ec25_comando_en_ejecucion;	//almacena ultimo comando enviado para ser ejecutado

//Listado de comando AT disponibles para ser enviados al modem Quectel
const char *ec25_comandos_at[] = {
	"AT+CPIN?",								//Conexion a internet a traves del modem
	"AT+QCFG=\"nwscanmode\",0,1",			//Activar bandas nacionales
	"AT+QCFG=\"band\",0, 800005A,0",
	"AT+QCSQ",								//Verificamos red conectada
	"AT+CREG?",								//registros
	"AT+CGREG?",
	"AT+CEREG?",
	"AT+CGDCONT=1,\"IP\",\"internet.comcel.com.co\"", //Configuracion APN claro
	"AT+QIACT=1",         					//Activacion de contexto
	"AT+QMTOPEN=0,\" 54.236.210.97\",1883",	//Conectarse al servidor MQTT
	"AT+QMTCONN=0,\"guest\",\"guest\" ,\"guest\"", //Conectarse al perfil dentro del broker
	"AT+QMTPUB=0,0,0,0,\"1/sensor\"",		//Publicar en un topico
	"Mensaje", 								//MENSAJE & CTRL+Z
	"AT+CFUN=0", 							//Apagar modem
	"AT+CFUN=1",							//iniciar modem
	};

//Lista de respuestas a cada comando AT
const char  *ec25_repuestas_at[]={
		"READY",	//CPIN?
		"OK",		//AT+QCFG=\"nwscanmode\",0,1
		"OK",		//AT+QCFG=\"band\",0, 800005A,0
		"LTE",		//AT+QCSQ
		"0,1",		//AT+CREG?
		"0,1",		//AT+CGREG?
		"0,1",		//AT+CEREG?
		"OK",		//AT+CGDCONT=1,\"IP\",\"internet.comcel.com.co\"
		"OK",		//AT+QIACT=1
		"OK",		//AT+QMTOPEN=0,\" 3.210.189.57\",1883
		"OK",		//AT+QMTCONN=0,\"guest\",\"guest\" ,\"guest\"
		">",		//AT+QMTPUB=0,0,0,0,\"1/sensor\"
		"OK",		//Mensaje & ctrl Z
		"OK",		//AT+CFUN=0
		"OK",		//AT+CFUN=1
};


/*******************************************************************************
 * Private Source Code
 ******************************************************************************/
//------------------------------------

void waytTime2(void) {
	uint32_t tiempo = 0xFFFFFF;
	do {
		tiempo--;
	} while (tiempo != 0x0000);
}


void ec25BorrarBufferRX(void){
	uint8_t i;

	//LLenar de ceros buffer que va a recibir datos provenientes del modem
	for(i=0;i<EC25_BYTES_EN_BUFFER;i++){
		ec25_buffer_rx[i]=0;
	}

	//borra apuntador de posicion donde se va a almacenar el proximo dato
	//Reset al apuntador
	ec25_index_buffer_rx=0;
}
//------------------------------------
void ec25BorrarBufferTX(void){
	uint8_t i;

	//LLenar de ceros buffer que va a recibir datos provenientes del modem
	for(i=0;i<EC25_BYTES_EN_BUFFER;i++){
		ec25_buffer_tx[i]=0;
	}
}
//------------------------------------
void waytTimeModem(void) {
	uint32_t tiempo = 0xFFFF;
	do {
		tiempo--;
	} while (tiempo != 0x0000);
}
//------------------------------------
/*******************************************************************************
 * Public Source Code
 ******************************************************************************/
status_t ec25Inicializacion(void){

	ec25_fsm.anterior=kFSM_INICIO;
	ec25_fsm.actual=kFSM_INICIO;

	ec25_timeout=0;	//borra contador de tiempo

	ec25BorrarBufferTX();	//borrar buffer de transmisión
	ec25BorrarBufferRX();	//borra buffer de recepción

	ec25_comando_en_ejecucion=kORDEN_NO_HAY_EN_EJECUCION;	//Borra orden en ejecucipon actual

	ec25_fsm.actual=kFSM_ENVIANDO_CPIN;//NNNNNNNNNNNNNNNNNNNNn

	return(kStatus_Success);
}
//------------------------------------
status_t ec25EnviarMensajeDeTexto(uint8_t *mensaje, uint8_t size_mensaje ){

	memcpy(&ec25_buffer_tx[0],mensaje, size_mensaje);	//copia mensaje a enviar en buffer TX del EC25

	ec25_comando_en_ejecucion=kORDEN_ENVIAR_MENSAJE_DE_TEXTO;
	ec25_fsm.actual=kFSM_ENVIANDO_CPIN;
	return(kStatus_Success);
}
//------------------------------------
uint8_t ec25Polling(void){
	status_t status;
	uint8_t nuevo_byte_uart;
	uint8_t *puntero_ok=0;	//variable temporal que será usada para buscar respuesta
	switch (ec25_fsm.actual) {
	case kFSM_INICIO:
		break;

	case kFSM_ENVIANDO_CPIN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_CPIN]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QCFG_nwscanmode:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QCFG_nwscanmode]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QCFG_band:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QCFG_band]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QCSQ:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QCSQ]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_CREG]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CGREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_CGREG]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CEREG:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_CEREG]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CGDCONT:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_CGDCONT]);	//Envia comando AT

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QIACT:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QIACT]);	//Envia comando AT+CPIN?

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTOPEN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QMTOPEN]);	//Envia comando AT+CREG?

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTCONN:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QMTCONN]);	//Envia comando AT+CMGF=1

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_QMTPUB:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[kAT_QMTPUB]);	//Envia comando AT+CMGS="3003564960"

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_MENSAJE_TXT:
		//printf("%s\r\n%c", ec25_buffer_tx,0x1A);	//Envia mensaje de texto incluido  CTRL+Z (0x1A)

		//printf("Temperatura,16.809999,Humedad,70.400391,Presion,737.739990\r\n%c",0x1A);
		sdk_mens();
		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CFUN_0:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[KAT_CFUN_0]);	//Envia comando AT+CMGS="3003564960"

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_ENVIANDO_CFUN_1:
		ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel
		printf("%s\r\n", ec25_comandos_at[KAT_CFUN_1]);	//Envia comando AT+CMGS="3003564960"

		ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
		ec25_fsm.actual = kFSM_ESPERANDO_RESPUESTA;	//avanza a esperar respuesta del modem
		waytTime2();
		ec25_timeout = 0;	//reset a contador de tiempo
		break;

	case kFSM_RESULTADO_ERROR:
		break;

	case kFSM_RESULTADO_EXITOSO:
		break;

	case kFSM_ESPERANDO_RESPUESTA:
		ec25_timeout++;	//incrementa contador de tiempo

		//Busca si llegaron nuevos datos desde modem mientras esperaba
		while (uart0CuantosDatosHayEnBuffer() > 0) {
			status = uart0LeerByteDesdeBuffer(&nuevo_byte_uart);
			if (status == kStatus_Success) {
				//reinicia contador de tiempo
				ec25_timeout=0;
				//almacena dato en buffer rx de quectel
				ec25_buffer_rx[ec25_index_buffer_rx] = nuevo_byte_uart;
				//incrementa apuntador de datos en buffer de quectel
				ec25_index_buffer_rx++;
			}
		}

		//pregunta si el tiempo de espera supera el configurado
		if(ec25_timeout>EC25_TIEMPO_MAXIMO_ESPERA){
			//busca la respuesta indicada dependiendeo de cual comando AT se le había enviado al modem
			switch(ec25_fsm.anterior){
			case kFSM_ENVIANDO_CPIN:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_CPIN])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QCFG_nwscanmode;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}

				break;
			case kFSM_ENVIANDO_QCFG_nwscanmode:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QCFG_nwscanmode])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QCFG_band;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_QCFG_band:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QCFG_band])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QCSQ;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_QCSQ:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QCSQ])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CREG;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_CREG:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_CREG])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CGREG;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;

			case kFSM_ENVIANDO_CGREG:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_CGREG])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CEREG;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_CEREG:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_CEREG])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CGDCONT;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_CGDCONT:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_CGDCONT])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QIACT;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_QIACT:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QIACT])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QMTOPEN;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_QMTOPEN:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QMTOPEN])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QMTCONN;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;
			case kFSM_ENVIANDO_QMTCONN:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QMTCONN])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_QMTPUB;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;


			case kFSM_ENVIANDO_QMTPUB:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_QMTPUB])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_MENSAJE_TXT;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;

			case kFSM_ENVIANDO_MENSAJE_TXT:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[kAT_TEXT_MSG_END])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CFUN_0;		//avanza a enviar nuevo comando al modem
					waytTime();
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;

			case kFSM_ENVIANDO_CFUN_0:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[KAT_CFUN_0])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_ENVIANDO_CFUN_1;		//avanza a enviar nuevo comando al modem

				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;

			case kFSM_ENVIANDO_CFUN_1:
				//Busca palabra EC25 en buffer rx de quectel
				puntero_ok = (uint8_t*) (strstr((char*) (&ec25_buffer_rx[0]),
						(char*) (ec25_repuestas_at[KAT_CFUN_1])));

				if(puntero_ok!=0x00){
					//Si la respuesta es encontrada, se avanza al siguiente estado
					ec25_fsm.anterior = ec25_fsm.actual;		//almacena el estado actual
					ec25_fsm.actual = kFSM_RESULTADO_EXITOSO;		//avanza a enviar nuevo comando al modem
				}else{
					//Si la respuesta es incorrecta, se queda en estado de error
					//No se cambia (ec25_fsm.anterior) para mantener en que comando AT fue que se generó error
					ec25_fsm.actual = kFSM_RESULTADO_ERROR;		//se queda en estado de error
				}
				break;

			default:
				//para evitar bloqueos, se reinicia la fsm en caso de entrar a un estado ilegal
				ec25Inicializacion();
				break;
			}
		}
		break;
	default:
		//para evitar bloqueos, se reinicia la fsm en caso de entrar a un estado ilegal
		ec25Inicializacion();
		break;
	}

	return(ec25_fsm.actual);
}


status_t detectarModemQuectel(void){
	uint8_t nuevo_byte_uart;
	status_t status;
	uint8_t *puntero_ok=0;
	uint32_t contador_tiempo_ms=0;

	ec25BorrarBufferRX();	//limpia buffer para recibir datos de quectel

	//envia comando ATI a modem quectel
	//printf("%s\r\n",ec25_comandos_at[kATI]);
	//printf("ATI\r\n");

	//Recibir la respuesta es automatica por IRq de UART0

	//peeeero Sacar los datos del buffer circular y llevarlos al buffer del modem
	//es tarea del proceso interno

	do {
		waytTimeModem();
		contador_tiempo_ms++;

		if (uart0CuantosDatosHayEnBuffer() > 0) {
			status = uart0LeerByteDesdeBuffer(&nuevo_byte_uart);
			if (status == kStatus_Success) {
				//reinicia contador de tiempo
				contador_tiempo_ms=0;
				//almacena dato en buffer rx de quectel
				ec25_buffer_rx[ec25_index_buffer_rx] = nuevo_byte_uart;
				//incrementa apuntador de datos en buffer de quectel
				ec25_index_buffer_rx++;
			}
		}
	} while (contador_tiempo_ms < 5);

	//Busca palabra EC25 en buffer rx de quectel
	//puntero_ok=(uint8_t *)(strstr((char*)(&ec25_buffer_rx[0]),(char *)(ec25_repuestas_at[kATI])));
	puntero_ok=(uint8_t *)(strstr((char*)(&ec25_buffer_rx[0]),(char *)("EC25")));
	if(puntero_ok!=0){
		return(kStatus_Success);
	}else{
		return(kStatus_Fail);
	}
}



status_t fsmModemQuectelEC25(void){

}
