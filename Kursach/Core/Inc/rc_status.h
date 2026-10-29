/*
 * rc_status.h
 *
 *  Created on: 13 окт. 2026 г.
 *      Author: Nikita
 */

#ifndef INC_RC_STATUS_H_
#define INC_RC_STATUS_H_

#include "inttypes.h"

typedef struct
{
	uint16_t voltage_raw;
	uint16_t current_raw;
	float target;
	float voltage;
	float current;
} RC_Struct_t;


void RC_PrintStatus(RC_Struct_t* rc);
void RC_Update(RC_Struct_t* rc);

#endif /* INC_RC_STATUS_H_ */
