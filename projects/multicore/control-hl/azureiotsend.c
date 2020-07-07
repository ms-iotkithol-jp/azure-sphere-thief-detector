#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/application.h>
#include <applibs/eventloop.h>

#include <sys/time.h>

// Azure IoT SDK

#include <iothub_client_core_common.h>
#include <iothub_device_client_ll.h>
#include <iothub_client_options.h>
#include <iothubtransportmqtt.h>
#include <iothub.h>
#include <azure_sphere_provisioning.h>
#include "eventloop_timer_utilities.h"

#include "azureiotsend.h"

// Azure IoT defines.
static char* scopeId; // ScopeId for DPS
static IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle = NULL;
static const int keepalivePeriodSeconds = 20;
static bool iothubAuthenticated = false;

static void AzureTimerEventHandler(EventLoopTimer* timer);

// Timer / polling
static EventLoop* eventLoop = NULL;
static EventLoopTimer* azureTimer = NULL;

// Azure IoT poll periods
static const int AzureIoTDefaultPollPeriodSeconds = 1;        // poll azure iot every second
static const int AzureIoTPollPeriodsPerTelemetry = 5;         // only send telemetry 1/5 of polls
static const int AzureIoTMinReconnectPeriodSeconds = 60;      // back off when reconnecting
static const int AzureIoTMaxReconnectPeriodSeconds = 10 * 60; // back off limit

static int azureIoTPollPeriodSeconds = -1;

/// <summary>
///     Converts AZURE_SPHERE_PROV_RETURN_VALUE to a string.
/// </summary>
static const char* GetAzureSphereProvisioningResultString(
    AZURE_SPHERE_PROV_RETURN_VALUE provisioningResult)
{
    switch (provisioningResult.result) {
    case AZURE_SPHERE_PROV_RESULT_OK:
        return "AZURE_SPHERE_PROV_RESULT_OK";
    case AZURE_SPHERE_PROV_RESULT_INVALID_PARAM:
        return "AZURE_SPHERE_PROV_RESULT_INVALID_PARAM";
    case AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_NETWORK_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY:
        return "AZURE_SPHERE_PROV_RESULT_DEVICEAUTH_NOT_READY";
    case AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_PROV_DEVICE_ERROR";
    case AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR:
        return "AZURE_SPHERE_PROV_RESULT_GENERIC_ERROR";
    default:
        return "UNKNOWN_RETURN_VALUE";
    }
}

/// <summary>
///     Callback invoked when the Azure IoT Hub send event request is processed.
/// </summary>
static void SendEventCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* context)
{
    Log_Debug("INFO: Azure IoT Hub send telemetry event callback: status code %d.\n", result);
}

static bool isAzureReady = false;
static char* dpsScopeId = NULL;

/// <summary>
///     Sets up the Azure IoT Hub connection (creates the iothubClientHandle)
///     When the SAS Token for a device expires the connection needs to be recreated
///     which is why this is not simply a one time call.
/// </summary>
void SetupAzureClient(char* scopeId)
{
    dpsScopeId = scopeId;
    if (iothubClientHandle != NULL) {
        IoTHubDeviceClient_LL_Destroy(iothubClientHandle);
    }
    isAzureReady = false;

    AZURE_SPHERE_PROV_RETURN_VALUE provResult =
        IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning(scopeId, 10000,
            &iothubClientHandle);
    Log_Debug("IoTHubDeviceClient_LL_CreateWithAzureSphereDeviceAuthProvisioning returned '%s'.\n",
        GetAzureSphereProvisioningResultString(provResult));

    if (provResult.result != AZURE_SPHERE_PROV_RESULT_OK) {
        Log_Debug("ERROR: Failed to create IoTHub Handle\n");
        return;
    }

    // Successfully connected, so make sure the polling frequency is back to the default
    iothubAuthenticated = true;

    if (IoTHubDeviceClient_LL_SetOption(iothubClientHandle, OPTION_KEEP_ALIVE,
        &keepalivePeriodSeconds) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: Failure setting Azure IoT Hub client option \"%s\".\n",
            OPTION_KEEP_ALIVE);
        return;
    }
    isAzureReady = true;

    eventLoop = EventLoop_Create();
    if (eventLoop == NULL) {
        Log_Debug("ERROR: Could not create event loop.\n");
        isAzureReady = false;
    }
    else {
        azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
        struct timespec azureTelemetryPeriod = { .tv_sec = azureIoTPollPeriodSeconds, .tv_nsec = 0 };
        azureTimer =
            CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
        if (azureTimer == NULL) {
            Log_Debug("ERROR: Failure creating azure timer!");
            isAzureReady = false;
        }
    }
}

static char timestamp[32];
static int soundDataId;

bool SendBinaryData(char* dataBuf, int dataSize, int soundDataIndex, int soundDataMax)
{
    if (!isAzureReady) {
        SetupAzureClient(dpsScopeId);
        if (!isAzureReady) {
            Log_Debug("INFO: Azure has not been ready yet.\n");
            return;
        }
    }

    if (soundDataIndex == 0) {
        soundDataId++;
        time_t now;
        now = time(NULL);
        struct tm tm;
        localtime_r(&now, &tm);
        sprintf(timestamp, "%04d%02d%02d%02d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    }
    // generate a sized binary data
//    uint8_t telemetryBuffer[dataSize];
//    for (int p = 0; p < dataSize; p++) {
//        telemetryBuffer[p] = (uint8_t)((p + soundDataIndex) % 256);
//    }
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(dataBuf, dataSize);
    IoTHubMessage_SetProperty(messageHandle, "datatype", "sound");
    uint8_t dataSizeStr[8];
    sprintf(dataSizeStr, "%d", dataSize);
    IoTHubMessage_SetProperty(messageHandle, "dataLength", dataSizeStr);
    sprintf(dataSizeStr, "%d", soundDataMax);
    IoTHubMessage_SetProperty(messageHandle, "dataPacketNo", dataSizeStr);
    sprintf(dataSizeStr, "%d", soundDataIndex);
    IoTHubMessage_SetProperty(messageHandle, "dataPacketIndex", dataSizeStr);
    sprintf(dataSizeStr, "%d", soundDataId);
    IoTHubMessage_SetProperty(messageHandle, "dataPacketId", dataSizeStr);
    IoTHubMessage_SetProperty(messageHandle, "dataFormat", "csv");
    IoTHubMessage_SetProperty(messageHandle, "dataTimestamp", timestamp);


    if (IoTHubDeviceClient_LL_SendEventAsync(iothubClientHandle, messageHandle, SendEventCallback,
        /*&callback_param*/ NULL) != IOTHUB_CLIENT_OK) {
        Log_Debug("ERROR: failure requesting IoTHubClient to send telemetry event.\n");
        return false;
    }
    else {
        Log_Debug("INFO: IoTHubClient accepted the telemetry event for delivery.\n");
    }

    IoTHubMessage_Destroy(messageHandle);
    
    return true;
}

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer* timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        Log_Debug("ERROR: failure notify event consuming");
        return;
    }

    if (iothubAuthenticated) {
        Log_Debug("INFO: iot hub work doing...");
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

bool WorkOnEventLoop()
{
    EventLoop_Run_Result result = EventLoop_Run(eventLoop, -1, true);
    if (result == EventLoop_Run_Failed) {
        return false;
    }
    return true;
}
