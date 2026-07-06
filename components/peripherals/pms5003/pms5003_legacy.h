#ifndef PMS5003_LEGACY_H
#define PMS5003_LEGACY_H

/*
 * Legacy/inactive PMS5003 driver header.
 *
 * This file exists for historical reference. The active PMS5003 component is:
 * components/pms5003
 *
 * Note: components/peripherals is not currently registered as an ESP-IDF
 * component root in this project, so this legacy code is not built.
 */

#include <stdint.h>
#include <stdbool.h>

// GPIO Configuration (legacy mapping)
#define PMS5003_RX_PIN 16
#define PMS5003_TX_PIN 17
#define PMS5003_SET_PIN 27   // Set pin for sleep mode control
#define PMS5003_UART_NUM UART_NUM_1

// Sensor data structure
typedef struct {
    uint16_t pm1_0;      // PM1.0 concentration (ug/m3)
    uint16_t pm2_5;      // PM2.5 concentration (ug/m3)
    uint16_t pm10;       // PM10 concentration (ug/m3)
    uint16_t pm1_0_atm;  // PM1.0 concentration under standard particle (ug/m3)
    uint16_t pm2_5_atm;  // PM2.5 concentration under standard particle (ug/m3)
    uint16_t pm10_atm;   // PM10 concentration under standard particle (ug/m3)
    uint16_t particles_0_3;   // Particles > 0.3um in 0.1L air
    uint16_t particles_0_5;   // Particles > 0.5um in 0.1L air
    uint16_t particles_1_0;   // Particles > 1.0um in 0.1L air
    uint16_t particles_2_5;   // Particles > 2.5um in 0.1L air
    uint16_t particles_5_0;   // Particles > 5.0um in 0.1L air
    uint16_t particles_10_0;  // Particles > 10.0um in 0.1L air
} pms5003_data_t;

/**
 * @brief Initialize PMS5003 sensor
 * Sets up UART2 and GPIO for SET pin
 */
void pms5003_init(void);

/**
 * @brief Read data from PMS5003 sensor
 * @param data Pointer to pms5003_data_t structure to store readings
 * @return true if data read successfully, false otherwise
 */
bool pms5003_read(pms5003_data_t *data);

/**
 * @brief Put sensor into sleep mode
 */
void pms5003_sleep(void);

/**
 * @brief Wake sensor from sleep mode
 */
void pms5003_wake(void);

/**
 * @brief Print sensor data to serial monitor
 * @param data Pointer to pms5003_data_t structure with sensor readings
 */
void pms5003_print_data(const pms5003_data_t *data);

#endif // PMS5003_LEGACY_H
