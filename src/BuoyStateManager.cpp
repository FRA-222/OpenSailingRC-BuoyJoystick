/**
 * @file BuoyStateManager.cpp
 * @brief Implémentation de la gestion d'état des bouées
 * @author Philippe Hubert
 * @date 2025
 */

#include "BuoyStateManager.h"
#include "Logger.h"

BuoyStateManager::BuoyStateManager(ESPNowCommunication& espNow) 
    : espNowComm(espNow) {
    selectedBuoyId = 0;
    lastUpdateTime = 0;
}

void BuoyStateManager::begin() {
    Logger::log("✓ BuoyStateManager: Initialisé");
}

void BuoyStateManager::update() {
    uint32_t currentTime = millis();
    
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // Vérifie si de nouvelles données sont disponibles
        if (espNowComm.hasNewData()) {
            espNowComm.clearNewData();
            // Les données sont déjà traitées dans ESPNowCommunication
        }
    }
}

void BuoyStateManager::selectNextBuoy() {
    uint8_t startId = selectedBuoyId;
    uint8_t count = espNowComm.getBuoyCount();
    
    if (count == 0) {
        Logger::log("⚠ BuoyStateManager: Aucune bouée enregistrée");
        return;
    }
    
    // Cherche la prochaine bouée enregistrée
    do {
        selectedBuoyId = (selectedBuoyId + 1) % MAX_BUOYS;
        
        // Vérifie si cette bouée existe
        BuoyState state = espNowComm.getBuoyState(selectedBuoyId);
        if (state.buoyId == selectedBuoyId) {
            Logger::logf("→ Bouée sélectionnée: #%d", selectedBuoyId);
            return;
        }
        
    } while (selectedBuoyId != startId);
}

void BuoyStateManager::selectPreviousBuoy() {
    uint8_t startId = selectedBuoyId;
    uint8_t count = espNowComm.getBuoyCount();
    
    if (count == 0) {
        Logger::log("⚠ BuoyStateManager: Aucune bouée enregistrée");
        return;
    }
    
    // Cherche la bouée précédente enregistrée
    do {
        if (selectedBuoyId == 0) {
            selectedBuoyId = MAX_BUOYS - 1;
        } else {
            selectedBuoyId--;
        }
        
        // Vérifie si cette bouée existe
        BuoyState state = espNowComm.getBuoyState(selectedBuoyId);
        if (state.buoyId == selectedBuoyId) {
            Logger::logf("← Bouée sélectionnée: #%d", selectedBuoyId);
            return;
        }
        
    } while (selectedBuoyId != startId);
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
    return espNowComm.getBuoyState(selectedBuoyId);
}

uint8_t BuoyStateManager::getConnectedBuoyCount() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_BUOYS; i++) {
        if (espNowComm.isBuoyConnected(i)) {
            count++;
        }
    }
    return count;
}

bool BuoyStateManager::isSelectedBuoyConnected() {
    return espNowComm.isBuoyConnected(selectedBuoyId);
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
