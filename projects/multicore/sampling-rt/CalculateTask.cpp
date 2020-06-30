#include "Config.h"
#include "CalculateTask.h"

#include <math.h>

#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "DigitalOut.h"

extern QueueHandle_t AdcDataWriteQueue;
extern QueueHandle_t AdcDataReadQueue;
extern QueueHandle_t MboxDataWriteQueue;

void CalculateTask(void* params)
{
	printf("Calculate Task Started.\n");

	while (AdcDataWriteQueue == nullptr || AdcDataReadQueue == nullptr || MboxDataWriteQueue == nullptr)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	while (true)
	{
		u16* adcData;
		if (xQueueReceive(AdcDataReadQueue, &adcData, portMAX_DELAY) != pdPASS)
		{
			printf("ERROR: Cannot pop from AdcDataReadQueue.\n");
			continue;
		}

		static DigitalOut debugCalculate{ DEBUG_PIN_CALCULATE };

		debugCalculate.Write(1);

		u32 sum = 0;
		for (int i = 0; i < ADC_DATA_NUMBER; ++i) sum += (u32)adcData[i];
		float average = (float)sum / (float)ADC_DATA_NUMBER;

		float rms = 0;
		for (int i = 0; i < ADC_DATA_NUMBER; ++i) rms += powf(((float)adcData[i] - average) / 4095.0f, 2.0f);
		rms = sqrtf(rms / (float)ADC_DATA_NUMBER);

		if (xQueueSendToBack(MboxDataWriteQueue, &rms, 0) != pdPASS)
		{
			printf("ERROR: Cannot push to MboxDataWriteQueue.\n");
		}

		if (xQueueSendToBack(AdcDataWriteQueue, &adcData, 0) != pdPASS)
		{
			printf("ERROR: Cannot push to AdcDataWriteQueue.\n");
			continue;
		}

		debugCalculate.Write(0);
	}
}
