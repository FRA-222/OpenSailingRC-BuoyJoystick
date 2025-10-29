/**
 * @file main.cpp
 * @brief Programme principal du joystick de contrôle de bouées
 * @author Philippe Hubert
 * @date 2025
 * 
 * Ossature avec communication ESP-NOW bidirectionnelle et affichage LCD
 */

#include <Arduino.h>
#include <M5Unified.h>
#include "Logger.h"
#include "JoystickManager.h"
#include "ESPNowCommunication.h"
#include "BuoyStateManager.h"
#include "DisplayManager.h"
#include "CommandManager.h"

// ============================================================================
// CONFIGURATION - DÉCOUVERTE AUTOMATIQUE DES BOUÉES
// ============================================================================
// Les bouées sont maintenant découvertes automatiquement via leurs broadcasts ESP-NOW.
// Plus besoin de configurer manuellement les adresses MAC !

// ============================================================================
// INSTANCES DES MANAGERS
// ============================================================================
JoystickManager joystick;
ESPNowCommunication espNow;
BuoyStateManager buoyState(espNow);
DisplayManager display(buoyState);
CommandManager cmdManager(espNow);

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
uint32_t lastLoopTime = 0;
const uint32_t LOOP_INTERVAL = 100;  // 10Hz

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // Test très précoce du Serial
    USBSerial.begin(115200);
    delay(2000);  // Délai important pour l'USB-JTAG
    
    // M5Stack Atom S3 initialization
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // Initialisation du Logger (sortie série uniquement par défaut)
    Logger::init(true, false);  // Serial activé, LCD désactivé
    
    Logger::log();
    Logger::log("*** DEMARRAGE ***");
    Logger::log("*** TEST SERIAL ***");
    Logger::log();
    Logger::log("===========================================");
    Logger::log("  OpenSailingRC - Buoy Joystick v1.0");
    Logger::log("===========================================");
    Logger::log();
    
    // 1. Initialisation du joystick (I2C)
    Logger::log();
    Logger::log("1. Initialisation Joystick...");
    if (!joystick.begin()) {
        Logger::log("   -> ERREUR: Echec initialisation joystick");
        Logger::log("   -> Verifiez la connexion I2C (SDA=38, SCL=39)");
        // On continue quand même pour tester ESP-NOW
    } else {
        Logger::log("   -> Joystick: OK");
    }
    
    // 2. Initialisation ESP-NOW
    Logger::log();
    Logger::log("2. Initialisation ESP-NOW...");
    if (!espNow.begin()) {
        Logger::log("   -> ERREUR CRITIQUE: Echec initialisation ESP-NOW");
        while (1) {
            delay(1000);
        }
    } else {
        Logger::log("   -> ESP-NOW: OK");
    }
    
    // 3. Préparation pour découverte automatique des bouées
    Logger::log();
    Logger::log("3. Attente découverte automatique des bouées...");
    Logger::log("   -> Les bouées seront ajoutées automatiquement");
    Logger::log("   -> lors de la réception de leurs broadcasts");
    
    // 4. Initialisation du gestionnaire d'état des bouées
    Logger::log();
    Logger::log("4. Initialisation BuoyStateManager...");
    buoyState.begin();
    
    // 5. Initialisation de l'affichage
    Logger::log();
    Logger::log("5. Initialisation Display...");
    if (!display.begin()) {
        Logger::log("   -> ERREUR: Echec initialisation display");
    } else {
        Logger::log("   -> Display: OK");
    }
    
    Logger::log();
    Logger::log("===========================================");
    Logger::log("  SYSTEM READY - Waiting for buoys");
    Logger::log("===========================================");
    Logger::log();
    
    // Affiche écran de connexion
    display.displayConnecting("Waiting for buoys...");
    
    delay(2000);
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    uint32_t currentTime = millis();
    
    // Maintient une fréquence de loop stable (10Hz)
    if (currentTime - lastLoopTime < LOOP_INTERVAL) {
        delay(1);
        return;
    }
    lastLoopTime = currentTime;
    
    // ========================================================================
    // 1. LECTURE DES JOYSTICKS ET BOUTONS
    // ========================================================================
    joystick.update();
    
    // Lecture des boutons jaunes L/R en haut
    // NOTE: Les vrais boutons jaunes correspondent aux BTN_LEFT_STICK et BTN_RIGHT_STICK
    
    // Bouton jaune GAUCHE : Initialisation du HOME
    if (joystick.wasButtonPressed(BTN_LEFT_STICK)) {
        uint8_t selectedId = buoyState.getSelectedBuoyId();
        if (buoyState.isSelectedBuoyConnected()) {
            Logger::logf("\n[L] Bouton jaune GAUCHE presse - Envoi CMD_INIT_HOME a Bouee #%d", selectedId);
            
            // Utilisation de CommandManager pour créer et envoyer la commande
            if (cmdManager.generateInitHomeCommand(selectedId)) {
                display.displayBuoySelection(); // Rafraîchit l'affichage
            }
        } else {
            Logger::log("\n[L] Bouton jaune GAUCHE presse - Aucune bouee connectee");
        }
    }
    
    // Bouton jaune DROIT : Changement de bouée suivante
    if (joystick.wasButtonPressed(BTN_RIGHT_STICK)) {
        Logger::log("\n[R] Bouton jaune DROIT presse - Bouee suivante");
        buoyState.selectNextBuoy();
        display.displayBuoySelection();
    }
    
    // Lecture des boutons sur les joysticks (pour futures fonctionnalités)
    if (joystick.wasButtonPressed(BTN_LEFT)) {
        Logger::log("\n[JS-L] Bouton stick joystick GAUCHE presse");
        // TODO: Ajouter fonctionnalité (ex: changer mode navigation)
    }
    
    if (joystick.wasButtonPressed(BTN_RIGHT)) {
        Logger::log("\n[JS-R] Bouton stick joystick DROIT presse");
        // TODO: Ajouter fonctionnalité (ex: reset heading, return home)
    }
    
    // Lecture du bouton écran Atom S3
    if (joystick.wasAtomScreenPressed()) {
        Logger::log("\n[ATOM] Bouton ecran Atom S3 presse");
        buoyState.selectNextBuoy();
        display.displayBuoySelection();
    }
    
    // Détection de maintien du bouton écran (ex: 2 secondes)
    if (joystick.isAtomScreenHeld(2000)) {
        static bool longPressHandled = false;
        if (!longPressHandled) {
            Logger::log("\n[ATOM] Bouton ecran MAINTENU (2s)");
            longPressHandled = true;
            // TODO: Ajouter fonctionnalité (ex: reset, calibration)
        }
        // Reset le flag quand le bouton est relâché
        if (joystick.wasAtomScreenReleased()) {
            longPressHandled = false;
        }
    }
    
    // ========================================================================
    // 2. MISE À JOUR ÉTAT DES BOUÉES ET NETTOYAGE
    // ========================================================================
    buoyState.update();
    
    // Nettoie les bouées inactives (toutes les 30 secondes)
    static uint32_t lastCleanup = 0;
    if (currentTime - lastCleanup > 30000) {
        lastCleanup = currentTime;
        espNow.removeInactiveBuoys(15000);  // Supprime les bouées inactives > 15s
    }
    
    // ========================================================================
    // 3. ENVOI DE COMMANDES (À IMPLÉMENTER)
    // ========================================================================
    // TODO: Implémenter CommandManager et génération de commandes
    
    // ========================================================================
    // 4. MISE À JOUR AFFICHAGE
    // ========================================================================
    display.update();
    
    // ========================================================================
    // 5. DEBUG SÉRIE (toutes les 2 secondes)
    // ========================================================================
    static uint32_t lastDebug = 0;
    if (currentTime - lastDebug > 2000) {
        lastDebug = currentTime;
        
        Logger::log("\n--- Etat systeme ---");
        
        // Joystick
        Logger::logf("Joystick L: X=%d Y=%d",
                     joystick.getAxisCentered(AXIS_LEFT_X),
                     joystick.getAxisCentered(AXIS_LEFT_Y));
        Logger::logf("Joystick R: X=%d Y=%d",
                     joystick.getAxisCentered(AXIS_RIGHT_X),
                     joystick.getAxisCentered(AXIS_RIGHT_Y));
        Logger::logf("Batteries: %.2fV / %.2fV",
                     joystick.getBattery1Voltage(),
                     joystick.getBattery2Voltage());
        
        // Bouées
        uint8_t selectedId = buoyState.getSelectedBuoyId();
        Logger::logf("Bouee selectionnee: #%d", selectedId);
        Logger::logf("Bouees connectees: %d/%d",
                     buoyState.getConnectedBuoyCount(),
                     espNow.getBuoyCount());
        
        if (buoyState.isSelectedBuoyConnected()) {
            BuoyState state = buoyState.getSelectedBuoyState();
            Logger::logf("  General Mode: %s", buoyState.getGeneralModeName(state.generalMode).c_str());
            Logger::logf("  Nav Mode: %s", buoyState.getNavModeName(state.navigationMode).c_str());
            Logger::logf("  Heading: %.0f deg Throttle: %d%%", 
                         state.autoPilotTrueHeadingCmde, 
                         state.autoPilotThrottleCmde);
            uint8_t batteryPercent = (uint8_t)((state.remainingCapacity / 10000.0) * 100);
            Logger::logf("  GPS: %s Battery: %d%% Temp: %.1fC",
                         state.gpsOk ? "OK" : "NO",
                         batteryPercent,
                         state.temperature);
        } else {
            Logger::log("  (Deconnectee)");
        }
        
        Logger::log("-------------------\n");
    }
}
