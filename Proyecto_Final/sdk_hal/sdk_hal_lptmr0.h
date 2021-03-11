/*
 * sdk_hal_lptmr0.h
 *
 *  Created on: 10/03/2021
 *      Author: JAVIER CASALLAS
 */

#ifndef SDK_HAL_LPTMR0_H_
#define SDK_HAL_LPTMR0_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "fsl_lptmr.h"

/*!
 * @addtogroup HAL
 * @{
 */
/*!
 * @addtogroup LPTMR0
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

/*******************************************************************************
 * Public Prototypes
 ******************************************************************************/
void lptmr0Init(void);

uint32_t lptmr0GetTimeValue(void);
void lptmr0SetTimeValue(uint32_t timeValue);

/** @} */ // end of X group
/** @} */ // end of X group

#endif /* SDK_HAL_LPTMR0_H_ */
