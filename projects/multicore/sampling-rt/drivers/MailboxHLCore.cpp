#include "MailboxHLCore.h"

#include <stdlib.h>

#include "FreeRTOS.h"
#include "semphr.h"

static constexpr u32 MBOX_MESSAGE_SIZE_MAX = 1048;
static constexpr u32 MBOX_IRQ_STATUS = 0x3;

//static SemaphoreHandle_t blockDeqSema;
SemaphoreHandle_t blockFifoSema;

static void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data* data)
{
	BaseType_t higher_priority_task_woken = pdFALSE;

	if (data->event.channel == OS_HAL_MBOX_CH0)
	{
		if (data->event.wr_int)
		{
			xSemaphoreGiveFromISR(blockFifoSema, &higher_priority_task_woken);
			portYIELD_FROM_ISR(higher_priority_task_woken);
		}
	}
}

//static void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data* data)
//{
//	BaseType_t higher_priority_task_woken = pdFALSE;
//
//	if (data->swint.channel == OS_HAL_MBOX_CH0)
//	{
//		if (data->swint.swint_sts & (1 << 1))
//		{
//			xSemaphoreGiveFromISR(blockDeqSema, &higher_priority_task_woken);
//			portYIELD_FROM_ISR(higher_priority_task_woken);
//		}
//	}
//}

MailboxHLCore::MailboxHLCore()
{
	mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

	//blockDeqSema = xSemaphoreCreateBinary();
	blockFifoSema = xSemaphoreCreateBinary();

	struct mbox_fifo_event mask;
	mask.channel = OS_HAL_MBOX_CH0;
	mask.ne_sts = 0;	/* FIFO Non-Empty interrupt */
	mask.nf_sts = 0;	/* FIFO Non-Full interrupt */
	mask.rd_int = 0;	/* Read FIFO interrupt */
	mask.wr_int = 1;	/* Write FIFO interrupt */
	mtk_os_hal_mbox_fifo_register_cb(OS_HAL_MBOX_CH0, mbox_fifo_cb, &mask);
	//mtk_os_hal_mbox_sw_int_register_cb(OS_HAL_MBOX_CH0, mbox_swint_cb, MBOX_IRQ_STATUS);

	if (GetIntercoreBuffers(&_SharedBuffer.Outbound, &_SharedBuffer.Inbound, &_SharedBuffer.BufferSize) == -1) abort();

	_Message.Size = _SharedBuffer.BufferSize <= MBOX_MESSAGE_SIZE_MAX ? _SharedBuffer.BufferSize : MBOX_MESSAGE_SIZE_MAX;
	_Message.Buffer = static_cast<u8*>(pvPortMalloc(_Message.Size));
	if (_Message.Buffer == nullptr) abort();
}

void MailboxHLCore::Send(size_t messageSendSize)
{
	EnqueueData(_SharedBuffer.Inbound, _SharedBuffer.Outbound, _SharedBuffer.BufferSize, _Message.Buffer, messageSendSize);
}

size_t MailboxHLCore::Receive()
{
	u32 messageReceiveSize = _Message.Size;
	int result = DequeueData(_SharedBuffer.Outbound, _SharedBuffer.Inbound, _SharedBuffer.BufferSize, _Message.Buffer, &messageReceiveSize);
	if (result != 0 || messageReceiveSize < MESSAGE_HEADER_SIZE) return 0;

	return messageReceiveSize;
}
