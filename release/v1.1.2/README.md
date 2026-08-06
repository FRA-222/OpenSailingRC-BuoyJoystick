# Release 1.1.2 — OpenSailingRC Buoy Joystick (AtomS3)

## Contenu

- `OpenSailingRC_BuoyJoystick_v1.1.2_AtomS3_MERGED.bin` — binaire complet (bootloader + partitions + application), à flasher à l'adresse **0x0**. Paramètres flash : QIO, 80 MHz, 8 MB.

## ⚠️ Configuration figée dans ce binaire

Le mode de communication est choisi à la compilation (`src/main.cpp`) et **n'est pas
modifiable après flashage** :

| Constante | Valeur dans ce binaire |
|---|---|
| `COMM_MODE` | **`CommMode::LORA_433`** — module E220-400T22S, canal 23, 433.125 MHz |

**La bouée doit être réglée sur la même bande (433 MHz)**, sinon la liaison est
silencieusement inexistante — aucun message d'erreur, simplement aucune trame reçue.
Le binaire correspondant côté bouée est la release **1.1.2** de
`Autonomous-GPS-Buoy`, également compilée en 433 MHz.

Pour un joystick équipé du module 920 MHz, recompiler avec
`#define COMM_MODE CommMode::LORA_920`.

## Nouveautés 1.1.2

### Support bi-bande LoRa

- Le firmware pilote au choix un module E220-900T22S(JP) à 920 MHz ou un E220-400T22S à
  433 MHz, sélectionné par `COMM_MODE`. Le module se remplace sur le même port, sans
  changement de brochage ni d'UART.
- L'énumération `CommMode` passe de `{ ESP_NOW, LORA }` à `{ ESP_NOW, LORA_920, LORA_433 }`.
  Tout le protocole est commun aux deux bandes ; seuls le canal, le débit air et la
  puissance en dépendent.
- **Puissance d'émission dépendante de la bande.** L'énumération `TX_POWER_*` de la
  bibliothèque est étiquetée pour le variant japonais : `0b00` y vaut 13 dBm (maximum ARIB)
  mais sélectionne **22 dBm** sur un E220-400T22S. Le firmware écrit désormais `0b11`
  (10 dBm) en 433, compatible avec la limite de 10 mW de l'ISM européen.
- **Débit air dépendant de la bande — correctif bloquant.** Le registre REG0 n'a pas la
  même structure selon la variante : sur un module 433, les bits [4:3] portent la **parité
  UART**. La valeur `BW125K_SF9` (`0b10000`) du variant JP y mettait la parité à 8E1 alors
  que l'ESP32 émet en 8N1 : le module devenait sourd dès la fin de sa configuration, aucune
  trame ne passait. Le firmware écrit `0b010` en 433 (8N1 + 2.4 kbps), soit la même
  modulation physique SF9/BW125 qu'en 920.
- **Traces de configuration LoRa au démarrage** : canal, fréquence, REG0 et puissance sont
  affichés explicitement, ce qui permet de comparer les deux extrémités de la liaison d'un
  coup d'œil. Une erreur de registre est autrement silencieuse — configuration acceptée par
  le module, aucun message d'erreur, et zéro trame reçue.
- Le libellé de source affiché à l'écran suit la bande (`LoRa 433` / `LoRa 920`).

### Affichage

- **Suppression du clignotement.** Deux causes cumulées : un `forceRefresh()` (donc un
  effacement complet de l'écran) déclenché à chaque ACK LoRa reçu, et un cache d'affichage
  dont les comparaisons portaient sur des valeurs brutes — un `float` comparé à un `uint8_t`
  provoquait un redessin permanent. Le rendu est désormais incrémental : chaque champ n'est
  repeint que si le **texte affiché** change, et l'effacement se fait par `setTextPadding()`
  au lieu d'un `fillRect` suivi d'un dessin, ce qui supprime le flash noir.
- **Correction : la température de la bouée ne s'affichait pas.** Elle était formatée avec
  `"%d%C"` alors que le champ est un `float`, avec en prime un second argument manquant.

## Premier démarrage après changement de bande

Le module E220 mémorise sa configuration radio. Après un changement de bande :

1. Placer le switch M0/M1 du module sur **ON** (mode configuration) et démarrer le joystick.
2. Vérifier dans les traces `# LoRa: Module E220-JP configured successfully!` ainsi que le
   canal, la fréquence, le REG0 et la puissance attendus — et les comparer à ceux affichés
   par la bouée.
3. Repasser le switch sur **OFF** et redémarrer. Un module laissé en mode configuration
   n'émet pas.

En mode configuration, le module force son UART à 9600 8N1 quelle que soit sa configuration
enregistrée : un module déjà mal configuré reste donc reprogrammable.

## Installation

```bash
# Mise à jour application seule
platformio run --environment m5stack-atoms3 --target upload

# Installation complète (équivalent M5Burner)
esptool.py --chip esp32s3 write_flash 0x0 OpenSailingRC_BuoyJoystick_v1.1.2_AtomS3_MERGED.bin
```
