/*
* IoT Hub sample for Intel Edison board - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mraa.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "iothub_client.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "iothubtransportmqtt.h"
#include "jsondecoder.h"

#include "certs.h"

static const int LED_PIN = 13;

static bool is_last_message_received = false;
static mraa_gpio_context g_context;

static void blink_led()
{
    mraa_gpio_write(g_context, 1);
    usleep(100000);  // light on the LED for 0.1 second
    mraa_gpio_write(g_context, 0);
}

IOTHUBMESSAGE_DISPOSITION_RESULT receive_message_callback(IOTHUB_MESSAGE_HANDLE message, void *user_context_callback)
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

    printf("[Device] Received message: %s\n", s);

    MULTITREE_HANDLE tree = NULL;

    if (JSON_DECODER_OK == JSONDecoder_JSON_To_MultiTree(s, &tree))
    {
        const void *value = NULL;

        if (MULTITREE_OK == MultiTree_GetLeafValue(tree, "/command", &value))
        {
            if (0 == strcmp((const char *)value, "\"blink\""))
            {
                blink_led();
            }
            else if (0 == strcmp((const char *)value, "\"stop\""))
            {
                is_last_message_received = true;
            }
        }
    }

    free(s);
    MultiTree_Destroy(tree);

    return IOTHUBMESSAGE_ACCEPTED;
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
                char device_id[257];
                char *device_id_src = get_device_id(argv[1]);
                if (device_id_src == NULL)
                {
                    printf("[Device] ERROR: Cannot parse device id from IoT device connection string\n");
                    return 1;
                }
                snprintf(device_id, sizeof(device_id), "%s", device_id_src);
                free(device_id_src);

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

            IoTHubClient_LL_SetMessageCallback(iot_hub_client_handle, receive_message_callback, NULL);

            while (!is_last_message_received)
            {
                IoTHubClient_LL_DoWork(iot_hub_client_handle);
                usleep(100000);  // sleep for 0.1 second
            }

            IoTHubClient_LL_Destroy(iot_hub_client_handle);
        }
        platform_deinit();
    }

    return 0;
}
