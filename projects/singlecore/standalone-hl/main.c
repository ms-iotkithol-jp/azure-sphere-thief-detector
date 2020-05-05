#include <stdlib.h>
#include <math.h>

#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/adc.h>
#include <applibs/gpio.h>

#include "mt3620.h"
#include "eventloop_timer_utilities.h"
#include "queue.h"  // https://github.com/seifzadeh/c-pthread-queue

static int DebugCapturingFd = -1;
static int DebugTriggerCalculateFd = -1;
static int DebugCalculateFd = -1;

static int AdcFd = -1;
static int AdcBitCount = -1;
static const float AdcMaxVoltage = 2.5f;

static EventLoop* EventLoopId = NULL;
static EventLoopTimer* CaptureTimer = NULL;

static void* AdcDataQueueBuffer[10];
static queue_t AdcDataQueue = QUEUE_INITIALIZER(AdcDataQueueBuffer);

static const int ADC_DATA_NUMBER = 4000;    // 4000 x 250[usec.] = 1[sec.]
static uint32_t AdcData[2][4000];
static int AdcDataBank = 0;
static int AdcDataCount = 0;

static void CaptureTask(EventLoopTimer* timer)
{
    GPIO_SetValue(DebugCapturingFd, GPIO_Value_High);

    if (ConsumeEventLoopTimerEvent(timer) != 0) abort();

    ADC_Poll(AdcFd, MT3620_ADC_CHANNEL1, &AdcData[AdcDataBank][AdcDataCount]);
    ++AdcDataCount;
    if (AdcDataCount >= ADC_DATA_NUMBER)
    {
        GPIO_SetValue(DebugTriggerCalculateFd, GPIO_Value_High);

        uint32_t* sendData = &AdcData[AdcDataBank][0];
        queue_enqueue(&AdcDataQueue, sendData);

        AdcDataBank = (AdcDataBank + 1) % 2;
        AdcDataCount = 0;

        GPIO_SetValue(DebugTriggerCalculateFd, GPIO_Value_Low);
    }

    GPIO_SetValue(DebugCapturingFd, GPIO_Value_Low);
}

static void* CalculateTask(void* arg)
{
    while (true)
    {
        uint32_t* adcData = queue_dequeue(&AdcDataQueue);

        GPIO_SetValue(DebugCalculateFd, GPIO_Value_High);

        uint32_t sum = 0;
        for (int i = 0; i < ADC_DATA_NUMBER; ++i) sum += adcData[i];
        float average = (float)sum / (float)ADC_DATA_NUMBER;

        float rms = 0;
        for (int i = 0; i < ADC_DATA_NUMBER; ++i) rms += powf(((float)adcData[i] - average) / (float)((1 << AdcBitCount) - 1), 2.0f);
        rms = sqrtf(rms / (float)ADC_DATA_NUMBER);

        Log_Debug("RMS = %f\n", rms);

        GPIO_SetValue(DebugCalculateFd, GPIO_Value_Low);
    }

    return NULL;
}

static void InitPeripheralsAndHandlers(void)
{
    DebugCapturingFd = GPIO_OpenAsOutput(MT3620_GPIO38, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    DebugTriggerCalculateFd = GPIO_OpenAsOutput(MT3620_GPIO36, GPIO_OutputMode_PushPull, GPIO_Value_Low);
    DebugCalculateFd = GPIO_OpenAsOutput(MT3620_GPIO39, GPIO_OutputMode_PushPull, GPIO_Value_Low);

    AdcFd = ADC_Open(MT3620_ADC_CONTROLLER0);
    AdcBitCount = ADC_GetSampleBitCount(AdcFd, MT3620_ADC_CHANNEL1);
    ADC_SetReferenceVoltage(AdcFd, MT3620_ADC_CHANNEL1, AdcMaxVoltage);

    EventLoopId = EventLoop_Create();

    struct timespec capturePeriod = { .tv_sec = 0, .tv_nsec = 250000 }; // 250[usec.]
    CaptureTimer = CreateEventLoopPeriodicTimer(EventLoopId, &CaptureTask, &capturePeriod);
}

int main(int argc, char *argv[])
{
    Log_Debug("standalone-hl starting.\n");
    InitPeripheralsAndHandlers();

    pthread_t calculateThread;
    pthread_create(&calculateThread, NULL, CalculateTask, NULL);

    while (true)
    {
        EventLoop_Run(EventLoopId, -1, true);
    }

    return 0;
}
