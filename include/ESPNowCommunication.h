/**
 * @file ESPNowCommunication.h
 * @brief ESP-NOW communication management with buoys
 * @author Philippe Hubert
 * @date 2025
 * 
 * This module handles bidirectional ESP-NOW communication between the joystick
 * and autonomous GPS buoys.
 */

#ifndef ESPNOW_COMMUNICATION_H
#define ESPNOW_COMMUNICATION_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "CommandManager.h"

// Maximum number of manageable buoys
#define MAX_BUOYS 6

/**
 * @brief Buoy state structure (received)
 */
struct BuoyState {
    uint8_t buoyId;                     ///< Buoy ID (0-5)
    uint32_t timestamp;                 ///< Message timestamp
    
    // General state
    tEtatsGeneral generalMode;          ///< General state (INIT, READY, MAINTENANCE, HOME_DEFINITION, NAV)
    tEtatsNav navigationMode;           ///< Current navigation mode
    
    // Sensor status
    bool gpsOk;                         ///< GPS sensor status
    bool headingOk;                     ///< Heading sensor status
    bool yawRateOk;                     ///< Yaw rate sensor status
    
    // Environmental data
    float temperature;                  ///< Temperature in °C
    
    // Battery data
    float remainingCapacity;            ///< Remaining battery capacity in mAh
    
    // Navigation data
    float distanceToCons;               ///< Distance to consigne/waypoint in meters
    
    // Autopilot commands
    int8_t autoPilotThrottleCmde;       ///< Autopilot throttle command (-100 to +100%)
    float autoPilotTrueHeadingCmde;     ///< Autopilot heading command in degrees
    int8_t autoPilotRudderCmde;         ///< Autopilot rudder command (-100 to +100%)
    
    // Forced commands
    int8_t forcedThrottleCmde;          ///< Forced throttle command (-100 to +100%)
    bool forcedThrottleCmdeOk;          ///< Forced throttle command active flag
    float forcedTrueHeadingCmde;        ///< Forced heading command in degrees
    bool forcedTrueHeadingCmdeOk;       ///< Forced heading command active flag
    int8_t forcedRudderCmde;            ///< Forced rudder command (-100 to +100%)
    bool forcedRudderCmdeOk;            ///< Forced rudder command active flag
};

/**
 * @brief Command packet structure (sent)
 */
struct CommandPacket {
    uint8_t targetBuoyId;       ///< Target buoy ID
    BuoyCommand command;        ///< Command type
    int16_t heading;            ///< Target heading
    int8_t throttle;            ///< Target speed
    uint32_t timestamp;         ///< Timestamp
};

/**
 * @brief Class to manage ESP-NOW communication
 */
class ESPNowCommunication {
public:
    /**
     * @brief Constructor
     */
    ESPNowCommunication();

    /**
     * @brief Initialize ESP-NOW
     * @return true if initialization succeeds
     */
    bool begin();

    /**
     * @brief Add a buoy to the peer list (DEPRECATED - now automatic)
     * @param buoyId Buoy ID (0-5)
     * @param macAddress Buoy MAC address
     * @return true if addition succeeds
     */
    bool addBuoy(uint8_t buoyId, const uint8_t* macAddress);

    /**
     * @brief Automatically add a buoy discovered via broadcast
     * @param macAddress Buoy MAC address
     * @param buoyId Buoy ID from broadcast message
     * @return true if addition succeeds
     */
    bool addBuoyDynamically(const uint8_t* macAddress, uint8_t buoyId);

    /**
     * @brief Send a command to a specific buoy
     * @param buoyId Target buoy ID
     * @param cmd Command structure to send
     * @return true if send succeeds
     */
    bool sendCommand(uint8_t buoyId, const Command& cmd);

    /**
     * @brief Get the state of the last buoy that sent data
     * @return BuoyState structure
     */
    BuoyState getLastBuoyState();

    /**
     * @brief Get the state of a specific buoy
     * @param buoyId Buoy ID
     * @return BuoyState structure
     */
    BuoyState getBuoyState(uint8_t buoyId);

    /**
     * @brief Check if new data has been received
     * @return true if new data is available
     */
    bool hasNewData();

    /**
     * @brief Reset the new data flag
     */
    void clearNewData();

    /**
     * @brief Get the number of registered buoys
     * @return Number of buoys
     */
    uint8_t getBuoyCount();

    /**
     * @brief Check if a buoy is connected (recent data)
     * @param buoyId Buoy ID
     * @param timeoutMs Timeout in milliseconds (default: 5000)
     * @return true if the buoy has sent data recently
     */
    bool isBuoyConnected(uint8_t buoyId, uint32_t timeoutMs = 5000);

    /**
     * @brief Remove inactive buoys (cleanup)
     * @param timeoutMs Timeout in milliseconds (default: 10000)
     * @return Number of buoys removed
     */
    uint8_t removeInactiveBuoys(uint32_t timeoutMs = 10000);

    /**
     * @brief Get local MAC address
     * @return Pointer to 6-byte array
     */
    const uint8_t* getLocalMacAddress();

private:
    struct BuoyPeer {
        uint8_t buoyId;
        uint8_t macAddress[6];
        bool registered;
        BuoyState lastState;
    };

    BuoyPeer buoys[MAX_BUOYS];
    uint8_t buoyCount;
    bool newDataAvailable;
    uint8_t localMac[6];
    
    static ESPNowCommunication* instance;  ///< Instance for callback

    /**
     * @brief Callback for received data (static)
     * @param mac Sender MAC address
     * @param data Received data buffer
     * @param len Data length
     */
    static void onDataRecv(const uint8_t* mac, const uint8_t* data, int len);

    /**
     * @brief Callback for data sent (static)
     * @param mac Receiver MAC address
     * @param status Send status
     */
    static void onDataSent(const uint8_t* mac, esp_now_send_status_t status);

    /**
     * @brief Process received data (instance method)
     * @param mac Sender MAC address
     * @param data Received data buffer
     * @param len Data length
     */
    void handleReceivedData(const uint8_t* mac, const uint8_t* data, int len);

    /**
     * @brief Find buoy index by ID
     * @param buoyId Buoy ID
     * @return Index in array or -1 if not found
     */
    int8_t findBuoyIndex(uint8_t buoyId);

    /**
     * @brief Find buoy index by MAC address
     * @param mac MAC address
     * @return Index in array or -1 if not found
     */
    int8_t findBuoyByMac(const uint8_t* mac);
};

#endif // ESPNOW_COMMUNICATION_H
