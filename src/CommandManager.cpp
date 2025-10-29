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
 * @brief Generate and send HOME_VALIDATION command
 */
bool CommandManager::generateHomeValidationCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande HOME_VALIDATION pour Bouee #%d", targetBuoyId);
    
    // Création de la commande
    Command validationCmd;
    validationCmd.targetBuoyId = targetBuoyId;
    validationCmd.type = CMD_HOME_VALIDATION;
    validationCmd.heading = 0;          // Non utilisé pour cette commande
    validationCmd.throttle = 0;         // Non utilisé pour cette commande
    validationCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = validationCmd;
    newCommandAvailable = true;
    
    Logger::log("   -> Type: CMD_HOME_VALIDATION");
    Logger::logf("   -> Target Buoy: #%d", targetBuoyId);
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, validationCmd);
    
    if (success) {
        Logger::log("   -> Commande HOME_VALIDATION envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande HOME_VALIDATION");
    }
    
    return success;
}

/**
 * @brief Generate and send NAV_CAP command
 */
bool CommandManager::generateNavCapCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande NAV_CAP pour Bouee #%d", targetBuoyId);
    
    // Création de la commande
    Command navCapCmd;
    navCapCmd.targetBuoyId = targetBuoyId;
    navCapCmd.type = CMD_NAV_CAP;
    navCapCmd.heading = 0;          // Non utilisé pour cette commande
    navCapCmd.throttle = 0;         // Non utilisé pour cette commande
    navCapCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = navCapCmd;
    newCommandAvailable = true;
    
    Logger::log("   -> Type: CMD_NAV_CAP");
    Logger::logf("   -> Target Buoy: #%d", targetBuoyId);
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, navCapCmd);
    
    if (success) {
        Logger::log("   -> Commande NAV_CAP envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande NAV_CAP");
    }
    
    return success;
}

/**
 * @brief Generate and send NAV_HOME command
 */
bool CommandManager::generateNavHomeCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande NAV_HOME pour Bouee #%d", targetBuoyId);
    
    // Création de la commande
    Command navHomeCmd;
    navHomeCmd.targetBuoyId = targetBuoyId;
    navHomeCmd.type = CMD_NAV_HOME;
    navHomeCmd.heading = 0;         // Non utilisé pour cette commande
    navHomeCmd.throttle = 0;        // Non utilisé pour cette commande
    navHomeCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = navHomeCmd;
    newCommandAvailable = true;
    
    Logger::log("   -> Type: CMD_NAV_HOME");
    Logger::logf("   -> Target Buoy: #%d", targetBuoyId);
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, navHomeCmd);
    
    if (success) {
        Logger::log("   -> Commande NAV_HOME envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande NAV_HOME");
    }
    
    return success;
}

/**
 * @brief Generate and send NAV_HOLD command
 */
bool CommandManager::generateNavHoldCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande NAV_HOLD pour Bouee #%d", targetBuoyId);
    
    // Création de la commande
    Command navHoldCmd;
    navHoldCmd.targetBuoyId = targetBuoyId;
    navHoldCmd.type = CMD_NAV_HOLD;
    navHoldCmd.heading = 0;         // Non utilisé pour cette commande
    navHoldCmd.throttle = 0;        // Non utilisé pour cette commande
    navHoldCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = navHoldCmd;
    newCommandAvailable = true;
    
    Logger::log("   -> Type: CMD_NAV_HOLD");
    Logger::logf("   -> Target Buoy: #%d", targetBuoyId);
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, navHoldCmd);
    
    if (success) {
        Logger::log("   -> Commande NAV_HOLD envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande NAV_HOLD");
    }
    
    return success;
}

/**
 * @brief Generate and send NAV_STOP command
 */
bool CommandManager::generateNavStopCommand(uint8_t targetBuoyId) {
    Logger::logf("\n[CommandManager] Generation commande NAV_STOP pour Bouee #%d", targetBuoyId);
    
    // Création de la commande
    Command navStopCmd;
    navStopCmd.targetBuoyId = targetBuoyId;
    navStopCmd.type = CMD_NAV_STOP;
    navStopCmd.heading = 0;         // Non utilisé pour cette commande
    navStopCmd.throttle = 0;        // Non utilisé pour cette commande
    navStopCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = navStopCmd;
    newCommandAvailable = true;
    
    Logger::log("   -> Type: CMD_NAV_STOP");
    Logger::logf("   -> Target Buoy: #%d", targetBuoyId);
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, navStopCmd);
    
    if (success) {
        Logger::log("   -> Commande NAV_STOP envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande NAV_STOP");
    }
    
    return success;
}

/**
 * @brief Generate and send SET_THROTTLE command with increment
 */
bool CommandManager::generateSetThrottleCommand(uint8_t targetBuoyId, int8_t currentThrottle, int8_t increment) {
    // Calcul du nouveau throttle
    int16_t newThrottle = currentThrottle + increment;
    
    // Limitation entre -100 et +100
    if (newThrottle > 100) newThrottle = 100;
    if (newThrottle < -100) newThrottle = -100;
    
    Logger::logf("\n[CommandManager] Generation commande SET_THROTTLE pour Bouee #%d", targetBuoyId);
    Logger::logf("   -> Throttle actuel: %d%%", currentThrottle);
    Logger::logf("   -> Increment: %+d%%", increment);
    Logger::logf("   -> Nouveau throttle: %d%%", newThrottle);
    
    // Création de la commande
    Command throttleCmd;
    throttleCmd.targetBuoyId = targetBuoyId;
    throttleCmd.type = CMD_SET_THROTTLE;
    throttleCmd.heading = 0;                    // Non utilisé
    throttleCmd.throttle = (int8_t)newThrottle;
    throttleCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = throttleCmd;
    newCommandAvailable = true;
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, throttleCmd);
    
    if (success) {
        Logger::log("   -> Commande SET_THROTTLE envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande SET_THROTTLE");
    }
    
    return success;
}

/**
 * @brief Generate and send SET_TRUE_HEADING command with increment
 */
bool CommandManager::generateSetHeadingCommand(uint8_t targetBuoyId, float currentHeading, int16_t increment) {
    // Calcul du nouveau cap
    float newHeading = currentHeading + increment;
    
    // Normalisation entre 0 et 360 degrés
    while (newHeading >= 360.0f) newHeading -= 360.0f;
    while (newHeading < 0.0f) newHeading += 360.0f;
    
    Logger::logf("\n[CommandManager] Generation commande SET_TRUE_HEADING pour Bouee #%d", targetBuoyId);
    Logger::logf("   -> Cap actuel: %.1f°", currentHeading);
    Logger::logf("   -> Increment: %+d°", increment);
    Logger::logf("   -> Nouveau cap: %.1f°", newHeading);
    
    // Création de la commande
    Command headingCmd;
    headingCmd.targetBuoyId = targetBuoyId;
    headingCmd.type = CMD_SET_TRUE_HEADING;
    headingCmd.heading = (int16_t)newHeading;   // Conversion en int16_t
    headingCmd.throttle = 0;                    // Non utilisé
    headingCmd.timestamp = millis();
    
    // Sauvegarde de la commande courante
    currentCommand = headingCmd;
    newCommandAvailable = true;
    
    // Envoi via ESP-NOW
    bool success = espNowComm.sendCommand(targetBuoyId, headingCmd);
    
    if (success) {
        Logger::log("   -> Commande SET_TRUE_HEADING envoyee avec succes");
    } else {
        Logger::log("   -> ERREUR: Echec envoi commande SET_TRUE_HEADING");
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
