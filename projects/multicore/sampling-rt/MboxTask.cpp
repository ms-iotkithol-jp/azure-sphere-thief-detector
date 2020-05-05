#include "Config.h"
#include "MboxTask.h"

#include "printf.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "MailboxHLCore.h"
#include "DigitalOut.h"

QueueHandle_t MboxDataWriteQueue = nullptr;

void MboxTask(void* params)
{
	printf("Mailbox Task Started.\n");

	MboxDataWriteQueue = xQueueCreate(10, sizeof(float));
	if (MboxDataWriteQueue == nullptr) abort();

	MailboxHLCore mbox;

	printf("Wait for ping...\n");
	size_t messageReceiveSize;
	while ((messageReceiveSize = mbox.Receive()) == 0) {}

	u8 messageHeader[MailboxHLCore::MESSAGE_HEADER_SIZE];
	memcpy(messageHeader, mbox.GetMessagePtr(), MailboxHLCore::MESSAGE_HEADER_SIZE);
	printf("Received message of %d bytes (from A7)\n", messageReceiveSize);

	while (true)
	{
		float mboxData;
		if (xQueueReceive(MboxDataWriteQueue, &mboxData, pdMS_TO_TICKS(100)) == pdPASS)
		{
			printf("RMS = %f\n", mboxData);

			memcpy(mbox.GetMessagePtr(), messageHeader, MailboxHLCore::MESSAGE_HEADER_SIZE);
			memcpy(&mbox.GetMessagePtr()[MailboxHLCore::MESSAGE_HEADER_SIZE], &mboxData, sizeof(mboxData));
			u32 messageSendSize = MailboxHLCore::MESSAGE_HEADER_SIZE + sizeof(mboxData);
			mbox.Send(messageSendSize);
		}

		size_t messageReceiveSize;
		while ((messageReceiveSize = mbox.Receive()) > 0)
		{
			printf("Received message of %d bytes (from A7)\n", messageReceiveSize);
		}
	}
}
