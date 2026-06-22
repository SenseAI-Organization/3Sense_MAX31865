/*******************************************************************************
 * @file HDC2080_with_UART_receiver.cpp
 * @brief HDC2080 sensor with UART receiver for MAX31865 data
 *
 * This example demonstrates:
 * - Reading HDC2080 temperature and humidity sensor
 * - Receiving MAX31865 PT100 temperature data via UART
 * - Displaying both sensor readings together
 *
 * @version v0.1.0
 * @date 2026-06-18
 * @author Sense AI
 *******************************************************************************
 *******************************************************************************/

#include "HDC2080.hpp"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

/* I2C Port Configuration */
constexpr gpio_num_t kSDA = GPIO_NUM_5;
constexpr gpio_num_t kSCL = GPIO_NUM_4;
I2C i2c(I2C_NUM_1, kSDA, kSCL, 400000, false);

/* UART Configuration for receiving MAX31865 data */
#define UART_PORT_NUM      UART_NUM_1
#define UART_RX_PIN        GPIO_NUM_16  // Connect to TX of MAX31865 ESP32
#define UART_TX_PIN        GPIO_NUM_17  // Connect to RX of MAX31865 ESP32
#define UART_BAUD_RATE     9600
#define UART_BUF_SIZE      1024

/* Structure to hold received MAX31865 data */
struct MAX31865Data {
    float temperature;
    uint16_t rtd;
    float resistance;
    uint8_t fault;
    bool valid;
};

MAX31865Data max31865Data = {0.0f, 0, 0.0f, 0, false};

/**
 * @brief Parse incoming UART data from MAX31865
 * Format: TEMP:25.50,RTD:16384,RES:110.25,FAULT:0
 */
void parseMAX31865Data(const char* data, MAX31865Data* result) {
    char temp_str[20], rtd_str[20], res_str[20], fault_str[20];
    
    // Parse the comma-separated values
    if (sscanf(data, "TEMP:%[^,],RTD:%[^,],RES:%[^,],FAULT:%s", 
               temp_str, rtd_str, res_str, fault_str) == 4) {
        result->temperature = atof(temp_str);
        result->rtd = (uint16_t)atoi(rtd_str);
        result->resistance = atof(res_str);
        result->fault = (uint8_t)atoi(fault_str);
        result->valid = true;
    } else {
        result->valid = false;
    }
}

/**
 * @brief UART receiver task
 */
void uart_receiver_task(void* arg) {
    uint8_t* data = (uint8_t*)malloc(UART_BUF_SIZE);
    
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, 
                                  pdMS_TO_TICKS(100));
        
        if (len > 0) {
            data[len] = '\0';  // Null terminate
            
            // Look for complete line ending with \n
            char* line_end = strchr((char*)data, '\n');
            if (line_end) {
                *line_end = '\0';  // Terminate at newline
                parseMAX31865Data((char*)data, &max31865Data);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    free(data);
}

/**
 * @brief Initialize UART for receiving data
 */
esp_err_t init_uart() {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    
    // Set UART pins
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, 
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 
                                        0, 0, NULL, 0));
    
    return ESP_OK;
}

extern "C" void app_main() {
    printf("\n\n<< HDC2080 + MAX31865 UART Receiver Example >>\n\n");

    // Initialize UART first
    esp_err_t err = init_uart();
    if (err != ESP_OK) {
        printf("Error initializing UART: %s\n", esp_err_to_name(err));
        while (1) {
            vTaskDelay(portMAX_DELAY);
        }
    }
    printf("UART initialized successfully (RX: GPIO%d, TX: GPIO%d, Baud: %d)\n", 
           UART_RX_PIN, UART_TX_PIN, UART_BAUD_RATE);

    // Create HDC2080 instance
    HDC2080 sensor(i2c, HDC2080::kAddressGnd);

    // Initialize I2C
    err = i2c.init();
    if (err != ESP_OK) {
        printf("Error initializing I2C: %s\n", esp_err_to_name(err));
        while (1) {
            vTaskDelay(portMAX_DELAY);
        }
    }
    printf("I2C initialized successfully\n");

    // Initialize HDC2080 sensor
    err = sensor.init();
    if (err != ESP_OK) {
        printf("Error initializing HDC2080: %s\n", esp_err_to_name(err));
        printf("Please check sensor connections\n");
        while (1) {
            vTaskDelay(portMAX_DELAY);
        }
    }
    printf("HDC2080 initialized successfully\n");

    // Read device information
    uint16_t manufacturerId = 0;
    uint16_t deviceId = 0;
    sensor.getManufacturerId(&manufacturerId);
    sensor.getDeviceId(&deviceId);
    printf("Manufacturer ID: 0x%04X\n", manufacturerId);
    printf("Device ID: 0x%04X\n\n", deviceId);

    // Configure sensor for 14-bit resolution
    sensor.setResolution(HDC2080::Resolution::k14bit, HDC2080::Resolution::k14bit);
    sensor.setMeasurementMode(HDC2080::MeasurementMode::kHumidityTemperature);

    // Start UART receiver task
    xTaskCreate(uart_receiver_task, "uart_rx", 4096, NULL, 10, NULL);

    printf("Starting measurements...\n");
    printf("======================================================================\n");
    printf("  HDC2080 (Local)         |  MAX31865 (UART Received)\n");
    printf("  Temp(°C)  | Humidity(%%) |  PT100(°C) | RTD    | Res(Ω) | Fault\n");
    printf("======================================================================\n");

    // Main measurement loop
    while (true) {
        // Measure HDC2080
        err = sensor.measure();
        if (err == ESP_OK) {
            float hdc_temp = sensor.getTemperature();
            float hdc_humidity = sensor.getHumidity();

            // Display combined data
            printf("  %8.2f  |  %9.2f  |", hdc_temp, hdc_humidity);
            
            if (max31865Data.valid) {
                printf("  %8.2f  | %6u | %6.2f |  0x%02X\n",
                       max31865Data.temperature,
                       max31865Data.rtd,
                       max31865Data.resistance,
                       max31865Data.fault);
            } else {
                printf("  No Data\n");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
