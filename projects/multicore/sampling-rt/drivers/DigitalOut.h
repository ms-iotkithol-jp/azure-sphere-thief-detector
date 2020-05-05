#pragma once

#include <stdlib.h>
#include "os_hal_gpio.h"

class DigitalOut
{
private:
	const os_hal_gpio_pin _Pin;

public:
	DigitalOut(int pin);
	~DigitalOut();

	DigitalOut(const DigitalOut&) = delete;
	DigitalOut& operator=(const DigitalOut) = delete;

	void Write(int value)
	{
#ifdef NDEBUG
		mtk_os_hal_gpio_set_output(_Pin, value ? OS_HAL_GPIO_DATA_HIGH : OS_HAL_GPIO_DATA_LOW);
#else
		if (mtk_os_hal_gpio_set_output(_Pin, value ? OS_HAL_GPIO_DATA_HIGH : OS_HAL_GPIO_DATA_LOW) != 0) abort();
#endif
	}

};
