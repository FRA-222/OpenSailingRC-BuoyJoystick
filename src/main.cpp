/**
 * @file main.cpp
 * @brief Programme principal du joystick de contrôle de bouées
 * @author Philippe Hubert
 * @date 2025
 * 
 * Ossature avec communication ESP-NOW ou LoRa bidirectionnelle et affichage LCD
 */

#include <Arduino.h>
#include <M5Unified.h>
#include "Logger.h"
#include "JoystickManager.h"
#include "CommunicationConfig.h"
#include "ESPNowCommunication.h"
#include "LoRaCommunication.h"
#include "BuoyStateManager.h"
#include "DisplayManager.h"
#include "CommandManager.h"

// ============================================================================
// CONFIGURATION - MODE DE COMMUNICATION
// ============================================================================
// Changer le mode de communication ici :
// - CommMode::ESP_NOW : Communication ESP-NOW (2.4 GHz, courte portée, rapide)
// - CommMode::LORA     : Communication LoRa (920 MHz, longue portée, lente)
#define COMM_MODE CommMode::LORA

// ============================================================================
// CONFIGURATION - DÉCOUVERTE AUTOMATIQUE DES BOUÉES
// ============================================================================
// Les bouées sont maintenant découvertes automatiquement via leurs broadcasts.
// Plus besoin de configurer manuellement les adresses MAC !

// ============================================================================
// INSTANCES DES MANAGERS
// ============================================================================
JoystickManager joystick;
ESPNowCommunication espNow;
LoRaCommunication lora;

// Instances statiques pour chaque mode
BuoyStateManager buoyStateESPNow(espNow);
CommandManager cmdManagerESPNow(espNow);
DisplayManager displayESPNow(buoyStateESPNow);

BuoyStateManager buoyStateLora(lora);
CommandManager cmdManagerLora(lora);
DisplayManager displayLora(buoyStateLora);

// Pointeurs vers les instances actives selon le mode
BuoyStateManager* buoyState = nullptr;
DisplayManager* display = nullptr;
CommandManager* cmdManager = nullptr;

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================
uint32_t lastLoopTime = 0;
const uint32_t LOOP_INTERVAL = 100;  // 10Hz

uint32_t lastHeartbeatTime = 0;
const uint32_t HEARTBEAT_INTERVAL = 3000;  // 3 secondes

// ============================================================================
// TÂCHE FREERTOS POUR RÉCEPTION LORA
// ============================================================================
TaskHandle_t loraRxTaskHandle = NULL;

/**
 * @brief Tâche dédiée à la réception LoRa (Core 0)
 * Cette tâche s'exécute en parallèle du loop principal pour optimiser
 * la réception des RESPONSE sans ralentir l'envoi des commandes
 */
void loraRxTask(void* parameter) {
    Logger::log("# Tâche LoRa RX démarrée sur Core 0");
    
    while (true) {        
        lora.listenForResponses();

        // Courte pause avant la prochaine série d'écoutes
        vTaskDelay(1 / portTICK_PERIOD_MS);  // 1ms
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    // M5Stack Atom S3 initialization (doit être fait en premier)
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // Initialisation du Logger (sortie série uniquement par défaut)
    Logger::init(true, false);  // Serial activé, LCD désactivé

    // 6. Initialisation de l'affichage
    Logger::log();
    Logger::log("6. Initialisation Display...");
    if (!display->begin()) {
        Logger::log("   -> ERREUR: Echec initialisation display");
    } else {
        Logger::log("   -> Display: OK");
    }

    // Initialisation Serial pour debug (non bloquant)
    USBSerial.begin(115200);
    delay(100);  // Court délai pour stabilisation (non bloquant)
    
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
    
    // 2. Initialisation de la communication
    Logger::log();
    if (COMM_MODE == CommMode::ESP_NOW) {
        Logger::log("2. Initialisation ESP-NOW...");
        if (!espNow.begin()) {
            Logger::log("   -> ERREUR CRITIQUE: Echec initialisation ESP-NOW");
            while (1) {
                delay(1000);
            }
        } else {
            Logger::log("   -> ESP-NOW: OK");
        }
    } else {
        Logger::log("2. Initialisation LoRa 920...");
        if (!lora.begin()) {
            Logger::log("   -> ERREUR CRITIQUE: Echec initialisation LoRa");
            while (1) {
                delay(1000);
            }
        } else {
            Logger::log("   -> LoRa 920: OK");
        }
    }
    
    // 3. Préparation pour découverte automatique des bouées
    Logger::log();
    Logger::log("3. Attente découverte automatique des bouées...");
    Logger::log("   -> Les bouées seront ajoutées automatiquement");
    Logger::log("   -> lors de la réception de leurs broadcasts");
    
    // 4. Sélection du mode de communication et des managers associés
    Logger::log();
    Logger::log("4. Configuration du mode de communication...");
    
    if (COMM_MODE == CommMode::ESP_NOW) {
        Logger::log("   -> Mode: ESP-NOW (2.4 GHz)");
        buoyState = &buoyStateESPNow;
        cmdManager = &cmdManagerESPNow;
        display = &displayESPNow;
        // Connecter le display pour feedback visuel
        espNow.setDisplayManager(display);
    } else {
        Logger::log("   -> Mode: LoRa 920 MHz");
        buoyState = &buoyStateLora;
        cmdManager = &cmdManagerLora;
        display = &displayLora;
        // Connecter le display pour feedback visuel
        lora.setDisplayManager(display);
    }
    
    // Initialiser BuoyStateManager
    Logger::log();
    Logger::log("5. Initialisation BuoyStateManager...");
    buoyState->begin();
    buoyState->setDisplayManager(display);
    
    
    
    Logger::log();
    Logger::log("===========================================");
    Logger::log("  SYSTEM READY");
    Logger::log("  Mode simplifié: COMMAND + RESPONSE");
    Logger::log("===========================================");
    Logger::log();
    
    // Affiche écran de connexion
    display->displayConnecting("Ready");
    
    // En mode LoRa, créer la tâche de réception sur Core 0
    if (COMM_MODE == CommMode::LORA) {
        Logger::log();
        Logger::log("7. Création tâche LoRa RX sur Core 0...");
        xTaskCreatePinnedToCore(
            loraRxTask,         // Fonction de la tâche
            "LoRaRxTask",       // Nom de la tâche
            4096,               // Taille de pile (4KB)
            NULL,               // Paramètre
            1,                  // Priorité (1 = normale)
            &loraRxTaskHandle,  // Handle de la tâche
            0                   // Core 0 (le loop() s'exécute sur Core 1)
        );
        Logger::log("   -> Tâche LoRa RX créée sur Core 0");
    }
    
    delay(2000);
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================
void loop() {
    uint32_t currentTime = millis();
    
    // Maintient une fréquence de loop stable (10Hz)
    if (currentTime - lastLoopTime < LOOP_INTERVAL) {
        vTaskDelay(1);
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
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        Logger::logf("\n[L] Bouton jaune GAUCHE presse - Envoi CMD_INIT_HOME a Bouee #%d", selectedId);
        cmdManager->generateInitHomeCommand(selectedId);
    }
    
    // Bouton jaune DROIT : Validation HOME
    if (joystick.wasButtonPressed(BTN_RIGHT_STICK)) {
        uint8_t activeBuoy = buoyState->getSelectedBuoyId();
        Logger::logf("\n[R] Bouton jaune DROIT presse - Envoi HOME_VALIDATION a Bouee #%d", activeBuoy);
        cmdManager->generateHomeValidationCommand(activeBuoy);
    }
    
    // Lecture des boutons sur les joysticks
    // Bouton joystick GAUCHE : CMD_NAV_HOLD
    if (joystick.wasButtonPressed(BTN_LEFT)) {
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        Logger::logf("\n[JS-L] Bouton stick joystick GAUCHE presse - NAV_HOLD (Bouee #%d)", selectedId);
        cmdManager->generateNavHoldCommand(selectedId);
    }
    
    
    // Détection mouvement joystick GAUCHE (Y axis)
    // Seuil pour détecter un mouvement significatif (valeur centrée autour de 0)
    static const int16_t JOYSTICK_THRESHOLD = 1500;  // Seuil de détection (sur ~2048)
    static bool leftUpProcessed = false;
    static bool leftDownProcessed = false;
    
    int16_t leftY = joystick.getAxisCentered(AXIS_LEFT_Y);

    // Joystick GAUCHE vers le HAUT : CMD_NAV_CAP
    if (leftY < -JOYSTICK_THRESHOLD && !leftUpProcessed)
    {
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        Logger::logf("\n[JS-L] Joystick GAUCHE vers le HAUT - NAV_CAP (Bouee #%d)", selectedId);
        cmdManager->generateNavCapCommand(selectedId);
        leftUpProcessed = true;
    }
    else if (leftY > -JOYSTICK_THRESHOLD / 2)
    {
        leftUpProcessed = false; // Reset quand joystick revient au centre
    }

    // Joystick GAUCHE vers le BAS : CMD_NAV_HOME
    if (leftY > JOYSTICK_THRESHOLD && !leftDownProcessed)
    {
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        Logger::logf("\n[JS-L] Joystick GAUCHE vers le BAS - NAV_HOME (Bouee #%d)", selectedId);
        cmdManager->generateNavHomeCommand(selectedId);
        leftDownProcessed = true;
    }
    else if (leftY < JOYSTICK_THRESHOLD / 2)
    {
        leftDownProcessed = false; // Reset quand joystick revient au centre
    }

    // ========================================================================
    // JOYSTICK DROIT - Contrôle Throttle et Heading
    // ========================================================================
    
    // Détection mouvement joystick DROIT
    static bool rightUpProcessed = false;
    static bool rightDownProcessed = false;
    static bool rightRightProcessed = false;
    static bool rightLeftProcessed = false;
    
    int16_t rightY = joystick.getAxisCentered(AXIS_RIGHT_Y);
    int16_t rightX = joystick.getAxisCentered(AXIS_RIGHT_X);
    
    // Récupération de l'état actuel de la bouée pour incrémenter les valeurs
    uint8_t selectedId = buoyState->getSelectedBuoyId();
    BuoyState currentBuoyState = buoyState->getSelectedBuoyState();

    // Joystick DROIT vers le HAUT : CMD_THROTTLE_INCREASE
    if (rightY < -JOYSTICK_THRESHOLD && !rightUpProcessed)
    {
        Logger::logf("\n[JS-R] Joystick DROIT vers le HAUT - THROTTLE_INCREASE (Bouee #%d)", selectedId);
        cmdManager->generateThrottleIncreaseCommand(selectedId);
        rightUpProcessed = true;
    }
    else if (rightY > -JOYSTICK_THRESHOLD / 2)
    {
        rightUpProcessed = false; // Reset
    }

    // Joystick DROIT vers le BAS : CMD_THROTTLE_DECREASE
    if (rightY > JOYSTICK_THRESHOLD && !rightDownProcessed)
    {
        Logger::logf("\n[JS-R] Joystick DROIT vers le BAS - THROTTLE_DECREASE (Bouee #%d)", selectedId);
        cmdManager->generateThrottleDecreaseCommand(selectedId);
        rightDownProcessed = true;
    }
    else if (rightY < JOYSTICK_THRESHOLD / 2)
    {
        rightDownProcessed = false; // Reset
    }

    // Joystick DROIT vers la DROITE : CMD_HEADING_INCREASE
    if (rightX > JOYSTICK_THRESHOLD && !rightRightProcessed)
    {
        Logger::logf("\n[JS-R] Joystick DROIT vers la DROITE - HEADING_INCREASE (Bouee #%d)", selectedId);
        cmdManager->generateHeadingIncreaseCommand(selectedId);
        rightRightProcessed = true;
    }
    else if (rightX < JOYSTICK_THRESHOLD / 2)
    {
        rightRightProcessed = false; // Reset
    }

    // Joystick DROIT vers la GAUCHE : CMD_HEADING_DECREASE
    if (rightX < -JOYSTICK_THRESHOLD && !rightLeftProcessed)
    {
        Logger::logf("\n[JS-R] Joystick DROIT vers la GAUCHE - HEADING_DECREASE (Bouee #%d)", selectedId);
        cmdManager->generateHeadingDecreaseCommand(selectedId);
        rightLeftProcessed = true;
    }
    else if (rightX > -JOYSTICK_THRESHOLD / 2)
    {
        rightLeftProcessed = false; // Reset
    }

    // Bouton joystick DROIT : CMD_NAV_STOP
    if (joystick.wasButtonPressed(BTN_RIGHT))
    {
        Logger::logf("\n[JS-R] Bouton stick joystick DROIT presse - NAV_STOP (Bouee #%d)", selectedId);
        cmdManager->generateNavStopCommand(selectedId);
    }

    // Lecture du bouton écran Atom S3
    if (joystick.wasAtomScreenPressed())
    {
        Logger::log("\n[ATOM] Bouton ecran Atom S3 presse");
        buoyState->selectNextBuoy();
        
        display->displayBuoySelection();
    }

    // ========================================================================
    // 2. MISE À JOUR COMMUNICATION ET ÉTAT DES BOUÉES
    // ========================================================================
    
    // Mode ESP-NOW : Réception via callbacks (pas besoin d'appel explicite)
    // Mode LoRa : La réception est gérée par loraRxTask() sur Core 0 (non-bloquant)
    
    // Traiter les retry de commandes en attente d'ACK
    if (COMM_MODE == CommMode::LORA) {
        lora.processCommandRetries();
    } else if (COMM_MODE == CommMode::ESP_NOW) {
        espNow.processCommandRetries();
    }
    
    // Mise à jour de l'état des bouées
    buoyState->update();
    
    // ========================================================================
    // 3. ENVOI HEARTBEAT PÉRIODIQUE (toutes les 5 secondes)
    // ========================================================================
    if (currentTime - lastHeartbeatTime >= HEARTBEAT_INTERVAL) {
        lastHeartbeatTime = currentTime;
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        // Envoyer heartbeat uniquement à la bouée sélectionnée
        cmdManager->generateHeartbeatCommand(selectedId);
        Logger::logf("💓 Heartbeat envoyé à Bouée #%d", selectedId);
    } 
    
    // ========================================================================
    // 4. MISE À JOUR AFFICHAGE
    // ========================================================================
    display->update();
    
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
        
        // Bouées - Affichage simplifié
        uint8_t selectedId = buoyState->getSelectedBuoyId();
        Logger::logf("Bouee selectionnee: #%d", selectedId);
        
        // Afficher les données si la bouée répond
        BuoyInfo* buoyInfo = lora.getBuoyInfo(selectedId);
        if (buoyInfo != nullptr && buoyInfo->lastUpdateTime > 0) {  
            BuoyState state = buoyInfo->lastState;
            uint32_t age = millis() - buoyInfo->lastUpdateTime;  // Utiliser timestamp LOCAL
            Logger::logf("  Donnees recues il y a %lu ms", age);
            Logger::logf("  General Mode: %s", buoyState->getGeneralModeName(state.generalMode).c_str());
            Logger::logf("  Nav Mode: %s", buoyState->getNavModeName(state.navigationMode).c_str());
            Logger::logf("  Heading: %.0f deg Throttle: %d%%", 
                         state.autoPilotTrueHeadingCmde, 
                         state.autoPilotThrottleCmde);
            uint8_t batteryPercent = (uint8_t)((state.remainingCapacity / 10000.0) * 100);
            Logger::logf("  GPS: %s Battery: %d%% Temp: %.1fC",
                         state.gpsOk ? "OK" : "NO",
                         batteryPercent,
                         state.temperature);
        } else {
            Logger::log("  Aucune donnee recue (bouee inactive ou hors portee)");
        }
        
        Logger::log("-------------------\n");
    }
}
