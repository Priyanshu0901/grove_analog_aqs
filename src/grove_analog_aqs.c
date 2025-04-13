/**
 * @file grove_analog_aqs.c
 * @brief Implementation of the Grove Analog Air Quality Sensor component
 * @version 1.0.0
 * @date 2023-10-15
 * 
 * @copyright Copyright (c) 2023
 * 
 * MIT License
 */

#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "grove_analog_aqs.h"

static const char *TAG = "grove_aqs";

typedef struct {
    grove_aqs_config_t config;
    bool initialized;
    adc_oneshot_unit_handle_t adc_handle;
    adc_cali_handle_t adc_cali_handle;
    bool do_calibration;
    adc_unit_t adc_unit;
} grove_aqs_dev_t;

static grove_aqs_dev_t sensor = {0};

esp_err_t grove_aqs_init(const grove_aqs_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    if (sensor.initialized) {
        ESP_LOGW(TAG, "Sensor already initialized, deinitializing first");
        grove_aqs_deinit();
    }

    // Store the configuration
    memcpy(&sensor.config, config, sizeof(grove_aqs_config_t));
    
    // Set the ADC unit based on the configuration
    sensor.adc_unit = sensor.config.adc_unit_num == 0 ? ADC_UNIT_1 : ADC_UNIT_2;
    
    // Log the configuration
    ESP_LOGI(TAG, "Initializing with ADC Unit: %d, ADC Channel: %d",
             sensor.config.adc_unit_num, sensor.config.adc_channel);
    
    // Initialize GPIO for power control if needed
    if (sensor.config.use_gpio_power && sensor.config.power_gpio != GPIO_NUM_NC) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << sensor.config.power_gpio),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO: %d", ret);
            return ret;
        }
        
        // Turn on the sensor by default
        ret = grove_aqs_power_on();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to power on sensor: %d", ret);
            return ret;
        }
    }

    // Initialize ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = sensor.adc_unit,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &sensor.adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC unit: %d", ret);
        return ret;
    }

    // Configure ADC channel
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = sensor.config.adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(sensor.adc_handle, sensor.config.adc_channel, &channel_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %d", ret);
        adc_oneshot_del_unit(sensor.adc_handle);
        return ret;
    }

    // Try to create ADC calibration handle
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = sensor.adc_unit,
        .atten = sensor.config.adc_atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &sensor.adc_cali_handle);
    if (ret == ESP_OK) {
        sensor.do_calibration = true;
        ESP_LOGI(TAG, "ADC calibration enabled");
    } else {
        sensor.do_calibration = false;
        ESP_LOGW(TAG, "ADC calibration disabled due to error: %d", ret);
    }

    sensor.initialized = true;
    ESP_LOGI(TAG, "Grove Analog Air Quality Sensor initialized successfully");
    return ESP_OK;
}

esp_err_t grove_aqs_deinit(void) {
    if (!sensor.initialized) {
        ESP_LOGW(TAG, "Sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Power off the sensor if we're using GPIO control
    if (sensor.config.use_gpio_power && sensor.config.power_gpio != GPIO_NUM_NC) {
        grove_aqs_power_off();
    }

    // Delete ADC calibration handle if it was created
    if (sensor.do_calibration) {
        esp_err_t ret = adc_cali_delete_scheme_curve_fitting(sensor.adc_cali_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to delete ADC calibration handle: %d", ret);
        }
        sensor.do_calibration = false;
    }

    // Delete ADC unit
    esp_err_t ret = adc_oneshot_del_unit(sensor.adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete ADC unit: %d", ret);
        return ret;
    }

    sensor.initialized = false;
    ESP_LOGI(TAG, "Grove Analog Air Quality Sensor deinitialized");
    return ESP_OK;
}

esp_err_t grove_aqs_read_data(grove_aqs_data_t *data) {
    if (!sensor.initialized) {
        ESP_LOGE(TAG, "Sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // Read raw ADC value
    esp_err_t ret = adc_oneshot_read(sensor.adc_handle, sensor.config.adc_channel, &data->raw_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC: %d", ret);
        return ret;
    }

    // Convert to voltage
    if (sensor.do_calibration) {
        ret = adc_cali_raw_to_voltage(sensor.adc_cali_handle, data->raw_value, &data->voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert ADC reading to voltage: %d", ret);
            return ret;
        }
    } else {
        // Simple linear approximation if calibration is not available
        data->voltage_mv = (data->raw_value * sensor.config.vref) / 4095;
    }

    // Determine air quality based on voltage and thresholds
    if (data->voltage_mv <= sensor.config.fresh_threshold) {
        data->quality = GROVE_AQS_QUALITY_FRESH;
    } else if (data->voltage_mv <= sensor.config.good_threshold) {
        data->quality = GROVE_AQS_QUALITY_GOOD;
    } else if (data->voltage_mv <= sensor.config.moderate_threshold) {
        data->quality = GROVE_AQS_QUALITY_MODERATE;
    } else if (data->voltage_mv <= sensor.config.poor_threshold) {
        data->quality = GROVE_AQS_QUALITY_POOR;
    } else {
        data->quality = GROVE_AQS_QUALITY_VERY_POOR;
    }

    ESP_LOGI(TAG, "Air quality reading: Raw=%d, Voltage=%dmV, Quality=%s", 
             data->raw_value, data->voltage_mv, grove_aqs_quality_to_string(data->quality));
    
    return ESP_OK;
}

esp_err_t grove_aqs_power_on(void) {
    if (!sensor.config.use_gpio_power || sensor.config.power_gpio == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "GPIO power control not enabled");
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret = gpio_set_level(sensor.config.power_gpio, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO high: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Sensor powered on");
    return ESP_OK;
}

esp_err_t grove_aqs_power_off(void) {
    if (!sensor.config.use_gpio_power || sensor.config.power_gpio == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "GPIO power control not enabled");
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret = gpio_set_level(sensor.config.power_gpio, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set GPIO low: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Sensor powered off");
    return ESP_OK;
}

const char* grove_aqs_quality_to_string(grove_aqs_quality_t quality) {
    switch (quality) {
        case GROVE_AQS_QUALITY_FRESH:
            return "Fresh";
        case GROVE_AQS_QUALITY_GOOD:
            return "Good";
        case GROVE_AQS_QUALITY_MODERATE:
            return "Moderate";
        case GROVE_AQS_QUALITY_POOR:
            return "Poor";
        case GROVE_AQS_QUALITY_VERY_POOR:
            return "Very Poor";
        default:
            return "Unknown";
    }
} 