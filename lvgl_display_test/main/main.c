#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "../components/esp_lcd_ili9341/include/esp_lcd_ili9341.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "hal/spi_types.h"
#include "esp_lcd_io_spi.h"
#include "../components/lvgl/examples/widgets/image/lv_example_image_2.c"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "../components/lvgl/lvgl.h"

#define LCD_HOST    SPI2_HOST
#define PIN_NUM_CS 5
#define PIN_NUM_RESET 10
#define PIN_NUM_DC 11
#define PIN_NUM_MOSI 12
#define PIN_NUM_SCLK 13
#define PIN_NUM_LED 14
#define PIN_NUM_MISO 9

#define LCD_H_RES 320
#define LCD_V_RES 240
#define LCD_PIXEL_CLOCK_HZ 20 * 1000 * 1000 //20 MHz
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

// ADDED: Static LVGL display variables (Crucial to prevent crashes!)
static lv_display_t * display;

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t lcd_panel_handle = NULL;

// ADDED: DMA Transfer complete callback
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_display_t * disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) lv_display_get_user_data(disp);
    
    // LVGL v9 passes the color map as a uint8_t pointer, we can pass it directly to the ESP-IDF driver
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, (void *)px_map);
}

static void display_init(void)
{
    gpio_set_direction(PIN_NUM_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_LED, 1); // Turn on the backlight

    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 1. Create the LVGL Display Object FIRST
    display = lv_display_create(LCD_H_RES, LCD_V_RES);

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = display, // Pass the v9 display object here!
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RESET,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &lcd_panel_handle));

    esp_lcd_panel_reset(lcd_panel_handle);
    esp_lcd_panel_init(lcd_panel_handle);
    esp_lcd_panel_disp_on_off(lcd_panel_handle, true);

    // 2. Link the ESP-IDF panel handle to LVGL
    lv_display_set_user_data(display, lcd_panel_handle);

    // 3. Set the flush callback
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    // 4. Allocate memory and set the draw buffers
    size_t draw_buffer_sz = LCD_H_RES * 80 * sizeof(uint16_t);
    void *buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_DMA);
    assert(buf1);
    
    // In v9, setting buffers is a single function call
    lv_display_set_buffers(display, buf1, NULL, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void app_main(void)
{
    lv_init();
    display_init();
    esp_lcd_panel_swap_xy(lcd_panel_handle, true); // Swap X and Y axes for portrait mode
    esp_lcd_panel_draw_bitmap(lcd_panel_handle, 0, 0, 100, 50, NULL); // Clear the screen
    lv_example_image_2();
    
    while (1)
    {
        lv_tick_inc(10);
        lv_task_handler(); // (Note: you can also use lv_timer_handler() here)
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}