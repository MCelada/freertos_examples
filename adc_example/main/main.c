#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

QueueHandle_t q_adc;

void task_print(void *params){
    int adc_data;
    while(1){
        xQueueReceive(q_adc, &adc_data, portMAX_DELAY);
        ESP_LOGI("PRINT", "Valor ADC: %d", adc_data);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_adc(void *params){
    adc_oneshot_unit_handle_t adc1_handle;

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE, //Procesador de muy bajo nivel de energia. Ahora desactivado
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle)); 

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));

    int raw_data;

    while(1){
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &raw_data);
        xQueueSendToBack(q_adc, &raw_data, portMAX_DELAY);

        ESP_LOGI("ADC", "Lectura Enviada: %d", raw_data);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void){
    q_adc = xQueueCreate(1, sizeof(int));

    xTaskCreate(task_adc, "Task ADC", 2048, NULL, 1, NULL);
    xTaskCreate(task_print, "Task Print", 2048, NULL, 1, NULL);
}