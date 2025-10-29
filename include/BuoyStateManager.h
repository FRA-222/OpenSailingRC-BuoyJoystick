/**
 * @file BuoyStateManager.h
 * @brief State management for all buoys and active buoy selection
 * @author Philippe Hubert
 * @date 2025
 * 
 * This module manages the state of all connected buoys and the selection
 * of the currently controlled buoy.
 */

#ifndef BUOY_STATE_MANAGER_H
#define BUOY_STATE_MANAGER_H

#include <Arduino.h>
#include "ESPNowCommunication.h"

/**
 * @brief Class to manage buoy states
 */
class BuoyStateManager {
public:
    /**
     * @brief Constructor
     * @param espNow Reference to ESPNowCommunication instance
     */
    BuoyStateManager(ESPNowCommunication& espNow);

    /**
     * @brief Initialize the manager
     */
    void begin();

    /**
     * @brief Update the state of all buoys
     * 
     * Should be called regularly in the main loop to check for new data
     * from buoys and update connection status.
     */
    void update();

    /**
     * @brief Select the next buoy
     * 
     * Cycles to the next buoy in the list (wraps around to first buoy)
     */
    void selectNextBuoy();

    /**
     * @brief Select the previous buoy
     * 
     * Cycles to the previous buoy in the list (wraps around to last buoy)
     */
    void selectPreviousBuoy();

    /**
     * @brief Select a specific buoy
     * @param buoyId Buoy ID to select (0-5)
     */
    void selectBuoy(uint8_t buoyId);

    /**
     * @brief Get the ID of the currently selected buoy
     * @return Buoy ID (0-5)
     */
    uint8_t getSelectedBuoyId();

    /**
     * @brief Get the state of the selected buoy
     * @return BuoyState structure with current data
     */
    BuoyState getSelectedBuoyState();

    /**
     * @brief Get the number of connected buoys
     * @return Number of buoys with recent data
     */
    uint8_t getConnectedBuoyCount();

    /**
     * @brief Check if the selected buoy is connected
     * @return true if connected (received data recently)
     */
    bool isSelectedBuoyConnected();

    /**
     * @brief Get the name of a buoy
     * @param buoyId Buoy ID (0-5)
     * @return Buoy name (e.g., "Buoy #1")
     */
    String getBuoyName(uint8_t buoyId);

    /**
     * @brief Get the name of a navigation mode
     * @param mode Navigation mode
     * @return Mode name (e.g., "HDG", "HOLD", etc.)
     */
    String getNavModeName(tEtatsNav mode);

    /**
     * @brief Get the name of a general mode
     * @param mode General mode
     * @return Mode name (e.g., "INIT", "READY", "NAV", etc.)
     */
    String getGeneralModeName(tEtatsGeneral mode);

private:
    ESPNowCommunication& espNowComm;
    uint8_t selectedBuoyId;
    uint32_t lastUpdateTime;
    
    static const uint32_t UPDATE_INTERVAL = 100;  ///< Update interval in ms
};

#endif // BUOY_STATE_MANAGER_H
