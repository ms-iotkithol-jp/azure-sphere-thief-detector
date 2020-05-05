#include "Ticker.h"

Ticker::Ticker(int gpt) :
	_Gpt{ static_cast<gpt_num>(gpt) }
{
}

Ticker::~Ticker()
{
	// TODO
}

void Ticker::Attach(void (*handler)(void*), float interval)
{
	struct os_gpt_int gptInt;
	gptInt.gpt_cb_hdl = handler;
	gptInt.gpt_cb_data = nullptr;
	mtk_os_hal_gpt_config(_Gpt, false, &gptInt);
	mtk_os_hal_gpt_reset_timer(_Gpt, interval * 1000000, false);
	mtk_os_hal_gpt_start(_Gpt);
}
