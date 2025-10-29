# 🎮 OpenSailingRC BuoyJoystick - Ossature v1.0

## ✅ État d'Avancement

### Modules Implémentés

- [x] **JoystickManager** - Lecture I2C des joysticks et boutons
- [x] **ESPNowCommunication** - Communication bidirectionnelle ESP-NOW
- [x] **BuoyStateManager** - Gestion état des bouées
- [x] **DisplayManager** - Affichage LCD 128x128
- [x] **Logger** - Système de logging centralisé (série + LCD)
- [x] **main.cpp** - Loop principal avec intégration

### Fonctionnalités Opérationnelles

✅ Lecture des 2 joysticks (4 axes)  
✅ Lecture des 5 boutons (4 JoyC + 1 écran Atom S3)  
✅ Lecture batteries (2x)  
✅ Communication ESP-NOW bidirectionnelle  
✅ Découverte automatique des bouées via broadcast  
✅ Réception état des bouées (1Hz)  
✅ Envoi commandes aux bouées  
✅ Commande INIT_HOME (bouton jaune gauche)  
✅ Sélection bouée active (bouton jaune droit / écran)  
✅ Affichage LCD temps réel  
✅ Affichage modes général et navigation  
✅ Affichage batterie/GPS/Signal  
✅ Logs série unifiés via Logger  

### À Implémenter (Phase 2)

- [ ] **CommandManager** - Génération commandes depuis joystick
- [ ] Mapping Joystick → Commandes bouées
- [ ] Commande INIT_HOME (BTN_RIGHT_STICK)
- [ ] Commande SET_HEADING (Joystick droit X)
- [ ] Commande SET_THROTTLE (Joystick gauche Y)
- [ ] Commande HOLD (BTN_LEFT_STICK)
- [ ] Commande STOP/RETURN (BTN_M5)

---

## 🚀 Démarrage Rapide

### 1. Configuration des Adresses MAC

Dans `main.cpp`, remplacez les adresses MAC par celles de vos bouées :

```cpp
const uint8_t BUOY1_MAC[] = {0x48, 0xE7, 0x29, 0x9E, 0x2B, 0xAC};
const uint8_t BUOY2_MAC[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};
```

**Pour obtenir l'adresse MAC d'une bouée**, exécutez sur la bouée :
```cpp
WiFi.mode(WIFI_STA);
Serial.println(WiFi.macAddress());
```

### 2. Compilation

```bash
cd "/Users/philippe/Documents/PlatformIO/Projects/OpenSailingRC-BuoyJoystick"
platformio run
```

### 3. Upload

```bash
platformio run --target upload
```

### 4. Moniteur Série

```bash
platformio device monitor
```

---

## 📡 Communication ESP-NOW

### Structure CommandPacket (Joystick → Bouée)

```cpp
struct CommandPacket {
    uint8_t targetBuoyId;    // 0-4
    BuoyCommand command;     // Type de commande
    int16_t heading;         // Cap cible (-180 à +180°)
    int8_t throttle;         // Vitesse (-100 à +100%)
    NavigationMode mode;     // Mode de navigation
    uint32_t timestamp;      // Horodatage
};
```

### Structure BuoyState (Bouée → Joystick)

```cpp
struct BuoyState {
    uint8_t buoyId;          // ID de la bouée
    double latitude;         // Position GPS
    double longitude;
    float heading;           // Cap actuel
    float speed;             // Vitesse m/s
    NavigationMode mode;     // Mode actuel
    uint8_t batteryLevel;    // Batterie 0-100%
    int8_t signalQuality;    // Signal LTE (-1 si pas LTE)
    bool gpsLocked;          // GPS verrouillé
    uint32_t timestamp;      // Horodatage
};
```

### Fréquences

- **Réception état bouées**: 1 Hz (toutes les secondes)
- **Heartbeat test**: 5 secondes
- **Loop principal**: 10 Hz (100ms)
- **Mise à jour affichage**: 2 Hz (500ms)

---

## 🖥️ Affichage LCD

### Écran Principal

```
┌────────────────────────┐
│     Bouée #1           │ Header (cyan)
├────────────────────────┤
│   CONNECTE             │ État connexion (vert/rouge)
│                        │
│      CAP MODE          │ Mode navigation
│                        │
│   Cap: 045°            │ Informations navigation
│   2.5 m/s              │
│                        │
│ 🔋85%  🛰️GPS  📶LTE   │ Indicateurs
└────────────────────────┘
```

### Codes Couleurs

- **Vert** : Connecté, batterie >50%, mode normal
- **Orange** : Batterie 20-50%
- **Rouge** : Déconnecté, batterie <20%
- **Bleu** : Mode RETURN HOME
- **Cyan** : Header, mode WAYPOINT
- **Jaune** : Mode HOLD

---

## 🎮 Contrôles Actuels

### Boutons Fonctionnels

- **BTN_RIGHT** → Sélectionner bouée suivante
- **BTN_LEFT** → Sélectionner bouée précédente

### À Implémenter (Phase 2)

- **Joystick GAUCHE Y** → Throttle (vitesse)
- **Joystick DROIT X** → Cap (+/- 10°)
- **BTN_RIGHT_STICK** → INIT HOME
- **BTN_LEFT_STICK** → HOLD
- **BTN_M5 court** → STOP
- **BTN_M5 long** → RETURN HOME

---

## 🔧 Architecture Logicielle

```
main.cpp (Loop 10Hz)
    │
    ├─→ JoystickManager.update()
    │   └─→ Lit I2C (STM32 @ 0x59)
    │       ├─→ 4 axes joystick
    │       ├─→ 4 boutons
    │       └─→ 2 batteries
    │
    ├─→ BuoyStateManager.update()
    │   └─→ ESPNowCommunication
    │       ├─→ Reçoit BuoyState (1Hz)
    │       └─→ Envoie CommandPacket
    │
    └─→ DisplayManager.update()
        └─→ Affiche sur LCD 128x128
            ├─→ État connexion
            ├─→ Mode navigation
            ├─→ Cap / Vitesse
            └─→ Batterie / GPS / Signal
```

---

## 📊 Logs Série Attendus

### Au Démarrage

```
===========================================
  OpenSailingRC - Buoy Joystick v1.0
===========================================

1. Initialisation Joystick...
✓ JoystickManager: STM32 détecté sur I2C

2. Initialisation ESP-NOW...
✓ ESP-NOW: Adresse MAC locale: 48:E7:29:9E:2B:AC
✓ ESP-NOW: Initialisé

3. Enregistrement des bouées...
✓ ESP-NOW: Bouée #0 ajoutée - MAC: 48:E7:29:9E:2B:AC

✓ 1 bouée(s) enregistrée(s)

4. Initialisation BuoyStateManager...
✓ BuoyStateManager: Initialisé

5. Initialisation Display...
✓ DisplayManager: Initialisé

===========================================
  SYSTÈME PRÊT - En attente bouées
===========================================
```

### En Fonctionnement

```
--- État système ---
Joystick L: X=-12 Y=234
Joystick R: X=0 Y=-5
Batteries: 4.15V / 4.18V
Bouée sélectionnée: #0
Bouées connectées: 1/1
  Mode: CAP
  Cap: 45° Vitesse: 2.3 m/s
  GPS: OK Batterie: 87%
-------------------

→ Heartbeat envoyé à Bouée #0
← État reçu de Bouée #0 (mode=1, bat=87%, GPS=OK)
```

### Changement de Bouée

```
→ Bouton DROIT pressé
→ Bouée sélectionnée: #1
```

---

## 🧪 Tests à Effectuer

### Test 1: Joystick (Sans Bouée)

1. Compiler et flasher le joystick
2. Ouvrir le moniteur série
3. Vérifier que les axes et boutons sont lus correctement
4. Bouger les joysticks → Valeurs changent
5. Presser les boutons → Détection OK

**Résultat attendu**: ✅ Valeurs affichées toutes les 2s

### Test 2: ESP-NOW (Avec Bouée)

1. Flasher la bouée avec firmware ESP-NOW compatible
2. Configurer MAC de la bouée dans `main.cpp`
3. Recompiler et flasher le joystick
4. Vérifier logs série

**Résultat attendu**:
- ✅ Bouée ajoutée au démarrage
- ✅ État reçu toutes les secondes
- ✅ Heartbeat envoyé toutes les 5s

### Test 3: Affichage LCD

1. Avec bouée connectée
2. Observer l'écran LCD
3. Vérifier affichage du mode, cap, vitesse
4. Vérifier icônes batterie/GPS/signal

**Résultat attendu**: ✅ Écran mis à jour toutes les 500ms

### Test 4: Sélection Bouée

1. Ajouter 2 bouées minimum
2. Presser BTN_LEFT et BTN_RIGHT
3. Observer changement sur écran et série

**Résultat attendu**: ✅ Navigation entre bouées

---

## ⚠️ Dépannage

### Problème: "Échec initialisation joystick"

**Cause**: STM32 ne répond pas sur I2C

**Solution**:
1. Vérifier connexion I2C (SDA=38, SCL=39)
2. Vérifier alimentation du STM32
3. Scanner I2C avec `i2cdetect` (devrait trouver 0x59)

### Problème: "Échec initialisation ESP-NOW"

**Cause**: WiFi non configuré correctement

**Solution**:
1. Vérifier que `WiFi.mode(WIFI_STA)` est appelé
2. Redémarrer l'ESP32
3. Vérifier version du firmware ESP32

### Problème: Pas de données reçues des bouées

**Cause**: Bouée non configurée ou MAC incorrecte

**Solution**:
1. Vérifier adresse MAC de la bouée
2. S'assurer que la bouée envoie des paquets `BuoyState`
3. Vérifier que la bouée est sous tension
4. Vérifier portée ESP-NOW (<100m)

### Problème: Écran noir

**Cause**: LCD non initialisé

**Solution**:
1. Vérifier que `M5.begin(true, ...)` est appelé
2. Vérifier alimentation
3. Essayer `display.setBrightness(255)`

---

## 📈 Prochaines Étapes (Phase 2)

1. **Implémenter CommandManager**
   - Création classe et méthodes
   - Mapping joystick → commandes

2. **Intégrer les commandes dans main.cpp**
   - Lecture joysticks
   - Génération commandes
   - Envoi via ESP-NOW

3. **Tester chaque commande**
   - INIT_HOME
   - SET_HEADING
   - SET_THROTTLE
   - HOLD
   - RETURN
   - STOP

4. **Affiner l'affichage**
   - Feedback visuel des commandes
   - Indicateur d'activité joystick
   - Menu de configuration

5. **Optimisations**
   - Zone morte joystick
   - Filtrage commandes redondantes
   - Gestion erreurs robuste

---

**Version**: 1.0 - Ossature  
**Date**: 26 octobre 2025  
**Auteur**: Philippe Hubert  
**Status**: ✅ Prêt pour tests Phase 1
