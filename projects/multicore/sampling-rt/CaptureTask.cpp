#include "Config.h"
#include "CaptureTask.h"

#include "printf.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "AnalogIn.h"
#include "Ticker.h"
#include "DigitalOut.h"

static constexpr int MIC = ADC_CHANNEL_1;
static AnalogIn* Mic;

static Ticker* CaptureTicker;
SemaphoreHandle_t CaptureSemaphore;
QueueHandle_t AdcDataWriteQueue = nullptr;
QueueHandle_t AdcDataReadQueue = nullptr;

static u16 AdcData[2][ADC_DATA_NUMBER];
static u16* AdcDataPtr;
static int AdcDataCount = 0;

static void CaptureTickerHandler(void* cb_data)
{
	CaptureTicker->Restart();

	BaseType_t higher_priority_task_woken = pdFALSE;
	xSemaphoreGiveFromISR(CaptureSemaphore, &higher_priority_task_woken);
	portYIELD_FROM_ISR(higher_priority_task_woken);
}

void CaptureTask(void* params)
{
	printf("Capture Task Started.\n");

	static AnalogIn mic{ MIC };
	Mic = &mic;

	CaptureSemaphore = xSemaphoreCreateBinary();
	if (CaptureSemaphore == nullptr) abort();
	AdcDataWriteQueue = xQueueCreate(2, sizeof(u16*));
	if (AdcDataWriteQueue == nullptr) abort();
	AdcDataReadQueue = xQueueCreate(2, sizeof(u16*));
	if (AdcDataReadQueue == nullptr) abort();
	AdcDataPtr = &AdcData[0][0];
	printf("AdcData[0] = %#010lx\n", (unsigned long)AdcDataPtr);
	if (xQueueSendToBack(AdcDataWriteQueue, &AdcDataPtr, 0) != pdPASS) abort();
	AdcDataPtr = &AdcData[1][0];
	printf("AdcData[1] = %#010lx\n", (unsigned long)AdcDataPtr);
	if (xQueueSendToBack(AdcDataWriteQueue, &AdcDataPtr, 0) != pdPASS) abort();

	static Ticker captureTicker{ GPT3 };
	CaptureTicker = &captureTicker;
	CaptureTicker->Attach(CaptureTickerHandler, 1.0 / 4000);	// 1[sec.] / 4000 = 250[usec.]

	if (xQueueReceive(AdcDataWriteQueue, &AdcDataPtr, 0) != pdPASS) abort();
	while (true)
	{
		xSemaphoreTake(CaptureSemaphore, portMAX_DELAY);

		static DigitalOut debugCapturing{ DEBUG_PIN_CAPTURING };
		debugCapturing.Write(1);

		AdcDataPtr[AdcDataCount] = Mic->Read();
		++AdcDataCount;
		if (AdcDataCount >= ADC_DATA_NUMBER)
		{
			static DigitalOut debugTriggerCalculate{ DEBUG_PIN_TRIGGER_CALCULATE };
			debugTriggerCalculate.Write(1);

			u16* adcDataNextPtr;
			if (xQueueReceive(AdcDataWriteQueue, &adcDataNextPtr, 0) != pdPASS)
			{
				printf("ERROR: Cannot pop from AdcDataWriteQueue.\n");
			}
			else
			{
				if (xQueueSendToBack(AdcDataReadQueue, &AdcDataPtr, 0) != pdPASS)
				{
					printf("ERROR: Cannot push to AdcDataReadQueue.\n");
					if (xQueueSendToBack(AdcDataWriteQueue, &adcDataNextPtr, 0) != pdPASS) abort();
				}
				else
				{
					AdcDataPtr = adcDataNextPtr;
				}
			}

			AdcDataCount = 0;

			debugTriggerCalculate.Write(0);
		}

		debugCapturing.Write(0);
	}
}
