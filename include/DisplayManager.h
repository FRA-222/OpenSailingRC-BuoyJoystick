/**
 * @file DisplayManager.h
 * @brief LCD display management for the joystick
 * @author Philippe Hubert
 * @date 2025
 * 
 * This module manages the display of information on the M5AtomS3's 128x128 LCD:
 * - Selected buoy
 * - Navigation state
 * - Battery level
 * - Heading and speed
 */

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <M5Unified.h>
#include "BuoyStateManager.h"
#include "CommandManager.h"

/**
 * @brief Class to manage the display
 */
class DisplayManager {
public:
    /**
     * @brief Constructor
     * @param buoyManager Reference to buoy state manager
     */
    DisplayManager(BuoyStateManager& buoyManager);

    /**
     * @brief Initialize the display
     * @return true if initialization succeeds
     */
    bool begin();

    /**
     * @brief Update the display
     * 
     * Should be called regularly in the main loop.
     * Updates are throttled by UPDATE_INTERVAL.
     */
    void update();

    /**
     * @brief Display the main screen
     * 
     * Shows buoy status, navigation mode, heading, speed, battery, GPS, and signal.
     */
    void displayMainScreen();

    /**
     * @brief Display an error message
     * @param message Message to display
     */
    void displayError(const String& message);

    /**
     * @brief Display a connection message
     * @param message Message to display
     */
    void displayConnecting(const String& message);

    /**
     * @brief Display buoy selection screen
     * 
     * Shows "SELECTED: Buoy #X" message briefly when switching buoys.
     */
    void displayBuoySelection();

    /**
     * @brief Enable/disable the display
     * @param enabled true to enable
     */
    void setEnabled(bool enabled);

    /**
     * @brief Set screen brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);

private:
    BuoyStateManager& buoyMgr;
    bool displayEnabled;
    uint32_t lastUpdateTime;
    uint8_t currentBrightness;
    
    // Cache pour éviter le flickering
    struct DisplayCache {
        uint8_t buoyId = 255;
        bool connected = false;
        tEtatsGeneral generalMode = INIT;
        tEtatsNav navigationMode = NAV_STOP;
        bool gpsOk = false;
        bool headingOk = false;
        bool yawRateOk = false;
        float temperature = 0.0f;
        uint8_t batteryPercent = 0;
        float distanceToCons = 0.0f;
        float forcedTrueHeadingCmde = 0.0f;
        int8_t autoPilotThrottleCmde = 0;
        bool firstUpdate = true;
    } cache;
    
    static const uint32_t UPDATE_INTERVAL = 500;  ///< Update interval in ms
    static const uint8_t DEFAULT_BRIGHTNESS = 128;
    
    /**
     * @brief Draw header with buoy name
     * @param connected Connection status
     * 
     * Displays buoy name in green if connected, red if disconnected.
     */
    void drawHeader(bool connected);

    /**
     * @brief Draw sensor status LEDs
     * @param state Buoy state
     * 
     * Displays three LED indicators for GPS, MAG (heading), and YAW sensors.
     * Green = OK, Red = KO.
     */
    void drawSensorLEDs(const BuoyState& state);

    /**
     * @brief Draw temperature and battery percentage
     * @param state Buoy state
     * 
     * Displays temperature in Celsius and battery level as percentage.
     */
    void drawTempBattery(const BuoyState& state);

    /**
     * @brief Draw navigation state
     * @param state Buoy state
     * 
     * Shows general mode and navigation mode.
     */
    void drawNavigationState(const BuoyState& state);

    /**
     * @brief Draw distance to consigne and throttle
     * @param state Buoy state
     * 
     * Displays distance to waypoint and autopilot throttle command.
     */
    void drawDistanceThrottle(const BuoyState& state);

    /**
     * @brief Draw battery indicator
     * @param batteryLevel Battery level (0-100%)
     * @param x X position
     * @param y Y position
     */
    void drawBattery(uint8_t batteryLevel, int16_t x, int16_t y);

    /**
     * @brief Draw LTE signal indicator
     * @param signalQuality Signal quality (-1 to 31)
     * @param x X position
     * @param y Y position
     */
    void drawSignal(int8_t signalQuality, int16_t x, int16_t y);

    /**
     * @brief Draw GPS indicator
     * @param locked GPS locked status
     * @param x X position
     * @param y Y position
     */
    void drawGPS(bool locked, int16_t x, int16_t y);

    /**
     * @brief Display heading and speed
     * @param heading Heading in degrees
     * @param speed Speed in m/s
     */
    void drawHeadingSpeed(float heading, float speed);

    /**
     * @brief Get color based on battery level
     * @param batteryLevel Battery level (0-100%)
     * @return Color (RGB565)
     */
    uint16_t getBatteryColor(uint8_t batteryLevel);

    /**
     * @brief Get color based on navigation mode
     * @param mode Navigation mode
     * @return Color (RGB565)
     */
    uint16_t getNavModeColor(tEtatsNav mode);

    /**
     * @brief Get color based on general mode
     * @param mode General mode
     * @return Color (RGB565)
     */
    uint16_t getGeneralModeColor(tEtatsGeneral mode);
};

#endif // DISPLAY_MANAGER_H
