#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Temperature_led
{
private:
    gpio_num_t gpio_num;
    uint8_t gpio_state = 0;
    uint32_t intervall = 0;
public:
    Temperature_led(gpio_num_t gpio_num);
    ~Temperature_led();

    void toggle();

    void start_blink(uint32_t intervall);
    void stop_blink();
    uint32_t get_intervall();
};


