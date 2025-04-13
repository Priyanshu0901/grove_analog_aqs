# Grove Analog Air Quality Sensor Component for ESP-IDF

This is an ESP-IDF component for the Grove Analog Air Quality Sensor. It provides an easy-to-use API to read and interpret data from the sensor using the ESP32's ADC.

## Features

* Simple API to read air quality data
* Configurable ADC IO pin, ADC unit, channel and settings
* Voltage-to-quality level interpretation with configurable thresholds
* Optional GPIO control for sensor power management
* Proper error handling and reporting
* Support for ESP-IDF 4.4 and later

## Installation

### Using ESP Component Registry (Recommended)

```bash
cd your_project
idf.py add-dependency "grove_analog_aqs"
```

### Manual Installation

1. Create a components directory in your project if it doesn't exist:

```bash
mkdir -p components
```

2. Clone this repository into the components directory:

```bash
cd components
git clone https://github.com/Priyanshu0901/grove_analog_aqs.git
```

## Configuration

The component can be configured through menuconfig. Navigate to "Component config → Grove Analog Air Quality Sensor Configuration" to configure:

* ADC IO pin number and ADC unit number
* ADC channel and attenuation settings
* Reference voltage
* Air quality thresholds
* Power management options

```bash
idf.py menuconfig
```

## Usage

### Basic Usage

```c
#include "grove_analog_aqs.h"

void app_main(void)
{
    // Initialize with default configuration
    grove_aqs_config_t config = GROVE_AQS_DEFAULT_CONFIG();
    esp_err_t ret = grove_aqs_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor initialization failed: %d", ret);
        return;
    }
    
    // Read sensor data
    grove_aqs_data_t data;
    ret = grove_aqs_read_data(&data);
    if (ret == ESP_OK) {
        printf("Raw ADC value: %d\n", data.raw_value);
        printf("Voltage: %d mV\n", data.voltage_mv);
        printf("Air quality: %s\n", grove_aqs_quality_to_string(data.quality));
    }
    
    // Cleanup when done
    grove_aqs_deinit();
}
```

### Custom Configuration

```c
#include "grove_analog_aqs.h"

void app_main(void)
{
    // Custom configuration
    grove_aqs_config_t config = {
        .adc_io_num = 4,              // Use GPIO4 for ADC input
        .adc_unit_num = 0,            // Use ADC_UNIT_1 (0 for ADC_UNIT_1, 1 for ADC_UNIT_2)
        .adc_channel = ADC1_CHANNEL_7,
        .adc_atten = ADC_ATTEN_DB_11,
        .vref = 3300,
        .fresh_threshold = 800,
        .good_threshold = 1200,
        .moderate_threshold = 1800,
        .poor_threshold = 2400,
        .use_gpio_power = true,
        .power_gpio = GPIO_NUM_5
    };
    
    esp_err_t ret = grove_aqs_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor initialization failed: %d", ret);
        return;
    }
    
    // Power management (if configured)
    grove_aqs_power_off();
    vTaskDelay(pdMS_TO_TICKS(1000));
    grove_aqs_power_on();
    vTaskDelay(pdMS_TO_TICKS(2000)); // Warm-up time
    
    // Read sensor data
    grove_aqs_data_t data;
    ret = grove_aqs_read_data(&data);
    if (ret == ESP_OK) {
        printf("Air quality: %s\n", grove_aqs_quality_to_string(data.quality));
    }
    
    // Cleanup when done
    grove_aqs_deinit();
}
```

## API Reference

### Initialization and Deinitialization

```c
esp_err_t grove_aqs_init(const grove_aqs_config_t *config);
esp_err_t grove_aqs_deinit(void);
```

### Data Reading

```c
esp_err_t grove_aqs_read_data(grove_aqs_data_t *data);
```

### Power Management

```c
esp_err_t grove_aqs_power_on(void);
esp_err_t grove_aqs_power_off(void);
```

### Utility Functions

```c
const char* grove_aqs_quality_to_string(grove_aqs_quality_t quality);
```

## Data Structures

```c
typedef enum {
    GROVE_AQS_QUALITY_FRESH = 0,
    GROVE_AQS_QUALITY_GOOD,
    GROVE_AQS_QUALITY_MODERATE,
    GROVE_AQS_QUALITY_POOR,
    GROVE_AQS_QUALITY_VERY_POOR
} grove_aqs_quality_t;

typedef struct {
    int adc_io_num;                   // ADC IO pin number connected to the sensor
    int adc_unit_num;                 // ADC unit number (0 for ADC_UNIT_1, 1 for ADC_UNIT_2)
    adc1_channel_t adc_channel;
    adc_atten_t adc_atten;
    int vref;
    int fresh_threshold;
    int good_threshold;
    int moderate_threshold;
    int poor_threshold;
    bool use_gpio_power;
    gpio_num_t power_gpio;
} grove_aqs_config_t;

typedef struct {
    int raw_value;
    int voltage_mv;
    grove_aqs_quality_t quality;
} grove_aqs_data_t;
```

## License

This component is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request. 