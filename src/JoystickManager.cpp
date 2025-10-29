/**
 * @file JoystickManager.cpp
 * @brief Implémentation de la gestion des joysticks et boutons
 * @author Philippe Hubert
 * @date 2025
 */

#include "JoystickManager.h"
#include "Logger.h"
#include <M5Unified.h>

JoystickManager::JoystickManager() {
    for (int i = 0; i < 5; i++) {  // 5 boutons maintenant (incluant Atom screen)
        if (i < 4) {
            axisValues[i] = 2048;  // Valeur centrée par défaut
        }
        buttonState[i] = false;
        buttonPrevState[i] = false;
    }
    batteryVoltage[0] = 0.0f;
    batteryVoltage[1] = 0.0f;
    atomScreenPressTime = 0;
}

bool JoystickManager::begin() {
    // Initialise I2C avec les pins du AtomS3
    Wire1.begin(38, 39);  // SDA=38, SCL=39
    Wire1.setClock(400000);  // 400kHz
    
    delay(100);
    
    // Vérifie si le STM32 répond à l'adresse I2C
    Wire1.beginTransmission(I2C_ADDRESS);
    uint8_t error = Wire1.endTransmission();
    
    if (error == 0) {
        Logger::log("✓ JoystickManager: STM32 détecté sur I2C");
        return true;
    } else {
        Logger::logf("✗ JoystickManager: Erreur I2C %d", error);
        return false;
    }
}

void JoystickManager::update() {
    // Lit les valeurs des 4 axes de joystick
    axisValues[AXIS_LEFT_X] = readWord(LEFT_STICK_X_REG);
    axisValues[AXIS_LEFT_Y] = readWord(LEFT_STICK_Y_REG);
    axisValues[AXIS_RIGHT_X] = readWord(RIGHT_STICK_X_REG);
    axisValues[AXIS_RIGHT_Y] = readWord(RIGHT_STICK_Y_REG);
    
    // Lit l'état des boutons I2C
    for (int i = 0; i < 4; i++) {
        buttonPrevState[i] = buttonState[i];
    }
    
    buttonState[BTN_LEFT_STICK] = (readByte(LEFT_STICK_BTN_REG) & 0x01) == 0;
    buttonState[BTN_RIGHT_STICK] = (readByte(RIGHT_STICK_BTN_REG) & 0x01) == 0;
    buttonState[BTN_LEFT] = (readByte(LEFT_BTN_REG) & 0x01) == 0;
    buttonState[BTN_RIGHT] = (readByte(RIGHT_BTN_REG) & 0x01) == 0;
    
    // Met à jour le bouton de l'écran Atom S3
    M5.update();  // Met à jour l'état des boutons M5
    buttonPrevState[BTN_ATOM_SCREEN] = buttonState[BTN_ATOM_SCREEN];
    buttonState[BTN_ATOM_SCREEN] = M5.BtnA.isPressed();
    
    // Gère le timestamp de pression pour la détection de maintien
    if (buttonState[BTN_ATOM_SCREEN] && !buttonPrevState[BTN_ATOM_SCREEN]) {
        atomScreenPressTime = millis();
    } else if (!buttonState[BTN_ATOM_SCREEN]) {
        atomScreenPressTime = 0;
    }
    
    // Lit les tensions des batteries
    batteryVoltage[0] = getBattery1Voltage();
    batteryVoltage[1] = getBattery2Voltage();
}

uint16_t JoystickManager::getAxisValue(uint8_t axis) {
    if (axis < 4) {
        return axisValues[axis];
    }
    return 2048;  // Valeur centrée par défaut
}

int16_t JoystickManager::getAxisCentered(uint8_t axis) {
    if (axis < 4) {
        // Convertit 0-4095 en -2048 à +2047
        return (int16_t)axisValues[axis] - 2048;
    }
    return 0;
}

bool JoystickManager::isButtonPressed(uint8_t button) {
    if (button < 5) {  // Supporte maintenant 5 boutons
        return buttonState[button];
    }
    return false;
}

bool JoystickManager::wasButtonPressed(uint8_t button) {
    if (button < 5) {  // Supporte maintenant 5 boutons
        // Détecte un front montant (transition false → true)
        return buttonState[button] && !buttonPrevState[button];
    }
    return false;
}

bool JoystickManager::wasButtonReleased(uint8_t button) {
    if (button < 5) {
        // Détecte un front descendant (transition true → false)
        return !buttonState[button] && buttonPrevState[button];
    }
    return false;
}

bool JoystickManager::isAtomScreenPressed() {
    return buttonState[BTN_ATOM_SCREEN];
}

bool JoystickManager::wasAtomScreenPressed() {
    return buttonState[BTN_ATOM_SCREEN] && !buttonPrevState[BTN_ATOM_SCREEN];
}

bool JoystickManager::wasAtomScreenReleased() {
    return !buttonState[BTN_ATOM_SCREEN] && buttonPrevState[BTN_ATOM_SCREEN];
}

bool JoystickManager::isAtomScreenHeld(uint32_t durationMs) {
    if (!buttonState[BTN_ATOM_SCREEN] || atomScreenPressTime == 0) {
        return false;
    }
    return (millis() - atomScreenPressTime) >= durationMs;
}

float JoystickManager::getBattery1Voltage() {
    uint16_t rawValue = readWord(BATTERY1_VOLTAGE_REG);
    return (float)rawValue / 1000.0f;  // Conversion en volts
}

float JoystickManager::getBattery2Voltage() {
    uint16_t rawValue = readWord(BATTERY2_VOLTAGE_REG);
    return (float)rawValue / 1000.0f;  // Conversion en volts
}

uint16_t JoystickManager::readWord(uint8_t reg) {
    uint8_t buffer[2];
    
    Wire1.beginTransmission(I2C_ADDRESS);
    Wire1.write(reg);
    Wire1.endTransmission(false);
    
    Wire1.requestFrom(I2C_ADDRESS, 2);
    
    if (Wire1.available() >= 2) {
        buffer[0] = Wire1.read();  // LSB
        buffer[1] = Wire1.read();  // MSB
        return (uint16_t)(buffer[0] | (buffer[1] << 8));
    }
    
    return 2048;  // Valeur par défaut en cas d'erreur
}

uint8_t JoystickManager::readByte(uint8_t reg) {
    Wire1.beginTransmission(I2C_ADDRESS);
    Wire1.write(reg);
    Wire1.endTransmission(false);
    
    Wire1.requestFrom(I2C_ADDRESS, 1);
    
    if (Wire1.available()) {
        return Wire1.read();
    }
    
    return 0xFF;  // Valeur par défaut en cas d'erreur
}
