/**
 * @file CommandManager.cpp
 * @brief Command management for sending to buoys
 * @author Philippe Hubert
 * @date 2025
 */

#include "CommandManager.h"
#include "ESPNowCommunication.h"
#include "Logger.h"

/**
 * @brief Constructor
 * @param espNowComm Reference to ESP-NOW communication instance
 */
CommandManager::CommandManager(ESPNowCommunication& espNowComm)
    : espNowComm(espNowComm),
      newCommandAvailable(false),
      currentHeading(0),
      currentThrottle(0),
      currentMode(NAV_NOTHING),
      lastHeadingChange(0),
      lastModeChange(0) {
}

/**
 * @brief Update commands based on joystick inputs
 */
void CommandManager::update(int16_t leftX, int16_t leftY, int16_t rightX, int16_t rightY,
                             bool btnLeft, bool btnRight, bool btnLeftStick, bool btnRightStick) {
    // TODO: Implement full update logic
    // For now, this is a placeholder
}

/**
 * @brief Get the last generated command
 */
Command CommandManager::getCommand() {
    return currentCommand;
}

/**
 * @brief Check if a new command is available
 */
bool CommandManager::hasNewCommand() {
    return newCommandAvailable;
}

/**
 * @brief Reset the new command flag
 */
void CommandManager::clearNewCommand() {
    newCommandAvailable = false;
}

/**
 * @brief Get the current calculated heading
 */
int16_t CommandManager::getCurrentHeading() {
    return currentHeading;
}

/**
 * @brief Get the current throttle
 */
int8_t CommandManager::getCurrentThrottle() {
    return currentThrottle;
}

/**
 * @brief Get the current navigation mode
 */
tEtatsNav CommandManager::getCurrentMode() {
    return currentMode;
}

/**
 * @brief Generate and send INIT_HOME command to a buoy
 * @param targetBuoyId ID of the buoy to send the command to
 * @return true if command was sent successfully
 */
bool CommandManager::generateInitHomeCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande INIT_HOME pour Bouee #%d", targetBuoyId);
    
    // Création de la commande d'initialisation du HOME
    Command homeCmd;
    homeCmd.targetBuoyId = targetBuoyId;
    homeCmd.type = CMD_INIT_HOME;
    homeCmd.heading = 0;        // Non utilisé pour cette commande
    homeCmd.throttle = 0;       // Non utilisé pour cette commande
    homeCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = homeCmd;
    newCommandAvailable = true;
    
    // Envoi de la commande via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, homeCmd);
    
    if (success) {
        Logger::log("   -> Commande HOME envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande HOME");
    }
    
    return success;
}

/**
 * @brief Apply deadzone to joystick value
 */
int16_t CommandManager::applyDeadzone(int16_t value) {
    if (abs(value) < STICK_DEADZONE) {
        return 0;
    }
    return value;
}

/**
 * @brief Generate heading change command
 */
void CommandManager::updateHeading(int16_t rightX) {
    // TODO: Implement heading update logic
}

/**
 * @brief Generate throttle command
 */
void CommandManager::updateThrottle(int16_t leftY) {
    // TODO: Implement throttle update logic
}
