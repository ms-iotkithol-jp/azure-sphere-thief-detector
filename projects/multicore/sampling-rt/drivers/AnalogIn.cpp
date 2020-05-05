#include "AnalogIn.h"

AnalogIn::AnalogIn(int pin) :
	_Pin{ static_cast<adc_channel>(pin) }
{
	if (mtk_os_hal_adc_ctlr_init(ADC_PMODE_ONE_TIME, ADC_FIFO_DIRECT, 1 << _Pin) != 0) abort();
}

AnalogIn::~AnalogIn()
{
	if (mtk_os_hal_adc_ctlr_deinit() != 0) abort();
}
