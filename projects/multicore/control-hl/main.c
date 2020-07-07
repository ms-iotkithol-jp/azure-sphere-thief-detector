#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <applibs/gpio.h>
#include <applibs/log.h>
#include <applibs/application.h>
#include <applibs/networking.h>
#include <sys/time.h>

static const char RTAppCID[] = "C0C33382-EBB5-486D-AB15-84DB92A387F3";

// Azure IoT Sending
#include "azureiotsend.h"

#include "eventloop_timer_utilities.h"

static const GPIO_Id BUTTON_PIN_A = 12;
static const GPIO_Id BUTTON_PIN_B = 13;

static const int ADC_DATA_NUMBER_BYTES = 4096 * sizeof(uint16_t);
static const int MBOX_SEND_DATA_MAX_SIZE = 1024;

int main(int argc, char* argv[])
{
    int buttonFdA = GPIO_OpenAsInput(BUTTON_PIN_A);
    int buttonFdB = GPIO_OpenAsInput(BUTTON_PIN_B);

    int mailboxFd = Application_Connect(RTAppCID);
    if (mailboxFd < 0)
    {
        Log_Debug("ERROR: %d %s\n", errno, strerror(errno));
        abort();
    }
    const struct timeval recvTimeout = { .tv_sec = 1, .tv_usec = 0 };
    if (setsockopt(mailboxFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout)) != 0)
    {
        Log_Debug("ERROR: %d %s\n", errno, strerror(errno));
        abort();
    }

    bool isNetworkingReady = false;
    if ((Networking_IsNetworkingReady(&isNetworkingReady) == -1) || !isNetworkingReady) {
        Log_Debug("WARNING: Network is not ready. Device cannot connect until network is ready.\n");
    }
    if (argc > 1) {
        if (isNetworkingReady) {
            Log_Debug("Using Azure IoT DPS Scope ID %s\n", argv[1]);
            SetupAzureClient(argv[1]);
        }
    }
    else {
        Log_Debug("ScopeId needs to be set in the app_manifest CmdArgs for IoT Hub telemetry\n");
        isNetworkingReady = false;
    }

    Log_Debug("SEND: \"ping\"\n");
    size_t sendSize = send(mailboxFd, "ping", 4, 0);

    bool hasAPushed = false;
    bool hasBPushed = false;
    bool isSending = false;
    int sendUnitSize = 2048;
    int captureHz = 4000;
    int sendUnitMilliSec = 1024;
    int capturingUnitSize = 4096;
    int mailBoxMaxByte = 1024;
    int packetNo = capturingUnitSize * sizeof(uint16_t) / mailBoxMaxByte;
    int packetIndex = 0;
    uint8_t recvData[MBOX_SEND_DATA_MAX_SIZE];

    while (true)
    {
        GPIO_Value_Type valueA, valueB;
        GPIO_GetValue(buttonFdA, &valueA);
        if (valueA == GPIO_Value_Low)
        {
            Log_Debug("Button A: \"click\"\n");
            send(mailboxFd, "click", 5, 0);
            hasAPushed = true;
            const struct timespec sleepTime = { .tv_sec = 1, .tv_nsec = 0 };
            nanosleep(&sleepTime, NULL);
        }
        GPIO_GetValue(buttonFdB, &valueB);
        if (valueB == GPIO_Value_Low) {
            Log_Debug("Button B: \"click\"\n");
            hasBPushed = true;
            const struct timespec sleepTime = { .tv_sec = 1, .tv_nsec = 0 };
            nanosleep(&sleepTime, NULL);
        }

        ssize_t recvSize = recv(mailboxFd, recvData, sizeof(recvData), 0);
        if (recvSize < 0)
        {
            if (errno != EAGAIN) Log_Debug("ERROR: %d %s\n", errno, strerror(errno));
            continue;
        }
        if (recvSize == 8) {
            if (hasBPushed) {
                if (isSending) {
                    isSending = false;
                }
                hasBPushed = false;
            }
            if (hasAPushed) {
                isSending = true;
                packetIndex = 0;
                hasAPushed = false;
            }
            continue;
        }
        if (isSending&&recvSize == MBOX_SEND_DATA_MAX_SIZE)
        {
            int retry = 0;
            /*
            while (!sendStatus) { */
               bool sendStatus = SendBinaryData(recvData, recvSize, packetIndex, packetNo);
                if (!sendStatus) {
                    Log_Debug("INFO: Failed to send data... retry=%d\n", retry++);
                }
          /*  }
            */
            packetIndex++;
            if (packetIndex >= packetNo) {
                packetIndex = 0;
            }
        }
        WorkOnEventLoop();
    }
}
