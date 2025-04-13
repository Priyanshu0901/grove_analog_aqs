/**
 * @file grove_analog_aqs.h
 * @brief ESP-IDF component for Grove Analog Air Quality Sensor
 * @version 1.0.0
 * @date 2023-10-15
 * 
 * @copyright Copyright (c) 2023
 * 
 * MIT License
 */

#ifndef GROVE_ANALOG_AQS_H
#define GROVE_ANALOG_AQS_H

#include "esp_err.h"
#include "driver/adc.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Air quality levels
 */
typedef enum {
    GROVE_AQS_QUALITY_FRESH = 0,      /*!< Fresh air */
    GROVE_AQS_QUALITY_GOOD,           /*!< Good air quality */
    GROVE_AQS_QUALITY_MODERATE,       /*!< Moderate air quality */
    GROVE_AQS_QUALITY_POOR,           /*!< Poor air quality */
    GROVE_AQS_QUALITY_VERY_POOR       /*!< Very poor air quality */
} grove_aqs_quality_t;

/**
 * @brief Configuration for the Grove Analog Air Quality Sensor
 */
typedef struct {
    int adc_io_num;                  /*!< ADC IO pin number connected to the sensor */
    int adc_unit_num;                /*!< ADC unit number (0 for ADC_UNIT_1, 1 for ADC_UNIT_2) */
    adc1_channel_t adc_channel;      /*!< ADC channel connected to the sensor output */
    adc_atten_t adc_atten;           /*!< ADC attenuation for the input */
    int vref;                        /*!< Reference voltage in mV (typically 3300 for 3.3V) */
    
    /* Thresholds for air quality classification (in mV) */
    int fresh_threshold;              /*!< Threshold for fresh air (in mV) */
    int good_threshold;               /*!< Threshold for good air quality (in mV) */
    int moderate_threshold;           /*!< Threshold for moderate air quality (in mV) */
    int poor_threshold;               /*!< Threshold for poor air quality (in mV) */
    
    bool use_gpio_power;              /*!< Whether to use GPIO pin for powering the sensor */
    gpio_num_t power_gpio;            /*!< GPIO pin number for sensor power control (if used) */
} grove_aqs_config_t;

/**
 * @brief Data structure for the sensor readings
 */
typedef struct {
    int raw_value;                   /*!< Raw ADC reading */
    int voltage_mv;                  /*!< Converted voltage in mV */
    grove_aqs_quality_t quality;     /*!< Interpreted air quality level */
} grove_aqs_data_t;

/**
 * @brief Default configuration for the Grove Analog Air Quality Sensor
 */
#define GROVE_AQS_DEFAULT_CONFIG() { \
    .adc_io_num = CONFIG_GROVE_AQS_ADC_IO_NUM, \
    .adc_unit_num = CONFIG_GROVE_AQS_ADC_UNIT_NUM, \
    .adc_channel = ADC1_CHANNEL_6,   \
    .adc_atten = ADC_ATTEN_DB_11,    \
    .vref = 3300,                    \
    .fresh_threshold = 700,          \
    .good_threshold = 1000,          \
    .moderate_threshold = 1500,      \
    .poor_threshold = 2000,          \
    .use_gpio_power = false,         \
    .power_gpio = GPIO_NUM_NC        \
}

/**
 * @brief Initialize the Grove Analog Air Quality Sensor
 * 
 * @param config Configuration structure for the sensor
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t grove_aqs_init(const grove_aqs_config_t *config);

/**
 * @brief Deinitialize the Grove Analog Air Quality Sensor
 * 
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t grove_aqs_deinit(void);

/**
 * @brief Read data from the Grove Analog Air Quality Sensor
 * 
 * @param data Pointer to a data structure to store the sensor readings
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t grove_aqs_read_data(grove_aqs_data_t *data);

/**
 * @brief Power on the sensor (if GPIO power control is enabled)
 * 
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t grove_aqs_power_on(void);

/**
 * @brief Power off the sensor (if GPIO power control is enabled)
 * 
 * @return esp_err_t ESP_OK on success, otherwise an error code
 */
esp_err_t grove_aqs_power_off(void);

/**
 * @brief Get a string representation of the air quality level
 * 
 * @param quality Air quality level
 * @return const char* String representation
 */
const char* grove_aqs_quality_to_string(grove_aqs_quality_t quality);

#ifdef __cplusplus
}
#endif

#endif /* GROVE_ANALOG_AQS_H */ 