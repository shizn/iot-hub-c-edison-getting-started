/*
* IoT Hub Intel Edison C Blink - Microsoft Sample Code - Copyright (c) 2016 - Licensed MIT
*/

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <mraa.h>

#define MAX_BLINK_TIMES 20
#define LED_PIN 13

int main(int argc, char *argv[])
{
    mraa_gpio_context context = mraa_gpio_init(LED_PIN);
    mraa_gpio_dir(context, MRAA_GPIO_OUT);

    int blinkNumber = 0;
    while (MAX_BLINK_TIMES > blinkNumber++)
    {
        printf("[Device] #%d Blink LED \n", blinkNumber);
        mraa_gpio_write(context, 1);
        usleep(200000); // light on the LED for 0.2 second
        mraa_gpio_write(context, 0);
        sleep(2); // turn off the LED for 2 seconds
    }
}
