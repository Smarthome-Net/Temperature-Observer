#include "temperature-led.h"

static TaskHandle_t blink_task_handle;

static void blink_task(void *pvParameters) 
{
    Temperature_led *ref = (Temperature_led *) pvParameters;
    while(1) {
        ref->toggle();
        vTaskDelay(ref->get_intervall() / portTICK_PERIOD_MS);
    }
}

Temperature_led::Temperature_led(gpio_num_t gpio_num)
{
    this->gpio_num = gpio_num;
    gpio_reset_pin(this->gpio_num);
    gpio_set_direction(this->gpio_num, GPIO_MODE_OUTPUT);
}

Temperature_led::~Temperature_led()
{
}

void Temperature_led::toggle()
{
    this->gpio_state = !this->gpio_state;
    gpio_set_level(this->gpio_num, this->gpio_state);
}

void Temperature_led::start_blink(uint32_t intervall)
{
    this->intervall = intervall;
    xTaskCreate(&blink_task, "BlinkTask", 2048, this, tskIDLE_PRIORITY, &blink_task_handle);
}

void Temperature_led::stop_blink()
{
    gpio_set_level(this->gpio_num, 0);
    vTaskDelete(blink_task_handle);
}

uint32_t Temperature_led::get_intervall()
{
    return this->intervall;
}
