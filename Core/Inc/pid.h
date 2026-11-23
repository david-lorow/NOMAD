/*
 * pid.h
 *
 *  Created on: Nov 20, 2026
 *      Author: davidlorow
 */
#ifndef PID_H
#define PID_H



#ifdef __cplusplus
extern "C" {
#endif

void PID_SetTarget(float target);
void PID_Step(float measured_speed);


#ifdef __cplusplus
}
#endif

#endif



