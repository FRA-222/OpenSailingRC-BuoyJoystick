# OpenSailingRC BuoyJoystick

🌐 [English version](README.md)

Contrôleur de bouées autonomes GPS basé sur le **M5Stack Atom S3**.  
Envoie des commandes de navigation aux bouées via **LoRa 920 MHz** (longue portée) ou **ESP-NOW 2.4 GHz** (courte portée), et affiche en temps réel leur état sur l'écran LCD embarqué.

Ce projet fait partie de l'écosystème **OpenSailingRC** :

| Projet | Rôle |
|--------|------|
| **OpenSailingRC-BuoyJoystick** | Ce projet — contrôleur joystick |
| OpenSailingRC-BoatGPS | Bouée GPS autonome |
| OpenSailingRC-Display | Affichage multi-bouées |
| OpenSailingRC-Anemometer-v2 | Capteur anémomètre |

---

## Matériel requis

| Composant | Référence | Description |
|-----------|-----------|-------------|
| Contrôleur | M5Stack Atom S3 | ESP32-S3, écran LCD 128×128 |
| Joystick | M5Stack Dual Atom JoyStick | 2 joysticks + 4 boutons via I2C (STM32 @ 0x59) |
| Module radio (LoRa) | M5Stack Unit LoRaE220-920 | 920 MHz, longue portée |

> Le module LoRa est optionnel : le mode **ESP-NOW** fonctionne avec le seul Atom S3.

---

## Prérequis logiciels

- [PlatformIO](https://platformio.org/) (CLI ou extension VS Code)
- Librairies installées automatiquement par PlatformIO :
  - `m5stack/M5GFX`
  - `m5stack/M5Unified`
  - `m5stack/M5-LoRa-E220-JP`

---

## Démarrage rapide

### 1. Cloner et compiler

```bash
git clone <url-du-repo>
cd OpenSailingRC-BuoyJoystick
platformio run
```

### 2. Flasher

```bash
platformio run --target upload
```

### 3. Moniteur série (115 200 baud)

```bash
platformio device monitor
```

---

## Configuration

### Mode de communication

Le mode se choisit dans `src/main.cpp` en modifiant la constante `COMM_MODE` :

```cpp
// ESP-NOW : 2.4 GHz, courte portée (~100 m), pas de module externe requis
#define COMM_MODE CommMode::ESP_NOW

// LoRa : 920 MHz, longue portée (> 1 km), nécessite le module LoRaE220-920
#define COMM_MODE CommMode::LORA
```

> **Mode par défaut : LoRa.**  
> En mode LoRa, l'ESP-NOW est également activé en écoute passive pour recevoir les broadcasts d'état des bouées (plus réactif que le polling LoRa).

### Câblage LoRa (Atom S3 + Unit LoRaE220-920)

| Signal | Pin Atom S3 | Pin LoRaE220-920 |
|--------|-------------|-----------------|
| RX (Atom reçoit) | G1 | TX du module |
| TX (Atom envoie) | G2 | RX du module |

> Les switches M0/M1 du module doivent être sur **OFF** (mode normal transmission).  
> Mettre M0/M1 sur ON uniquement pour reconfigurer le module via `LORA_MODE_CONFIGURATION`.

### Paramètres LoRa configurés

| Paramètre | Valeur |
|-----------|--------|
| Fréquence | 920.6 MHz (canal 0x00, bande ISM Japon) |
| Débit radio | 2.4 kbps |
| Puissance TX | 22 dBm |
| Adresse joystick | 0x0007 |

---

## Contrôles

### Boutons

| Bouton | Index | Action |
|--------|-------|--------|
| BTN_LEFT_STICK | 0 | Pression courte → `CMD_INIT_HOME` |
| BTN_RIGHT_STICK | 1 | Pression courte → `CMD_HOME_VALIDATION` |
| BTN_LEFT | 2 | Pression courte → `CMD_NAV_HOLD` |
| BTN_RIGHT | 3 | Pression courte → `CMD_NAV_STOP` |
| BTN_ATOM_SCREEN | 4 | Pression courte → Sélectionner la bouée suivante |

### Joystick gauche (navigation)

| Mouvement | Commande envoyée |
|-----------|-----------------|
| Haut | `CMD_NAV_CAP` — passer en mode navigation par cap |
| Bas | `CMD_NAV_HOME` — retour au point Home |

### Joystick droit (throttle / cap)

| Mouvement | Commande envoyée |
|-----------|-----------------|
| Haut | `CMD_THROTTLE_INCREASE` |
| Bas | `CMD_THROTTLE_DECREASE` |
| Droite | `CMD_HEADING_INCREASE` |
| Gauche | `CMD_HEADING_DECREASE` |

> Le seuil de détection est `±1500` sur une plage de `±2048`.

---

## Architecture logicielle

```
main.cpp  (loop 10 Hz — Core 1)
│
├── JoystickManager        Lecture I2C des axes et boutons (STM32 @ 0x59)
│
├── ESPNowCommunication    Implémente ICommunication — mode broadcast ESP-NOW
├── LoRaCommunication      Implémente ICommunication — UART vers LoRaE220-920
│       └── loraRxTask()   Tâche FreeRTOS dédiée à la réception LoRa (Core 0)
│
├── BuoyStateManager       Sélection de la bouée active, consolidation des données
│                          (fusionne LoRa + ESP-NOW passif)
│
├── CommandManager         Génération et envoi des commandes (avec retry + ACK)
│
├── DisplayManager         Affichage LCD 128×128 (modes, cap, batterie, GPS…)
│
└── Logger                 Logging centralisé → USB Serial + LCD optionnel
```

### Interface `ICommunication`

Les deux modes radio (`ESPNowCommunication` et `LoRaCommunication`) implémentent la même interface `ICommunication`. `BuoyStateManager` et `CommandManager` sont agnostiques du mode utilisé.

---

## Commandes disponibles

```cpp
enum BuoyCommand : uint8_t {
    CMD_INIT_HOME = 0,      // Initialiser le point Home (position GPS courante)
    CMD_THROTTLE_INCREASE,  // Augmenter les gaz
    CMD_THROTTLE_DECREASE,  // Réduire les gaz
    CMD_HEADING_INCREASE,   // Augmenter le cap
    CMD_HEADING_DECREASE,   // Réduire le cap
    CMD_NAV_HOLD,           // Maintenir la position courante
    CMD_NAV_CAP,            // Navigation par cap
    CMD_NAV_HOME,           // Retour au point Home
    CMD_NAV_STOP,           // Arrêt complet
    CMD_HOME_VALIDATION,    // Valider le point Home
    CMD_HEARTBEAT           // Keepalive (envoi automatique toutes les 3 s)
};
```

---

## Structures de données

### Paquet de commande (Joystick → Bouée)

```cpp
struct __attribute__((packed)) CommandPacket {
    uint8_t  targetBuoyId;   // ID de la bouée cible (0–7)
    BuoyCommand command;     // Type de commande (BuoyCommand)
    uint32_t timestamp;      // Horodatage millis()
    uint16_t sequenceNumber; // Numéro de séquence (déduplication)
    uint8_t  ttl;            // 1 = paquet original, 0 = relayé par un Hub
};
```

### État de la bouée (Bouée → Joystick)

```cpp
struct BuoyState {
    uint8_t        buoyId;
    uint32_t       timestamp;
    tEtatsGeneral  generalMode;              // INIT / READY / MAINTENANCE / HOME_DEFINITION / NAV
    tEtatsNav      navigationMode;           // NAV_NOTHING / NAV_HOME / NAV_HOLD / NAV_STOP / NAV_CAP / NAV_BASIC / NAV_TARGET
    bool           gpsOk;
    bool           headingOk;
    bool           yawRateOk;
    double         latitude;
    double         longitude;
    float          temperature;
    float          remainingCapacity;        // mAh restants
    float          distanceToCons;           // Distance à la consigne (m)
    int8_t         autoPilotThrottleCmde;    // Throttle autopilote (-100 à +100 %)
    float          autoPilotTrueHeadingCmde; // Cap autopilote (degrés)
    uint16_t       sequenceNumber;
    uint8_t        ttl;
};
```

### Hiérarchie des modes de navigation

```
tEtatsGeneral (niveau 1)    tEtatsNav (niveau 2, actif si generalMode == NAV)
─────────────────────────   ──────────────────────────────────────────────────
INIT                        NAV_NOTHING
READY                       NAV_HOME
MAINTENANCE                 NAV_HOLD
HOME_DEFINITION             NAV_STOP
NAV ──────────────────────→ NAV_CAP
                            NAV_BASIC
                            NAV_TARGET
```

---

## Affichage LCD

```
┌────────────────────────┐
│  Bouée #1  ●           │  Header (cyan) + indicateur état
├────────────────────────┤
│   CONNECTE             │  Vert / Rouge selon connexion
│                        │
│   NAV                  │  Mode général
│   NAV_CAP              │  Mode navigation
│                        │
│   Cap: 045°            │  Cap autopilote
│   75%                  │  Throttle
│                        │
│  GPS  BAT:87%  -72dBm  │  Capteurs et signal
└────────────────────────┘
```

**Codes couleur :**
- Vert : connecté, batterie > 50 %
- Orange : batterie 20–50 %
- Rouge : déconnecté ou batterie < 20 %
- Bleu : commande en cours d'envoi (attente ACK)
- Cyan : en-tête
- Jaune : mode NAV_HOLD

---

## Fréquences et timings

| Élément | Valeur |
|---------|--------|
| Loop principal | 10 Hz (100 ms) |
| Heartbeat automatique | toutes les 3 s |
| Tâche LoRa RX (Core 0) | 1 ms de pause entre écoutes |
| Timeout débogage série | toutes les 2 s |

---

## Logs série au démarrage

```
===========================================
  OpenSailingRC - Buoy Joystick v1.0
===========================================

1. Initialisation Joystick...
   -> Joystick: OK

2. Initialisation LoRa 920...
   -> LoRa 920: OK

2b. Initialisation ESP-NOW (écoute passive)...
   -> ESP-NOW passif: OK (réception broadcasts bouées)

3. Attente découverte automatique des bouées...
4. Configuration du mode de communication...
   -> Mode: LoRa 920 MHz
5. Initialisation BuoyStateManager...
6. Initialisation Display...
7. Création tâche LoRa RX sur Core 0...

===========================================
  SYSTEM READY
===========================================
```

---

## Dépannage

| Symptôme | Cause probable | Solution |
|----------|---------------|----------|
| `ERREUR: Echec initialisation joystick` | STM32 ne répond pas sur I2C | Vérifier câblage SDA=38, SCL=39 ; alimentation STM32 |
| `ERREUR CRITIQUE: Echec initialisation LoRa` | Module non connecté ou RX/TX inversés | Vérifier câblage G1/G2 ; essayer LORA_MODE_CONFIGURATION |
| Bouées jamais détectées en ESP-NOW | Bouée hors portée ou ne diffuse pas | Vérifier firmware bouée ; distance < 100 m |
| Bouées jamais détectées en LoRa | Adresse ou canal non correspondants | Vérifier `LORA_ADDRESS_L`, `LORA_CHANNEL` dans LoRaCommunication.h |
| Écran noir | Display non initialisé | Vérifier `M5.begin()` ; vérifier alimentation |

---

## Documentation complémentaire

- [ARCHITECTURE.md](ARCHITECTURE.md) — Vue détaillée de l'architecture
- [API_DOCUMENTATION.md](API_DOCUMENTATION.md) — Documentation des APIs publiques
- [LORA_INTEGRATION_GUIDE.md](LORA_INTEGRATION_GUIDE.md) — Guide d'intégration LoRa
- [LORA_TROUBLESHOOTING.md](LORA_TROUBLESHOOTING.md) — Résolution des problèmes LoRa
- [DYNAMIC_BUOY_DISCOVERY.md](DYNAMIC_BUOY_DISCOVERY.md) — Découverte automatique des bouées
- [BUOY_ESPNOW_CONFIG.md](BUOY_ESPNOW_CONFIG.md) — Configuration ESP-NOW côté bouée

---

## Spécifications techniques

| Paramètre | Valeur |
|-----------|--------|
| Microcontrôleur | ESP32-S3 (M5Stack Atom S3) |
| Écran | LCD 128×128 px |
| Interface joystick | I2C, STM32F030F4P6 @ 0x59 |
| Modules radio | ESP-NOW 2.4 GHz / LoRa 920 MHz |
| Bouées supportées simultanément | 8 (constante `MAX_BUOYS`) |
| Framework | Arduino (PlatformIO, espressif32 @ 6.5.0) |

---

## Licence

Ce projet est distribué sous la licence **GNU General Public License v3.0**.

- Texte complet de la licence : **[LICENSE](LICENSE)**
- Référence officielle : <https://www.gnu.org/licenses/gpl-3.0.html>
