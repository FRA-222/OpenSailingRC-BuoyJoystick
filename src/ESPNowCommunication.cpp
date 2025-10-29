/**
 * @file ESPNowCommunication.cpp
 * @brief Implémentation de la communication ESP-NOW
 * @author Philippe Hubert
 * @date 2025
 */

#include "ESPNowCommunication.h"
#include "Logger.h"

// Instance statique pour les callbacks
ESPNowCommunication* ESPNowCommunication::instance = nullptr;

ESPNowCommunication::ESPNowCommunication() {
    buoyCount = 0;
    newDataAvailable = false;
    instance = this;
    
    // Initialise le tableau de bouées
    for (int i = 0; i < MAX_BUOYS; i++) {
        buoys[i].registered = false;
        buoys[i].buoyId = i;
        buoys[i].lastState.buoyId = i;
        buoys[i].lastState.timestamp = 0;
    }
}

bool ESPNowCommunication::begin() {
    // Configure WiFi en mode Station
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    // Obtient et affiche l'adresse MAC locale
    WiFi.macAddress(localMac);
    Logger::print("✓ ESP-NOW: Adresse MAC locale: ");
    for (int i = 0; i < 6; i++) {
        Logger::printf("%02X", localMac[i]);
        if (i < 5) Logger::print(":");
    }
    Logger::log();
    
    // Initialise ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Logger::log("✗ ESP-NOW: Échec initialisation");
        return false;
    }
    
    Logger::log("✓ ESP-NOW: Initialisé");
    
    // Enregistre les callbacks
    esp_now_register_recv_cb(onDataRecv);
    esp_now_register_send_cb(onDataSent);
    
    return true;
}

bool ESPNowCommunication::addBuoy(uint8_t buoyId, const uint8_t* macAddress) {
    Logger::log("⚠️  ESP-NOW: addBuoy() est obsolète - utilisation de la découverte automatique");
    return addBuoyDynamically(macAddress, buoyId);
}

bool ESPNowCommunication::addBuoyDynamically(const uint8_t* macAddress, uint8_t buoyId) {
    if (buoyId >= MAX_BUOYS) {
        Logger::logf("✗ ESP-NOW: ID bouée invalide %d", buoyId);
        return false;
    }
    
    // Vérifie si cette bouée existe déjà
    int8_t existingIndex = findBuoyByMac(macAddress);
    if (existingIndex >= 0) {
        // Bouée déjà connue, met à jour son ID si nécessaire
        if (buoys[existingIndex].buoyId != buoyId) {
            Logger::logf("🔄 ESP-NOW: Bouée change d'ID %d -> %d", 
                         buoys[existingIndex].buoyId, buoyId);
            buoys[existingIndex].buoyId = buoyId;
        }
        return true;
    }
    
    // Trouve un slot libre
    int8_t freeSlot = -1;
    for (int i = 0; i < MAX_BUOYS; i++) {
        if (!buoys[i].registered) {
            freeSlot = i;
            break;
        }
    }
    
    if (freeSlot < 0) {
        Logger::logf("✗ ESP-NOW: Aucun slot libre pour Bouée #%d", buoyId);
        return false;
    }
    
    // Configure la nouvelle bouée
    memcpy(buoys[freeSlot].macAddress, macAddress, 6);
    buoys[freeSlot].buoyId = buoyId;
    buoys[freeSlot].registered = true;
    buoys[freeSlot].lastState.buoyId = buoyId;
    buoys[freeSlot].lastState.timestamp = 0;
    
    // Ajoute le peer ESP-NOW
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;  // Canal par défaut  
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Logger::logf("✗ ESP-NOW: Échec ajout Bouée #%d", buoyId);
        buoys[freeSlot].registered = false;
        return false;
    }
    
    buoyCount++;
    
    Logger::print("🆕 ESP-NOW: Bouée #");
    Logger::print(String(buoyId));
    Logger::print(" découverte - MAC: ");
    for (int i = 0; i < 6; i++) {
        Logger::printf("%02X", macAddress[i]);
        if (i < 5) Logger::print(":");
    }
    Logger::logf(" (total: %d bouées)", buoyCount);
    
    return true;
}

bool ESPNowCommunication::sendCommand(uint8_t buoyId, const Command& cmd) {
    int8_t index = findBuoyIndex(buoyId);
    if (index < 0) {
        Logger::logf("✗ ESP-NOW: Bouée #%d non trouvée", buoyId);
        return false;
    }
    
    // Prépare le paquet de commande
    CommandPacket packet;
    packet.targetBuoyId = cmd.targetBuoyId;
    packet.command = cmd.type;
    packet.heading = cmd.heading;
    packet.throttle = cmd.throttle;
    packet.timestamp = cmd.timestamp;
    
    // Envoie via ESP-NOW
    esp_err_t result = esp_now_send(
        buoys[index].macAddress,
        (uint8_t*)&packet,
        sizeof(CommandPacket)
    );
    
    if (result == ESP_OK) {
        Logger::logf("→ Commande envoyée à Bouée #%d (type=%d)", buoyId, cmd.type);
        Logger::log("Tentative d'envoi à MAC: ");
        for (int i = 0; i < 6; i++) {
            Logger::printf("%02X", buoys[index].macAddress[i]);
            if (i < 5) Logger::print(":");
        }
        Logger::log();
        return true;
    } else {
        Logger::logf("✗ ESP-NOW: Échec envoi à Bouée #%d (err=%d)", buoyId, result);
        return false;
    }
}

BuoyState ESPNowCommunication::getLastBuoyState() {
    // Retourne l'état de la dernière bouée qui a envoyé des données
    for (int i = 0; i < MAX_BUOYS; i++) {
        if (buoys[i].registered && buoys[i].lastState.timestamp > 0) {
            return buoys[i].lastState;
        }
    }
    
    // Retourne un état vide si aucune donnée
    BuoyState emptyState = {};
    return emptyState;
}

BuoyState ESPNowCommunication::getBuoyState(uint8_t buoyId) {
    int8_t index = findBuoyIndex(buoyId);
    if (index >= 0) {
        return buoys[index].lastState;
    }
    
    // Retourne un état vide si bouée non trouvée
    BuoyState emptyState = {};
    emptyState.buoyId = buoyId;
    return emptyState;
}

bool ESPNowCommunication::hasNewData() {
    return newDataAvailable;
}

void ESPNowCommunication::clearNewData() {
    newDataAvailable = false;
}

uint8_t ESPNowCommunication::getBuoyCount() {
    return buoyCount;
}

bool ESPNowCommunication::isBuoyConnected(uint8_t buoyId, uint32_t timeoutMs) {
    int8_t index = findBuoyIndex(buoyId);
    if (index < 0) {
        return false;
    }
    
    uint32_t lastUpdate = buoys[index].lastState.timestamp;
    if (lastUpdate == 0) {
        return false;  // Aucune donnée reçue
    }
    
    return (millis() - lastUpdate) < timeoutMs;
}

const uint8_t* ESPNowCommunication::getLocalMacAddress() {
    return localMac;
}

uint8_t ESPNowCommunication::removeInactiveBuoys(uint32_t timeoutMs) {
    uint8_t removedCount = 0;
    uint32_t now = millis();
    
    for (int i = 0; i < MAX_BUOYS; i++) {
        if (buoys[i].registered) {
            // Vérifie si la bouée est inactive
            if (buoys[i].lastState.timestamp == 0 || 
                (now - buoys[i].lastState.timestamp) > timeoutMs) {
                
                // Supprime le peer ESP-NOW
                esp_err_t result = esp_now_del_peer(buoys[i].macAddress);
                if (result == ESP_OK) {
                    Logger::logf("🗑️  ESP-NOW: Bouée #%d supprimée (inactive)", buoys[i].buoyId);
                } else {
                    Logger::logf("⚠️  ESP-NOW: Échec suppression peer Bouée #%d", buoys[i].buoyId);
                }
                
                // Remet à zéro le slot
                buoys[i].registered = false;
                buoys[i].buoyId = i;
                buoys[i].lastState.timestamp = 0;
                memset(buoys[i].macAddress, 0, 6);
                
                buoyCount--;
                removedCount++;
            }
        }
    }
    
    if (removedCount > 0) {
        Logger::logf("🧹 ESP-NOW: %d bouée(s) inactive(s) supprimée(s) (total restant: %d)", 
                     removedCount, buoyCount);
    }
    
    return removedCount;
}

// Callback statique pour réception de données
void ESPNowCommunication::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (instance) {
        instance->handleReceivedData(mac, data, len);
    }
}

// Callback statique pour envoi de données
void ESPNowCommunication::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Logger::log("✓ ESP-NOW: Envoi réussi");
    } else {
        Logger::log("✗ ESP-NOW: Échec envoi");
    }
}

// Traite les données reçues
void ESPNowCommunication::handleReceivedData(const uint8_t* mac, const uint8_t* data, int len) {
    // Vérifie la taille des données
    if (len != sizeof(BuoyState)) {
        Logger::logf("✗ ESP-NOW: Taille invalide reçue %d (attendu %d)", len, sizeof(BuoyState));
        return;
    }
    
    // Parse l'état reçu pour obtenir l'ID de la bouée
    BuoyState receivedState;
    memcpy(&receivedState, data, sizeof(BuoyState));
    
    // Trouve la bouée par son adresse MAC
    int8_t index = findBuoyByMac(mac);
    if (index < 0) {
        // Bouée inconnue - tentative de découverte automatique
        Logger::print("📡 ESP-NOW: Nouvelle bouée détectée - MAC: ");
        for (int i = 0; i < 6; i++) {
            Logger::printf("%02X", mac[i]);
            if (i < 5) Logger::print(":");
        }
        Logger::logf(" ID: %d", receivedState.buoyId);
        
        // Tente d'ajouter la bouée dynamiquement
        if (addBuoyDynamically(mac, receivedState.buoyId)) {
            // Trouve l'index de la bouée nouvellement ajoutée
            index = findBuoyByMac(mac);
        } else {
            Logger::logf("✗ ESP-NOW: Impossible d'ajouter la Bouée #%d", receivedState.buoyId);
            return;
        }
    }
    
    // Copie l'état reçu
    memcpy(&buoys[index].lastState, data, sizeof(BuoyState));
    buoys[index].lastState.timestamp = millis();
    
    newDataAvailable = true;
    
    uint8_t batteryPercent = (uint8_t)((buoys[index].lastState.remainingCapacity / 10000.0) * 100);
    Logger::logf("← État reçu de Bouée #%d (genMode=%d, navMode=%d, bat=%d%%, GPS=%s)",
                  buoys[index].lastState.buoyId,
                  buoys[index].lastState.generalMode,
                  buoys[index].lastState.navigationMode,
                  batteryPercent,
                  buoys[index].lastState.gpsOk ? "OK" : "NO");
}

int8_t ESPNowCommunication::findBuoyIndex(uint8_t buoyId) {
    for (int i = 0; i < MAX_BUOYS; i++) {
        if (buoys[i].registered && buoys[i].buoyId == buoyId) {
            return i;
        }
    }
    return -1;
}

int8_t ESPNowCommunication::findBuoyByMac(const uint8_t* mac) {
    for (int i = 0; i < MAX_BUOYS; i++) {
        if (buoys[i].registered && 
            memcmp(buoys[i].macAddress, mac, 6) == 0) {
            return i;
        }
    }
    return -1;
}
