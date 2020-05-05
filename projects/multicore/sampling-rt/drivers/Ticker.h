#pragma once

#include "os_hal_gpt.h"

class Ticker
{
private:
	const gpt_num _Gpt;

public:
	Ticker(int gpt);
	~Ticker();

	Ticker(const Ticker&) = delete;
	Ticker& operator=(const Ticker) = delete;

	void Attach(void (*handler)(void*), float interval);

	void Restart()
	{
		mtk_os_hal_gpt_restart(_Gpt);
	}

};
