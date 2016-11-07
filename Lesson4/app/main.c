/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <mraa.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransporthttp.h"
#include "jsondecoder.h"

const int LED_PIN = 13;

bool lastMessageReceived = false;
mraa_gpio_context context;

static void blinkLED()
{
    mraa_gpio_write(context, 1);
    usleep(100000); // light on the LED for 0.1 second
    mraa_gpio_write(context, 0);
}

IOTHUBMESSAGE_DISPOSITION_RESULT receiveMessageCallback(IOTHUB_MESSAGE_HANDLE message, void *userContextCallback)
{
    const unsigned char *buffer = NULL;
    size_t size = 0;

    if (IOTHUB_MESSAGE_OK != IoTHubMessage_GetByteArray(message, &buffer, &size))
        return IOTHUBMESSAGE_ABANDONED;

    // message needs to be converted to zero terminated string
    char *s = malloc(size + 1);

    if (NULL == s)
        return IOTHUBMESSAGE_ABANDONED;

    strncpy(s, buffer, size);
    s[size] = 0;

    printf("[Device] Received message: %s\r\n", s);

    MULTITREE_HANDLE tree = NULL;

    if (JSON_DECODER_OK == JSONDecoder_JSON_To_MultiTree(s, &tree))
    {
        const void *value = NULL;

        if (MULTITREE_OK == MultiTree_GetLeafValue(tree, "/command", &value))
        {
            if (0 == strcmp((const char *)value, "\"blink\""))
            {
                blinkLED();
            }
            else if (0 == strcmp((const char *)value, "\"stop\""))
            {
                lastMessageReceived = true;
            }
        }
    }

    free(s);
    MultiTree_Destroy(tree);

    return IOTHUBMESSAGE_ACCEPTED;
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

    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

    (void)printf("[Device] Starting the IoTHub client sample HTTP...\r\n");

    if (platform_init() != 0)
    {
        printf("[Device] Failed to initialize the platform.\r\n");
    }
    else
    {
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(argv[1], HTTP_Protocol)) == NULL)
        {
            (void)printf("[Device] ERROR: iotHubClientHandle is NULL!\r\n");
        }
        else
        {
            unsigned int minimumPollingTime = 1; // poll message right away instead of waiting
            IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_MIN_POLLING_TIME, &minimumPollingTime);
            IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, receiveMessageCallback, NULL);

            while (!lastMessageReceived)
            {
                IoTHubClient_LL_DoWork(iotHubClientHandle);
                usleep(100000); // sleep for 0.1 second
            }

            IoTHubClient_LL_Destroy(iotHubClientHandle);
        }
        platform_deinit();
    }

    return 0;
}
