#include "DigitalOut.h"

DigitalOut::DigitalOut(int pin) :
	_Pin{ static_cast<os_hal_gpio_pin>(pin) }
{
	if (mtk_os_hal_gpio_request(_Pin) != 0) abort();
	if (mtk_os_hal_gpio_set_direction(_Pin, OS_HAL_GPIO_DIR_OUTPUT) != 0) abort();
}

DigitalOut::~DigitalOut()
{
	if (mtk_os_hal_gpio_free(_Pin) != 0) abort();
}
