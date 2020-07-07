#include "Config.h"
#include "MboxTask.h"

#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "MailboxHLCore.h"
#include "DigitalOut.h"

QueueHandle_t MboxDataWriteQueue = nullptr;
extern QueueHandle_t AdcDataWriteQueue;
extern QueueHandle_t AdcDataReadQueue;

void MboxTask(void* params)
{
	printf("Mailbox Task Started.\n");

	MboxDataWriteQueue = xQueueCreate(10, sizeof(float));
	if (MboxDataWriteQueue == nullptr) abort();

	while (AdcDataWriteQueue == nullptr || AdcDataReadQueue == nullptr || MboxDataWriteQueue == nullptr)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
	}


	MailboxHLCore mbox;

	printf("Wait for ping...\n");
	size_t messageReceiveSize;
	while ((messageReceiveSize = mbox.Receive()) == 0) {}

	/*
	u16* tmpPtr = nullptr;
	if (xQueueSendToBack(AdcDataReadQueue, &tmpPtr, 0) != pdPASS) {
		printf("ERROR: Failed to send start message to CaptureTask.\n");
		abort();
	}
	*/

	u8 messageHeader[MailboxHLCore::MESSAGE_HEADER_SIZE];
	memcpy(messageHeader, mbox.GetMessagePtr(), MailboxHLCore::MESSAGE_HEADER_SIZE);
	printf("Received message of %d bytes (from A7)\n", messageReceiveSize);
	u8 startMarkMessage[8] = "start";

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

		int sendDataUnitSize = 1024;
		u8* sendDataPtr = (u8*)adcData;
		size_t sendDataLength = ADC_DATA_NUMBER * sizeof(u16);
		size_t sendSize = 0;

		memcpy(mbox.GetMessagePtr(), messageHeader, MailboxHLCore::MESSAGE_HEADER_SIZE);
		memcpy(&mbox.GetMessagePtr()[MailboxHLCore::MESSAGE_HEADER_SIZE], startMarkMessage, sizeof(startMarkMessage));
		u32 messageSendSize = MailboxHLCore::MESSAGE_HEADER_SIZE + sizeof(startMarkMessage);
		mbox.Send(messageSendSize);

		while (sendSize < sendDataLength) {
			memcpy(mbox.GetMessagePtr(), messageHeader, MailboxHLCore::MESSAGE_HEADER_SIZE);
			memcpy(&mbox.GetMessagePtr()[MailboxHLCore::MESSAGE_HEADER_SIZE], &sendDataPtr[sendSize], sendDataUnitSize);
			messageSendSize = MailboxHLCore::MESSAGE_HEADER_SIZE + sendDataUnitSize;
			mbox.Send(messageSendSize);
			sendSize += sendDataUnitSize;
			if (sendSize+sendDataUnitSize > sendDataLength) {
				sendDataUnitSize -= ((sendSize + sendDataUnitSize) - sendDataLength);
			}
		}

		if (xQueueSendToBack(AdcDataWriteQueue, &adcData, 0) != pdPASS)
		{
			printf("ERROR: Cannot push to AdcDataWriteQueue.\n");
			continue;
		}

		debugCalculate.Write(0);

		size_t messageReceiveSize;
		while ((messageReceiveSize = mbox.Receive()) > 0)
		{
			printf("Received message of %d bytes (from A7)\n", messageReceiveSize);
		}
	}
}
