/*
 * pid.cpp
 *
 *  Created on: Nov 20, 2026
 *      Author: davidlorow
 */


#include "pid.h"




class Session {

private:
	Data data;

public:
	Session(Data& init_data)
		: data(init_data) { }

	void setData() {

	}

	//Pure motor example
	float P_DIddy(float new_target) {
		if(data.d_current != new_target) data.d_target = new_target;
		else { return 0; }

		float error = data.d_target - data.d_current;
		if(error > 0) {
			data.d_current++;
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, data.d_current);
			return error = data.d_target - data.d_current;
		}
		else {
			data.d_current--;
			__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, data.d_current);
			return error = data.d_target - data.d_current;
		}
	}
};


Data init_data = {stop, stop};
Session controller(init_data);










