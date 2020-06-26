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

//#define	MBOX_SEND_DATA_MAX_SIZE 1024
static u8 mboxSendData[MBOX_SEND_DATA_MAX_SIZE];
static QueueDataPacket sendDataPacket[ADC_DATA_NUMBER/MBOX_SEND_DATA_MAX_SIZE*sizeof(u16)];

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

		/*
		u32 sum = 0;
		for (int i = 0; i < ADC_DATA_NUMBER; ++i) sum += adcData[i];
		float average = (float)sum / (float)ADC_DATA_NUMBER;

		float rms = 0;
		for (int i = 0; i < ADC_DATA_NUMBER; ++i) rms += powf(((float)adcData[i] - average) / 4095.0f, 2.0f);
		rms = sqrtf(rms / (float)ADC_DATA_NUMBER);

		if (xQueueSendToBack(MboxDataWriteQueue, &rms, 0) != pdPASS)
		{
			printf("ERROR: Cannot push to MboxDataWriteQueue.\n");
		}
		
		u32 max = 0;
		for (int i = 0; i < ADC_DATA_NUMBER; i++)
		{
			if (max < abs(adcData[i])) {
				max = abs(adcData[i]);
			}
		}
		adcData[i] = (u32)(((float)max)*adc
		*/

		int round = 0;
		int sendDataNum = 0;
		int sendDataUnitSize = MBOX_SEND_DATA_MAX_SIZE / sizeof(u16);
		u8* dataPtr = (u8*)adcData;
		while (sendDataNum < ADC_DATA_NUMBER) {
			sendDataPacket[round].dataSize = sendDataUnitSize;
			sendDataPacket[round].dataPtr = dataPtr;
			if (xQueueSendToBack(AdcDataWriteQueue, &(sendDataPacket[round]), 0) != pdPASS)
			{
				printf("ERROR: Cannot push to AdcDataWriteQueue.\n");
				continue;
			}
			dataPtr += MBOX_SEND_DATA_MAX_SIZE;
			sendDataNum += sendDataUnitSize;
			round++;
		}
		debugCalculate.Write(0);
	}
}
