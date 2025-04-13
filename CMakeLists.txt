idf_component_register(
    SRCS "src/grove_analog_aqs.c"
    INCLUDE_DIRS "include"
    REQUIRES "driver" "esp_adc"
)