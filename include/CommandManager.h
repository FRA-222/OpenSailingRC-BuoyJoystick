/**
 * @file CommandManager.h
 * @brief Command management for sending to buoys
 * @author Philippe Hubert
 * @date 2025
 * 
 * This module translates joystick inputs into commands for autonomous buoys.
 */

#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>

// Forward declaration
class ESPNowCommunication;

/**
 * @brief Command types sent to buoys
 */
enum BuoyCommand {
    CMD_INIT_HOME = 0,      ///< Initialize Home with current GPS position
    CMD_SET_TRUE_HEADING,   ///< Set target true heading
    CMD_SET_THROTTLE,       ///< Set speed
    CMD_NAV_HOLD,           ///< Hold current position
    CMD_NAV_CAP,            ///< Navigate by heading (cap mode)
    CMD_NAV_HOME,           ///< Navigate to Home position
    CMD_NAV_STOP,           ///< Stop all movements
    CMD_HOME_VALIDATION     ///< Validate Home position
};

// Modes globaux de la bouee
enum tEtatsGeneral
{
    INIT = 0,
    READY,
    MAINTENANCE,
    HOME_DEFINITION, 
    NAV
}; 

// Modes de navigation de la bouee
enum tEtatsNav
{
    NAV_NOTHING = 0, 
    NAV_HOME,    
    NAV_HOLD,
    NAV_STOP,
    NAV_BASIC,
    NAV_CAP,
    NAV_TARGET
}; 


/**
 * @brief Complete command structure
 */
struct Command {
    uint8_t targetBuoyId;   ///< Target buoy ID
    BuoyCommand type;       ///< Command type
    int16_t heading;        ///< Target heading (-180 to +180 degrees)
    int8_t throttle;        ///< Speed (-100 to +100%)
    uint32_t timestamp;     ///< Command timestamp
};

/**
 * @brief Class to manage buoy commands
 */
class CommandManager {
public:
    /**
     * @brief Constructor
     * @param espNowComm Reference to ESP-NOW communication instance
     */
    CommandManager(ESPNowCommunication& espNowComm);

    /**
     * @brief Update commands based on joystick inputs
     * @param leftX Left X axis (-2048 to +2047)
     * @param leftY Left Y axis (-2048 to +2047)
     * @param rightX Right X axis (-2048 to +2047)
     * @param rightY Right Y axis (-2048 to +2047)
     * @param btnLeft Left button pressed
     * @param btnRight Right button pressed
     * @param btnLeftStick Left stick button pressed
     * @param btnRightStick Right stick button pressed
     */
    void update(int16_t leftX, int16_t leftY, int16_t rightX, int16_t rightY,
                bool btnLeft, bool btnRight, bool btnLeftStick, bool btnRightStick);

    /**
     * @brief Get the last generated command
     * @return Command structure
     */
    Command getCommand();

    /**
     * @brief Check if a new command is available
     * @return true if command is available
     */
    bool hasNewCommand();

    /**
     * @brief Reset the new command flag
     */
    void clearNewCommand();

    /**
     * @brief Get the current calculated heading
     * @return Heading in degrees (-180 to +180)
     */
    int16_t getCurrentHeading();

    /**
     * @brief Get the current throttle
     * @return Throttle (-100 to +100%)
     */
    int8_t getCurrentThrottle();

    /**
     * @brief Get the current navigation mode
     * @return Navigation mode
     */
    tEtatsNav getCurrentMode();

    /**
     * @brief Generate and send INIT_HOME command to a buoy
     * @param targetBuoyId ID of the buoy to send the command to
     * @return true if command was sent successfully
     */
    bool generateInitHomeCommand(uint8_t targetBuoyId);

private:
    ESPNowCommunication& espNowComm;  ///< Reference to ESP-NOW communication
    
    Command currentCommand;
    bool newCommandAvailable;
    
    int16_t currentHeading;      ///< Current heading
    int8_t currentThrottle;      ///< Current throttle
    tEtatsNav currentMode;  ///< Current mode
    
    uint32_t lastHeadingChange;  ///< Last heading change timestamp
    uint32_t lastModeChange;     ///< Last mode change timestamp
    
    static const int16_t HEADING_INCREMENT = 10;  ///< Heading increment in degrees
    static const uint32_t HEADING_DELAY = 200;    ///< Delay between heading changes (ms)
    static const int16_t STICK_DEADZONE = 300;    ///< Joystick deadzone
    
    /**
     * @brief Apply deadzone to joystick value
     * @param value Joystick value
     * @return Value with deadzone applied
     */
    int16_t applyDeadzone(int16_t value);

    /**
     * @brief Generate heading change command
     * @param rightX Right X axis of joystick
     */
    void updateHeading(int16_t rightX);

    /**
     * @brief Generate throttle command
     * @param leftY Left Y axis of joystick
     */
    void updateThrottle(int16_t leftY);
};

#endif // COMMAND_MANAGER_H
