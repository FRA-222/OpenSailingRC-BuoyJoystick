/**
 * @file DisplayManager.cpp
 * @brief Implémentation de la gestion de l'affichage LCD
 * @author Philippe Hubert
 * @date 2025
 */

#include "DisplayManager.h"
#include "Logger.h"

DisplayManager::DisplayManager(BuoyStateManager& buoyManager)
    : buoyMgr(buoyManager) {
    displayEnabled = true;
    lastUpdateTime = 0;
    currentBrightness = DEFAULT_BRIGHTNESS;
}

bool DisplayManager::begin() {
    M5.begin();
    M5.Display.setRotation(0);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setTextSize(1);
    
    // Affiche écran de démarrage
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("OpenSailingRC", 64, 40);
    M5.Display.drawString("Buoy Joystick", 64, 60);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString("v1.0", 64, 90);
    
    delay(2000);
    
    M5.Display.fillScreen(TFT_BLACK);
    
    Logger::log("✓ DisplayManager: Initialisé");
    return true;
}

void DisplayManager::update() {
    if (!displayEnabled) {
        return;
    }
    
    uint32_t currentTime = millis();
    
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        displayMainScreen();
    }
}

void DisplayManager::displayMainScreen() {
    M5.Display.fillScreen(TFT_BLACK);
    
    BuoyState state = buoyMgr.getSelectedBuoyState();
    uint8_t buoyId = buoyMgr.getSelectedBuoyId();
    bool connected = buoyMgr.isSelectedBuoyConnected();
    
    // Header avec nom de la bouée (couleur selon connexion)
    drawHeader(connected);
    
    if (connected) {
        // Indicateurs LED des capteurs (ligne 2)
        drawSensorLEDs(state);
        
        // Température et Batterie (ligne 3)
        drawTempBattery(state);
        
        // Modes général et navigation (ligne 4-5)
        drawNavigationState(state);
        
        // Distance to consigne et Throttle (ligne 6)
        drawDistanceThrottle(state);
    } else {
        M5.Display.setTextDatum(MC_DATUM);
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.setFont(&fonts::Font2);
        M5.Display.drawString("Waiting...", 64, 64);
    }
}

void DisplayManager::drawHeader(bool connected) {
    uint8_t buoyId = buoyMgr.getSelectedBuoyId();
    String buoyName = buoyMgr.getBuoyName(buoyId);
    
    M5.Display.setTextDatum(TC_DATUM);
    // Couleur verte si connecté, rouge si non connecté
    if (connected) {
        M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    } else {
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    }
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString(buoyName, 64, 5);
}

void DisplayManager::drawSensorLEDs(const BuoyState& state) {
    // Ligne 2 : Indicateurs LED des capteurs
    const int16_t y = 28;  // Position Y sous le header (ajusté de 22 à 28)
    const int16_t ledRadius = 4;  // Rayon de la LED (augmenté de 3 à 4)
    const int16_t spacing = 42;   // Espacement entre les LEDs
    
    // Centre de l'écran (128 pixels / 2 = 64)
    // 3 LEDs espacées : GPS, MAG, YAW
    const int16_t startX = 64 - spacing;  // Position de la première LED
    
    M5.Display.setFont(&fonts::Font0);  // Petite police pour les labels
    M5.Display.setTextDatum(TC_DATUM);
    
    // GPS LED
    int16_t gpsX = startX;
    M5.Display.fillCircle(gpsX, y, ledRadius, state.gpsOk ? TFT_GREEN : TFT_RED);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("GPS", gpsX, y + 7);
    
    // MAG (Heading) LED
    int16_t magX = startX + spacing;
    M5.Display.fillCircle(magX, y, ledRadius, state.headingOk ? TFT_GREEN : TFT_RED);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("MAG", magX, y + 7);
    
    // YAW (YawRate) LED
    int16_t yawX = startX + spacing * 2;
    M5.Display.fillCircle(yawX, y, ledRadius, state.yawRateOk ? TFT_GREEN : TFT_RED);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString("YAW", yawX, y + 7);
}

void DisplayManager::drawTempBattery(const BuoyState& state) {
    // Ligne 3 : Température et % Batterie
    const int16_t y = 50;
    
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    
    // Température à gauche
    char tempBuffer[16];
    snprintf(tempBuffer, sizeof(tempBuffer), "%.1fC", state.temperature);
    M5.Display.drawString(tempBuffer, 5, y);
    
    // Batterie à droite (conversion de mAh en %)
    uint8_t batteryPercent = (uint8_t)((state.remainingCapacity / 10000.0) * 100);
    if (batteryPercent > 100) batteryPercent = 100;
    
    char battBuffer[16];
    snprintf(battBuffer, sizeof(battBuffer), "Batt:%d%%", batteryPercent);
    M5.Display.setTextDatum(TR_DATUM);
    M5.Display.drawString(battBuffer, 123, y);
}

void DisplayManager::drawDistanceThrottle(const BuoyState& state) {
    // Ligne 6 : Distance to consigne et Throttle
    const int16_t y = 100;
    
    M5.Display.setFont(&fonts::Font0);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    
    // Distance à gauche
    char distBuffer[16];
    M5.Display.setTextDatum(TL_DATUM);
    if (state.distanceToCons < 1000) {
        snprintf(distBuffer, sizeof(distBuffer), "Dst:%.0fm", state.distanceToCons);
    } else {
        snprintf(distBuffer, sizeof(distBuffer), "Dst:%.1fkm", state.distanceToCons / 1000.0);
    }
    M5.Display.drawString(distBuffer, 5, y);
    
    // Throttle à droite
    char throttleBuffer[16];
    snprintf(throttleBuffer, sizeof(throttleBuffer), "Thr:%d%%", state.autoPilotThrottleCmde);
    M5.Display.setTextDatum(TR_DATUM);
    M5.Display.drawString(throttleBuffer, 123, y);
}

void DisplayManager::drawNavigationState(const BuoyState& state) {
    // Mode général (ligne 4)
    String generalModeName = buoyMgr.getGeneralModeName(state.generalMode);
    uint16_t generalColor = getGeneralModeColor(state.generalMode);
    
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(generalColor, TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString(generalModeName, 64, 68);  // Ajusté pour ligne 4
    
    // Mode de navigation (ligne 5, plus petit)
    String navModeName = buoyMgr.getNavModeName(state.navigationMode);
    uint16_t navModeColor = getNavModeColor(state.navigationMode);
    
    M5.Display.setTextColor(navModeColor, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString("Nav: " + navModeName, 64, 85);  // Ajusté pour ligne 5
}

void DisplayManager::drawHeadingSpeed(float heading, float speed) {
    char buffer[32];
    
    // Cap
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    sprintf(buffer, "Hdg: %03d", (int)heading);
    M5.Display.drawString(buffer, 64, 65);
    
    // Vitesse
    sprintf(buffer, "%.1f m/s", speed);
    M5.Display.drawString(buffer, 64, 83);
}

void DisplayManager::drawBattery(uint8_t batteryLevel, int16_t x, int16_t y) {
    uint16_t color = getBatteryColor(batteryLevel);
    
    // Dessine icône de batterie
    M5.Display.drawRect(x, y, 30, 15, TFT_WHITE);
    M5.Display.fillRect(x + 30, y + 5, 3, 5, TFT_WHITE);
    
    // Remplit selon le niveau
    int fillWidth = (batteryLevel * 26) / 100;
    if (fillWidth > 26) fillWidth = 26;
    M5.Display.fillRect(x + 2, y + 2, fillWidth, 11, color);
    
    // Affiche pourcentage
    M5.Display.setTextDatum(TL_DATUM);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    char buffer[8];
    sprintf(buffer, "%d%%", batteryLevel);
    M5.Display.drawString(buffer, x, y + 16);
}

void DisplayManager::drawGPS(bool locked, int16_t x, int16_t y) {
    uint16_t color = locked ? TFT_GREEN : TFT_RED;
    
    // Dessine icône GPS
    M5.Display.fillCircle(x + 5, y + 5, 5, color);
    M5.Display.drawCircle(x + 5, y + 5, 8, color);
    M5.Display.drawCircle(x + 5, y + 5, 11, color);
    
    // Texte
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.setTextColor(color, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString(locked ? "GPS" : "NO", x + 5, y + 16);
}

void DisplayManager::drawSignal(int8_t signalQuality, int16_t x, int16_t y) {
    if (signalQuality < 0) {
        M5.Display.setTextDatum(TL_DATUM);
        M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
        M5.Display.setFont(&fonts::Font0);
        M5.Display.drawString("--", x, y);
        return;
    }
    
    // Calcule le nombre de barres (0-3)
    uint8_t bars = 0;
    if (signalQuality > 20) bars = 3;
    else if (signalQuality > 10) bars = 2;
    else if (signalQuality > 5) bars = 1;
    
    uint16_t color = (bars >= 2) ? TFT_GREEN : (bars == 1) ? TFT_YELLOW : TFT_RED;
    
    // Dessine les barres
    for (int i = 0; i < 3; i++) {
        int height = (i + 1) * 4;
        uint16_t barColor = (i < bars) ? color : TFT_DARKGREY;
        M5.Display.fillRect(x + i * 5, y + 12 - height, 3, height, barColor);
    }
    
    // Texte
    M5.Display.setTextDatum(TC_DATUM);
    M5.Display.setTextColor(color, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString("LTE", x + 7, y + 16);
}

void DisplayManager::displayError(const String& message) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("ERROR", 64, 40);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString(message, 64, 70);
}

void DisplayManager::displayConnecting(const String& message) {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("CONNECTING", 64, 40);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.setFont(&fonts::Font0);
    M5.Display.drawString(message, 64, 70);
}

void DisplayManager::displayBuoySelection() {
    uint8_t buoyId = buoyMgr.getSelectedBuoyId();
    String buoyName = buoyMgr.getBuoyName(buoyId);
    
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextDatum(MC_DATUM);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.drawString("SELECTED", 64, 40);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(buoyName, 64, 70);
    
    delay(500);  // Affiche pendant 500ms
}

void DisplayManager::setEnabled(bool enabled) {
    displayEnabled = enabled;
    if (!enabled) {
        M5.Display.fillScreen(TFT_BLACK);
    }
}

void DisplayManager::setBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    M5.Display.setBrightness(brightness);
}

uint16_t DisplayManager::getBatteryColor(uint8_t batteryLevel) {
    if (batteryLevel > 50) {
        return TFT_GREEN;
    } else if (batteryLevel > 20) {
        return TFT_ORANGE;
    } else {
        return TFT_RED;
    }
}

uint16_t DisplayManager::getNavModeColor(tEtatsNav mode) {
    switch (mode) {
        case NAV_CAP:
            return TFT_GREEN;
        case NAV_TARGET:
            return TFT_CYAN;
        case NAV_HOLD:
            return TFT_YELLOW;
        case NAV_HOME:
            return TFT_BLUE;
        case NAV_STOP:
            return TFT_RED;
        case NAV_BASIC:
            return TFT_ORANGE;
        case NAV_NOTHING:
        default:
            return TFT_WHITE;
    }
}

uint16_t DisplayManager::getGeneralModeColor(tEtatsGeneral mode) {
    switch (mode) {
        case INIT:
            return TFT_YELLOW;
        case READY:
            return TFT_CYAN;
        case MAINTENANCE:
            return TFT_ORANGE;
        case HOME_DEFINITION:
            return TFT_MAGENTA;
        case NAV:
            return TFT_GREEN;
        default:
            return TFT_WHITE;
    }
}
