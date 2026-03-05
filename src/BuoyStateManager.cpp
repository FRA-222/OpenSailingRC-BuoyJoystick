/**
 * @file BuoyStateManager.cpp
 * @brief Implémentation de la gestion d'état des bouées
 * @author Philippe Hubert
 * @date 2025
 */

#include "BuoyStateManager.h"
#include "DisplayManager.h"
#include "Logger.h"

BuoyStateManager::BuoyStateManager(ICommunication& comm) 
    : comm(comm), displayMgr(nullptr) {
    selectedBuoyId = 0;
    lastUpdateTime = 0;
    lastConnectedBuoyId = 255;
}

void BuoyStateManager::begin() {
    Logger::log("✓ BuoyStateManager: Initialisé");
}

void BuoyStateManager::setDisplayManager(DisplayManager* display) {
    displayMgr = display;
}

void BuoyStateManager::update() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // Vérifie si de nouvelles données sont disponibles
        if (comm.hasNewData()) {
            comm.clearNewData();
            // Les données sont déjà traitées dans ESPNowCommunication/LoRaCommunication
        }
        
        // Mise à jour du statut de connexion pour détecter les reconnexions
        // NOTE: Pas d'auto-sélection - l'utilisateur choisit la bouée manuellement
        // via selectNextBuoy() (appui écran), même si elle est déconnectée.
        if (comm.isBuoyConnected(selectedBuoyId) && lastConnectedBuoyId != selectedBuoyId) {
            lastConnectedBuoyId = selectedBuoyId;
            if (displayMgr != nullptr) {
                Logger::log("🔄 Rafraîchissement écran - Bouée reconnectée");
                displayMgr->forceRefresh();
            }
        } else if (!comm.isBuoyConnected(selectedBuoyId) && lastConnectedBuoyId == selectedBuoyId) {
            lastConnectedBuoyId = 255;  // Marquer comme déconnectée
        }
    }
}

void BuoyStateManager::selectNextBuoy() {
    // Cycle simple entre bouées 0-5 (pas de vérification de connexion)
    selectedBuoyId = (selectedBuoyId + 1) % MAX_BUOYS;
    Logger::logf("→ Bouée sélectionnée: #%d", selectedBuoyId);
}

void BuoyStateManager::selectPreviousBuoy() {
    // Cycle simple entre bouées 0-5 en arrière (pas de vérification de connexion)
    if (selectedBuoyId == 0) {
        selectedBuoyId = MAX_BUOYS - 1;
    } else {
        selectedBuoyId--;
    }
    Logger::logf("→ Bouée sélectionnée: #%d", selectedBuoyId);
}

void BuoyStateManager::selectBuoy(uint8_t buoyId) {
    if (buoyId < MAX_BUOYS) {
        selectedBuoyId = buoyId;
        Logger::logf("✓ Bouée sélectionnée: #%d", selectedBuoyId);
    }
}

uint8_t BuoyStateManager::getSelectedBuoyId() {
    return selectedBuoyId;
}

BuoyState BuoyStateManager::getSelectedBuoyState() {
    return comm.getBuoyState(selectedBuoyId);
}

uint8_t BuoyStateManager::getConnectedBuoyCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_BUOYS; i++) {
        if (comm.isBuoyConnected(i)) {
            count++;
        }
    }
    return count;
}

bool BuoyStateManager::isSelectedBuoyConnected() {
    return comm.isBuoyConnected(selectedBuoyId);
}

String BuoyStateManager::getBuoyName(uint8_t buoyId) {
    return "Buoy #" + String(buoyId + 1);
}

String BuoyStateManager::getNavModeName(tEtatsNav mode) {
    switch (mode) {
        case NAV_NOTHING:
            return "NOTHING";
        case NAV_HOME:
            return "HOME";
        case NAV_HOLD:
            return "HOLD";
        case NAV_STOP:
            return "STOP";
        case NAV_BASIC:
            return "BASIC";
        case NAV_CAP:
            return "CAP";
        case NAV_TARGET:
            return "TARGET";
        default:
            return "INCONNU";
    }
}

String BuoyStateManager::getGeneralModeName(tEtatsGeneral mode) {
    switch (mode) {
        case INIT:
            return "INIT";
        case READY:
            return "READY";
        case MAINTENANCE:
            return "MAINT";
        case HOME_DEFINITION:
            return "HOME_DEF";
        case NAV:
            return "NAV";
        default:
            return "UNKNOWN";
    }
}
