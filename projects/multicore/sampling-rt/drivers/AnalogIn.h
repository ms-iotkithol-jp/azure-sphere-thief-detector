#pragma once

#include <stdlib.h>
#include "os_hal_adc.h"

class AnalogIn
{
private:
	const adc_channel _Pin;

public:
	AnalogIn(int pin);
	~AnalogIn();

	AnalogIn(const AnalogIn&) = delete;
	AnalogIn& operator=(const AnalogIn) = delete;

	u32 Read()
	{
		if (mtk_os_hal_adc_start_ch(1 << _Pin) != 0) abort();

		u32 data;
		if (mtk_os_hal_adc_one_shot_get_data(_Pin, &data) != 0) abort();

		return data;
	}

};
