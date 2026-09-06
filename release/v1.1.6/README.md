# Release 1.1.6 — Buoy Joystick (v1)

Version **gelée** du 6 septembre 2026. Met le joystick v1 au débit air commun et corrige
deux défauts de réception.

## Débit air 2,4 → 9,6 kbps

`getAirDataRate()` renvoyait `0b010` (2,4 kbps) **en dur**. Le joystick v1 était le seul
équipement resté à l'ancien débit : il ne pouvait plus dialoguer avec les bouées une fois
celles-ci migrées.

Le `#define LORA_AIR_DATA_RATE` du header n'est **pas utilisé** en 433 et portait une
valeur trompeuse ; il est corrigé et documenté comme tel.

## ACK concaténés perdus

La bouée émet chaque ACK **deux fois**, et le E220 les livre régulièrement concaténés —
buffers de 37 octets couramment observés. Le test d'égalité stricte
`recv_data_len == sizeof(AckWithStatePacketLora)` échouait alors et **les deux copies
étaient perdues**, provoquant une réémission de commande et un acquittement jamais affiché.

Le buffer est désormais parcouru trame par trame, en relisant le `messageType`.

## Mutex LoRa relâché avant traitement

`loraMutex` était tenu pendant tout le traitement, `processAck()` compris. L'envoi de
commande n'attend le mutex que 50 ms : au-delà il abandonne et l'opérateur voit
`Echec envoi commande`.

**Une commande de sécurité comme `NAV_STOP` ne doit pas échouer parce qu'un ACK était en
cours de décodage.** Le mutex ne protège plus que l'accès au module.

## Diagnostic de mise en service

Mode réel du module annoncé en clair au démarrage, et trace explicite quand la
configuration n'a pas pu être écrite.

## Réglages radio figés dans ce binaire

| Paramètre | Valeur |
|---|---|
| Bande / canal | 433,125 MHz — canal 23 |
| Débit air | **9,6 kbps** (SF7/BW125), REG0 `0b100` |
| Puissance | registre `0b11` = 10 dBm (limite ISM 433 européenne) |
| UART module | 9600 bauds |

## ⚠️ Mise en service — le reflashage ne suffit pas

Le E220 conserve sa configuration **dans sa propre mémoire**. Flasher ce binaire ne change
ni le débit air, ni le canal, ni la puissance : il faut les réécrire une fois.

1. Flasher le binaire fusionné à l'adresse **0x0**.
2. Switch M0/M1 sur **ON**, mettre sous tension. Vérifier dans la trace :
   `MODULE EN MODE CONFIGURATION` **puis** `Module E220-JP configured successfully!`
3. **Couper l'alimentation.** Un reset ne suffit pas — le firmware ne coupe jamais
   l'alimentation du module.
4. Switch M0/M1 sur **OFF**, remettre sous tension. Vérifier :
   `MODULE EN MODE NORMAL — liaison radio active.`
5. **Vérifier qu'une commande passe réellement.** C'est la seule preuve que les
   équipements sont au même débit.

À l'étape 4, le message `configuration non ecrite a ce demarrage` est **normal** : avec le
switch sur OFF l'écriture échoue toujours, et le module utilise ce qui a été écrit à
l'étape 2.

> Un débit air discordant entre deux équipements coupe la liaison **sans aucun message
> d'erreur**. C'est le mode de panne le plus coûteux de cet écosystème.

## Contenu

| Fichier | Usage |
|---|---|
| `OpenSailingRC_BuoyJoystick_v1.1.6_AtomS3_MERGED.bin` | Binaire complet — bootloader + partitions + application, à flasher à l'adresse **0x0** |

Paramètres flash : QIO, 80 MHz, 8 MB. Bootloader à 0x0 dans l'image (AtomS3).

```
sha256  508eaa8f7111187a97ef306206394ec221a1a95935b6650ecbf2ddb11d41f2e4
taille  952992 octets
```

> **M5Burner flashe toujours à l'adresse 0x0** : c'est ce qui impose le binaire fusionné.
> Les préférences NVS étant écrasées, la sauvegarde SD introduite en 1.1.0 les restaure au
> démarrage suivant.
