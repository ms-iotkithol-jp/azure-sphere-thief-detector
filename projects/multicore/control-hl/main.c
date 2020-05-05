#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/application.h>
#include <sys/time.h>
#include <sys/socket.h>

static const char RTAppCID[] = "4187B775-2AE9-4670-B151-1FA335D51398";

static const GPIO_Id BUTTON_PIN = 12;

int main(void)
{
    int buttonFd = GPIO_OpenAsInput(BUTTON_PIN);

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

    Log_Debug("SEND: \"ping\"\n");
    send(mailboxFd, "ping", 4, 0);

    while (true)
    {
        GPIO_Value_Type value;
        GPIO_GetValue(buttonFd, &value);
        if (value == GPIO_Value_Low)
        {
            Log_Debug("SEND: \"click\"\n");
            send(mailboxFd, "click", 5, 0);

            const struct timespec sleepTime = { .tv_sec = 1, .tv_nsec = 0 };
            nanosleep(&sleepTime, NULL);
        }

        uint8_t recvData[4];
        ssize_t recvSize = recv(mailboxFd, recvData, sizeof(recvData), 0);
        if (recvSize < 0)
        {
            if (errno != EAGAIN) Log_Debug("ERROR: %d %s\n", errno, strerror(errno));
            continue;
        }
        
        if (recvSize != 4)
        {
            Log_Debug("ERROR: size mismatch.\n");
            continue;
        }

        float rms;
        memcpy(&rms, recvData, sizeof(rms));

        Log_Debug("RMS = %f\n", rms);
    }
}
