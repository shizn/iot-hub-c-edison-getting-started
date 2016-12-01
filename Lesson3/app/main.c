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

static int g_total_blink_times = 1;
static int g_last_message_sent_time = 0;
static bool g_is_message_pending = false;
static mraa_gpio_context g_context;

int get_time_in_seconds()
{
    struct timeval now;
    gettimeofday(&now, NULL);

    return now.tv_sec;
}

static void send_callback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *user_context_callback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        mraa_gpio_write(g_context, 1);
        usleep(100000);  // light on the LED for 0.1 second
        mraa_gpio_write(g_context, 0);
    }
    else
    {
        printf("[Device] ERROR: Failed to send message to Azure IoT Hub\n");
    }

    g_is_message_pending = false;

    IoTHubMessage_Destroy((IOTHUB_MESSAGE_HANDLE)user_context_callback);
}

static void send_message_and_blink(IOTHUB_CLIENT_LL_HANDLE iot_hub_client_handle, char *device_id)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "{\"deviceId\":\"%s\",\"messageId\":%d}", device_id, g_total_blink_times);

    IOTHUB_MESSAGE_HANDLE message_handle = IoTHubMessage_CreateFromByteArray(buffer, strlen(buffer));
    if (message_handle == NULL)
    {
        printf("[Device] ERROR: Unable to create a new IoTHubMessage\n");
    }
    else
    {
        if (IoTHubClient_LL_SendEventAsync(iot_hub_client_handle, message_handle, send_callback, message_handle) != IOTHUB_CLIENT_OK)
        {
            printf("[Device] ERROR: Failed to hand over the message to IoTHubClient\n");
        }
        else
        {
            g_last_message_sent_time = get_time_in_seconds();
            g_is_message_pending = true;
            printf("[Device] Sending message #%d: %s\n", g_total_blink_times, buffer);
        }
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

static char *read_file(char *fileName)
{
    FILE *fp;
    int64_t lSize;
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

static bool set_x509_certificate(IOTHUB_CLIENT_LL_HANDLE iot_hub_client_handle, char *device_id)
{
    char cert_name[256];
    char key_name[256];
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    snprintf(cert_name, sizeof(cert_name), "%s/%s-cert.pem", cwd, device_id);
    snprintf(key_name, sizeof(key_name), "%s/%s-key.pem", cwd, device_id);

    char *x509_certificate = read_file(cert_name);
    char *x509_private_key = read_file(key_name);

    if (x509_certificate == NULL ||
        x509_private_key == NULL ||
        IoTHubClient_LL_SetOption(iot_hub_client_handle, OPTION_X509_CERT, x509_certificate) != IOTHUB_CLIENT_OK ||
        IoTHubClient_LL_SetOption(iot_hub_client_handle, OPTION_X509_PRIVATE_KEY, x509_private_key) != IOTHUB_CLIENT_OK)
    {
        printf("[Device] ERROR: Failed to set options for x509.\n");
        return false;
    }

    free(x509_certificate);
    free(x509_private_key);

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
    snprintf(device_id, sizeof(device_id), "%s", device_id_src);
    free(device_id_src);

    // Initialize GPIO and set its direction to output
    g_context = mraa_gpio_init(LED_PIN);
    mraa_gpio_dir(g_context, MRAA_GPIO_OUT);

    if (platform_init() != 0)
    {
        printf("[Device] ERROR: Failed to initialize the platform.\n");
    }
    else
    {
        IOTHUB_CLIENT_LL_HANDLE iot_hub_client_handle;
        if ((iot_hub_client_handle = IoTHubClient_LL_CreateFromConnectionString(argv[1], MQTT_Protocol)) == NULL)
        {
            printf("[Device] ERROR: iot_hub_client_handle is NULL!\n");
        }
        else
        {
            if (strstr(argv[1], "x509=true") != NULL)
            {
                // Use X.509 certificate authentication.
                if (!set_x509_certificate(iot_hub_client_handle, device_id))
                {
                    return 1;
                }
            }

            if (IoTHubClient_LL_SetOption(iot_hub_client_handle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
            {
                printf("[Device] ERROR: Failed to set TrustedCerts option\n");
            }

            while ((g_total_blink_times <= MAX_BLINK_TIMES) || g_is_message_pending)
            {
                if ((g_last_message_sent_time + 2 <= get_time_in_seconds()) && !g_is_message_pending)
                {
                    send_message_and_blink(iot_hub_client_handle, device_id);
                    g_total_blink_times++;
                }

                IoTHubClient_LL_DoWork(iot_hub_client_handle);
                usleep(100000);  // sleep for 0.1 second
            }

            IoTHubClient_LL_Destroy(iot_hub_client_handle);
        }
        platform_deinit();
    }

    return 0;
}
