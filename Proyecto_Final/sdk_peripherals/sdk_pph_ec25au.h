/*! @file : sdk_pph_ec25au.h
 * @author  Ernesto Andres Rincon Cruz
 * @version 1.0.0
 * @date    23/01/2021
 * @brief   Driver para modem EC25AU
 * @details
 *
 */
#ifndef SDK_PPH_EC25AU_H_
#define SDK_PPH_EC25AU_H_
/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "sdk_hal_uart0.h"

/*!
 * @addtogroup PPH
 * @{
 */
/*!
 * @addtogroup EC25AU
 * @{
 */
/*******************************************************************************
 * Public Definitions
 ******************************************************************************/
enum _ec25_lista_comandos_at {
	kAT_CPIN = 0,
	kAT_QCFG_nwscanmode,
	kAT_QCFG_band,
	kAT_QCSQ,
	kAT_CREG,
	kAT_CGREG,
	kAT_CEREG,
	kAT_CGDCONT,
	kAT_QIACT,
	kAT_QMTOPEN,
	kAT_QMTCONN,
	kAT_QMTPUB,
	kAT_TEXT_MSG_END,
};

enum _fsm_ec25_state{
	kFSM_INICIO=0,
	kFSM_ENVIANDO_CPIN,
	kFSM_ENVIANDO_QCFG_nwscanmode,
	kFSM_ENVIANDO_QCFG_band,
	kFSM_ENVIANDO_QCSQ,
	kFSM_ENVIANDO_CREG,
	kFSM_ENVIANDO_CGREG,
	kFSM_ENVIANDO_CEREG,
	kFSM_ENVIANDO_CGDCONT,
	kFSM_ENVIANDO_QIACT,
	kFSM_ENVIANDO_QMTOPEN,
	kFSM_ENVIANDO_QMTCONN,
	kFSM_ENVIANDO_QMTPUB,
	kFSM_ENVIANDO_MENSAJE_TXT,//enviando mensaje
	kFSM_ESPERANDO_RESPUESTA,
	kFSM_RESULTADO_ERROR,
	kFSM_RESULTADO_EXITOSO
};
/*******************************************************************************
 * External vars
 ******************************************************************************/

/*******************************************************************************
 * Public vars
 ******************************************************************************/

/*******************************************************************************
 * Public Prototypes
 ******************************************************************************/
status_t ec25Inicializacion(void);
status_t ec25EnviarMensajeDeTexto(uint8_t *mensaje, uint8_t size_mensaje );
uint8_t ec25Polling(void);

/** @} */ // end of X group
/** @} */ // end of X group

#endif /* SDK_PPH_EC25AU_H_ */
