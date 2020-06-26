#pragma once

#include <mhal_osai.h>

static constexpr int ADC_DATA_NUMBER = 4096;
static constexpr int MBOX_SEND_DATA_MAX_SIZE = 1024;

static constexpr int DEBUG_PIN_CAPTURING = 38;
static constexpr int DEBUG_PIN_TRIGGER_CALCULATE = 36;
static constexpr int DEBUG_PIN_CALCULATE = 39;

typedef struct {
	u32 dataSize;
	u8* dataPtr;
} QueueDataPacket;