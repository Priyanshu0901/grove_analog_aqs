/**
 * @file app_main.c
 * @brief Example application for Grove Analog Air Quality Sensor component
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "grove_analog_aqs.h"

static const char *TAG = "grove_aqs_example";

void app_main(void)
{
    // Initialize with default configuration
    grove_aqs_config_t config = GROVE_AQS_DEFAULT_CONFIG();
    
    // Optionally customize configuration
    // config.adc_io_num = 4;             // Use GPIO4 for ADC
    // config.adc_unit_num = 1;           // Use ADC_UNIT_2
    // config.adc_channel = ADC1_CHANNEL_7;
    // config.fresh_threshold = 800;
    // config.use_gpio_power = true;
    // config.power_gpio = GPIO_NUM_5;
    
    ESP_LOGI(TAG, "Initializing Grove Analog Air Quality Sensor");
    ESP_LOGI(TAG, "Using ADC IO: %d, ADC Unit: %d, ADC Channel: %d", 
             config.adc_io_num, config.adc_unit_num, config.adc_channel);
             
    esp_err_t ret = grove_aqs_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor: %d", ret);
        return;
    }

    ESP_LOGI(TAG, "Waiting for sensor to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(3000)); // Give the sensor some time to stabilize

    // Read and display sensor data in a loop
    int readings_count = 0;
    const int MAX_READINGS = 30; // Take 30 readings and then stop
    
    while (readings_count < MAX_READINGS) {
        grove_aqs_data_t data;
        ret = grove_aqs_read_data(&data);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Reading #%d:", readings_count + 1);
            ESP_LOGI(TAG, "  Raw ADC value: %d", data.raw_value);
            ESP_LOGI(TAG, "  Voltage: %d mV", data.voltage_mv);
            ESP_LOGI(TAG, "  Air quality: %s", grove_aqs_quality_to_string(data.quality));
            
            // Print advice based on air quality
            switch (data.quality) {
                case GROVE_AQS_QUALITY_FRESH:
                    ESP_LOGI(TAG, "  Advice: Air is fresh and clean!");
                    break;
                case GROVE_AQS_QUALITY_GOOD:
                    ESP_LOGI(TAG, "  Advice: Air quality is good, no action needed.");
                    break;
                case GROVE_AQS_QUALITY_MODERATE:
                    ESP_LOGI(TAG, "  Advice: Consider ventilation to improve air quality.");
                    break;
                case GROVE_AQS_QUALITY_POOR:
                    ESP_LOGI(TAG, "  Advice: Poor air quality. Open windows or use air purifier.");
                    break;
                case GROVE_AQS_QUALITY_VERY_POOR:
                    ESP_LOGI(TAG, "  Advice: Very poor air quality! Immediate ventilation needed.");
                    break;
                default:
                    break;
            }
        } else {
            ESP_LOGE(TAG, "Failed to read sensor data: %d", ret);
        }
        
        readings_count++;
        vTaskDelay(pdMS_TO_TICKS(1000)); // Read once per second
    }
    
    // Clean up
    ESP_LOGI(TAG, "Done with readings, deinitializing sensor");
    grove_aqs_deinit();
    
    ESP_LOGI(TAG, "Example completed successfully");
} 