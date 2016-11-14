/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mraa.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"

static const int MAX_BLINK_TIMES = 20;
static const int LED_PIN = 13;

static int totalBlinkTimes = 1;
static int lastMessageSentTime = 0;
static bool messagePending = false;
static mraa_gpio_context context;

int getTimeInSecond()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_sec;
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        mraa_gpio_write(context, 1);
        usleep(100000); // light on the LED for 0.1 second
        mraa_gpio_write(context, 0);
    }
    else
    {
        printf("[Device] Failed to send message to Azure IoT Hub\r\n");
    }

    messagePending = false;
}

static void sendMessageAndBlink(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle)
{
    // TODO: change the message.
    char buffer[256];
    sprintf(buffer, "{\"deviceId\":\"%s\",\"messageId\":%d}", "myedisonboard", totalBlinkTimes);

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, strlen(buffer));
    if (messageHandle == NULL)
    {
        printf("[Device] unable to create a new IoTHubMessage\r\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            printf("[Device] Failed to hand over the message to IoTHubClient\r\n");
        }
        else
        {
            lastMessageSentTime = getTimeInSecond();
            messagePending = true;
            printf("[Device] Sending message #%d: %s\r\n", totalBlinkTimes, buffer);
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("[Device] IoT Hub connection string should be passed as a parameter\r\n");
        return 1;
    }

    // Initialize GPIO and set its direction to output
    context = mraa_gpio_init(LED_PIN);
    mraa_gpio_dir(context, MRAA_GPIO_OUT);

    if (platform_init() != 0)
    {
        printf("Failed to initialize the platform.\r\n");
    }
    else
    {
        (void)printf("Starting the IoTHub client sample HTTP...\r\n");

        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(argv[1], HTTP_Protocol)) == NULL)
        {
            (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            while ((totalBlinkTimes <= MAX_BLINK_TIMES) || messagePending)
            {
                if ((lastMessageSentTime + 2 <= getTimeInSecond()) && !messagePending)
                {
                    sendMessageAndBlink(iotHubClientHandle);
                    totalBlinkTimes++;
                }

                IoTHubClient_LL_DoWork(iotHubClientHandle);
                usleep(100000); // sleep for 0.1 second
            }

            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }

    return 0;
}
