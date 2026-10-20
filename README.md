# ESP32 maitre - OTA + console Web Wi-Fi (sans Telnet)

Projet PlatformIO ESP32 / Arduino. Le `main.cpp` reste minimal :

```cpp
#include <Arduino.h>
#include "OTA.h"
void setup() { Serial.begin(115200); setupOTA(); }
void loop() {}
```

## Demarrage

1. Ouvrir ce dossier dans VS Code avec PlatformIO.
2. Verifier `include/wifi_credentials.h` : les identifiants Wi-Fi du projet precedent y sont conserves.
3. Ouvrir `include/terminal_credentials.h` pour connaitre / personnaliser le compte du terminal.
4. Televerser une premiere fois le firmware par **USB**.
5. Ouvrir le moniteur serie a **115200 bauds**. Une ligne affiche l'URL `http://IP_DE_L_ESP32/`.
6. Sur un ordinateur / telephone **du meme reseau Wi-Fi**, ouvrir cette URL dans un navigateur.
7. A la demande de connexion, saisir l'utilisateur et le mot de passe du fichier `terminal_credentials.h`.

## Fonctions du terminal

- Affichage des derniers 8 Kio de journaux de `otaLog()` ; actualisation toutes les secondes.
- Reconnexion automatique de la page apres une coupure temporaire du Wi-Fi.
- Commandes : `help`, `status`, `ip`, `heap`, `uptime`, `clear`, `restart`.
- La sortie est dupliquee sur le moniteur serie USB pour chaque `otaLog()`.

**Important :** `Serial.print()` et `Serial.println()` ecrits ailleurs dans votre programme ne
sont **pas** automatiquement copies dans la console Wi-Fi. Pour rendre les futurs messages visibles
des deux cotes, utiliser par exemple :

```cpp
otaLog("Capteur initialise");
otaLog("Valeur=" + String(valeur));
```

## OTA

Le service ArduinoOTA tourne dans sa propre tache FreeRTOS ; `loop()` peut rester vide.
Pour les mises a jour OTA, decommenter les options dans `platformio.ini` :

```ini
upload_protocol = espota
upload_port = espmaitre.local
```

Si la resolution `.local` ne fonctionne pas, remplacer le nom par l'IP affichee dans la console.
Le terminal Web et le service ArduinoOTA sont independants.

## Technique et securite

- La bibliotheque native `WebServer.h` est fournie avec Arduino ESP32 : **aucun lib_deps externe**.
- Les requetes de la page sont des requetes HTTP classiques (actualisation toutes les secondes) : **ce n'est pas Telnet ni un WebSocket**.
- Le compte du terminal est protege par HTTP Digest. **HTTP n'est pas chiffre** :
  il ne garantit ni la confidentialite du trafic, ni la securite sur un reseau hostile.
- Ne pas ouvrir le port 80 du terminal sur Internet. Pour un acces hors du reseau local,
  utiliser un VPN sur le reseau (routeur ou passerelle), sans redirection de port publique.
- Ne jamais publier `include/wifi_credentials.h` ni `include/terminal_credentials.h`.
- La connexion Wi-Fi initiale doit aboutir pour que les deux services soient lances.

## Structure

```text
include/
  wifi_credentials.h                  # prive, conserve du projet precedent
  wifi_credentials.example.h
  terminal_credentials.h              # prive, nouvel identifiant console
  terminal_credentials.example.h
lib/OTA/
  OTA.h
  OTA.cpp
  WebTerminalPage.h                   # interface web embarquee
src/main.cpp
platformio.ini
```
