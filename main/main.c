/*
 * Application entry point
 * This file intentionally contains only high-level startup orchestration.
 * The application is split into ESP-IDF components:
 * - system:
 *      Platform initialization (NVS, system-level setup)
 * - bacnet_app:
 *      BACnet stack initialization and runtime services:
 *      BACnet/IP, BACnet MS/TP, COV, objects, tasks, and networking
 * - sensors:
 *      Sensor-specific application logic.
 *      Sensors publish values into BACnet objects but remain independent
 *      from BACnet transport details.
 * - ui:
 *      Display/touch user interface.
 *      UI reads application state and BACnet values without owning logic.
 * main.c should remain hardware-independent.
 * New sensors, displays, or communication hardware should be added
 * through components, not by expanding this file.
 */

#include "esp_log.h"
#include "app_system.h"
#include "bacnet_app.h"
#include "sen54_app.h"
#include "ui_manager.h"
#include "User_Settings.h"

static const char *TAG = "main";

void app_main(void)
{
    /*
     * Initialize system services:
     * - NVS storage
     * - persistent configuration handling
     * - future board-level initialization
     */
    app_system_init();

    /*
     * Print active user configuration.
     * User_Settings remains the central product configuration layer.
     */
    User_Settings_Print();

    /*
     * Initialize BACnet core.
     * This prepares objects, services, transports, and protocol state.
     */
    bacnet_app_init();

    /*
     * Start enabled application modules.
     * Sensors run independently and publish data through BACnet objects.
     */
    if (USER_ENABLE_SEN54) {
        sen54_app_start();
    } else {
        ESP_LOGW(TAG, "SEN54 task disabled by USER_ENABLE_SEN54=false");
    }

    /*
     * Start display/touch UI layer.
     * UI implementation can vary by hardware without changing main.c.
     */
    ui_manager_start();

    /*
     * Start BACnet runtime tasks after all producers/consumers are ready.
     */
    bacnet_app_start();
}