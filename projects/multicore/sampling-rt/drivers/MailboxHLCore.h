#pragma once

#include "os_hal_mbox.h"
#include "os_hal_mbox_shared_mem.h"

class MailboxHLCore
{
private:
	struct
	{
		BufferHeader* Outbound;
		BufferHeader* Inbound;
		u32 BufferSize;
	}
	_SharedBuffer;

	struct
	{
		u8* Buffer;
		u32 Size;
	}
	_Message;

public:
	static constexpr u32 MESSAGE_HEADER_SIZE = 20; /* UUID 16B, Reserved 4B */

public:
	MailboxHLCore();

	MailboxHLCore(const MailboxHLCore&) = delete;
	MailboxHLCore& operator=(const MailboxHLCore) = delete;

	u8* GetMessagePtr() const { return _Message.Buffer; }
	u32 GetMessageSizeMax() const { return _Message.Size; }

	void Send(size_t messageSendSize);
	size_t Receive();

};
