#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"

#include "led_strip.h" // Libreria para controlar led RGB de la placa esp32-s3

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

led_strip_handle_t led_strip;

void inicializar_led(void) {

    // config general
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48, // 48 es el GPIO del led de la placa
        .max_leds = 1, // Cantidad de leds en la placa. Esto es porque la funcion se usa para tiras de led.
    };

    // resolucion del timer para controlar el led. Esto es importante para controlar la velocidad de parpadeo del led
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz resolution
        .flags.with_dma = false,
    };

    // inicializo el led
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    
    // limpio el estado
    led_strip_clear(led_strip);
}

void task_log(void *params){
    while(1){
        ESP_LOGI("main","Hello world!");
        vTaskDelay(50); // 50 ticks. la base de tiempo se configura en Ctrl+shift+P -> menuconfig
                        // Sino puedo usar la macro pdMS_TO_TIC}KS()
        }
}

void task_led(void *params){ // Buscar config para led de esp32-s3
    int led_state = 0;

    while (1) {
        if (led_state) {
            // Set the pixel to Green (Index 0, Red: 0, Green: 16, Blue: 0)
            // Values range from 0 to 255. Keeping it at 16 prevents it from being blindingly bright.
            led_strip_set_pixel(led_strip, 0, 0, 16, 0);
            
            // Push the data to the LED
            led_strip_refresh(led_strip);
        } else {
            // Turn the LED off
            led_strip_clear(led_strip);
        }
        led_state = !led_state;
        vTaskDelay(100);
}
}

void app_main(void)
{
    inicializar_led();

    xTaskCreate(task_log, "Log Task",2048, NULL, 1, NULL);
    xTaskCreate(task_led, "Led Task",2048, NULL, 1, NULL);

    
}