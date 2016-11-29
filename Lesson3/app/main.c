/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <mraa.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"

#include "certs.h"

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
        printf("[Device] ERROR: Failed to send message to Azure IoT Hub\n");
    }

    messagePending = false;
}

static void sendMessageAndBlink(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *deviceId)
{
    char buffer[256];
    sprintf(buffer, "{\"deviceId\":\"%s\",\"messageId\":%d}", deviceId, totalBlinkTimes);

    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, strlen(buffer));
    if (messageHandle == NULL)
    {
        printf("[Device] ERROR: Unable to create a new IoTHubMessage\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            printf("[Device] ERROR: Failed to hand over the message to IoTHubClient\n");
        }
        else
        {
            lastMessageSentTime = getTimeInSecond();
            messagePending = true;
            printf("[Device] Sending message #%d: %s\n", totalBlinkTimes, buffer);
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

char *get_device_id(char *str)
{
    char *substr = strstr(str, "DeviceId=");
    if (substr == NULL)
        return NULL;

    // skip "DeviceId="
    substr += 9;

    char *semicolon = strstr(substr, ";");
    int length = semicolon == NULL ? strlen(substr) : semicolon - substr;

    char *device_id = calloc(1, length + 1);
    memcpy(device_id, substr, length);
    device_id[length] = '\0';

    return device_id;
}

static char *readFile(char *fileName)
{
    FILE *fp;
    long lSize;
    char *buffer;

    fp = fopen(fileName, "rb");
    if (fp == NULL)
    {
        printf("[Device] ERROR: File %s doesn't exist!\n", fileName);
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    // Allocate memory for entire content
    buffer = calloc(1, lSize + 1);
    if (buffer == NULL)
    {
        fclose(fp);
        printf("[Device] ERROR: Failed to allocate memory.\n");
        return NULL;
    }

    // Read the file into the buffer
    if (1 != fread(buffer, lSize, 1, fp))
    {
        fclose(fp);
        free(buffer);
        printf("[Device] ERROR: Failed to read the file %s into memory.\n", fileName);
        return NULL;
    }

    fclose(fp);
    return buffer;
}

static bool setX509Certificate(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *deviceId)
{
    char certName[256];
    char keyName[256];
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    sprintf(certName, "%s/%s-cert.pem", cwd, deviceId);
    sprintf(keyName, "%s/%s-key.pem", cwd, deviceId);

    char *x509certificate = readFile(certName);
    char *x509privatekey = readFile(keyName);

    if (x509certificate == NULL ||
        x509privatekey == NULL ||
        IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_CERT, x509certificate) != IOTHUB_CLIENT_OK ||
        IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_X509_PRIVATE_KEY, x509privatekey) != IOTHUB_CLIENT_OK)
    {
        printf("[Device] ERROR: Failed to set options for x509.\n");
        return false;
    }

    free(x509certificate);
    free(x509privatekey);

    return true;
}

int main(int argc, char *argv[])
{
    printf("[Device] Starting the IoT Hub sample...\n");
    
    // argv[1] is the IoT Hub connection string.
    if (argc < 2)
    {
        printf("[Device] ERROR: IoT device connection string should be passed as a parameter\n");
        return 1;
    }

    char device_id[257];
    char *device_id_src = get_device_id(argv[1]);
    if (device_id_src == NULL)
    {
        printf("[Device] ERROR: Cannot parse device id from IoT device connection string\n");
        return 1;
    }
    strcpy(device_id, device_id_src);
    free(device_id_src);

    // Initialize GPIO and set its direction to output
    context = mraa_gpio_init(LED_PIN);
    mraa_gpio_dir(context, MRAA_GPIO_OUT);

    if (platform_init() != 0)
    {
        printf("[Device] ERROR: Failed to initialize the platform.\n");
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;
        if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(argv[1], MQTT_Protocol)) == NULL)
        {
            printf("[Device] ERROR: iotHubClientHandle is NULL!\n");
        }
        else
        {
            if (strstr(argv[1], "x509=true") != NULL)
            {
                // Use X.509 certificate authentication.
                if (!setX509Certificate(iotHubClientHandle, device_id))
                {
                    return 1;
                }
            }

            if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                printf("[Device] ERROR: Failed to set TrustedCerts option\n");
            }

            while ((totalBlinkTimes <= MAX_BLINK_TIMES) || messagePending)
            {
                if ((lastMessageSentTime + 2 <= getTimeInSecond()) && !messagePending)
                {
                    sendMessageAndBlink(iotHubClientHandle, device_id);
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
