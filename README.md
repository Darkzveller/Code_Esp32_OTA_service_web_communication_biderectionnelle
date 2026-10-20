# ESP32 maitre - OTA + console Web Wi-Fi (sans Telnet)

Projet PlatformIO ESP32 / Arduino. Le `main.cpp` contient un exemple de reception des commandes saisies dans le navigateur :

```cpp
#include <Arduino.h>
#include "OTA.h"
void setup() { Serial.begin(115200); setupOTA(); }
void loop() { String cmd; if (otaReadCommand(cmd)) otaLog("Recu : " + cmd); delay(10); }
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
- Commandes integrees : `help`, `status`, `ip`, `heap`, `uptime`, `clear`, `restart`.
- Les autres commandes (`bonjour`, `moteur_on`, `moteur_off`, etc.) sont ajoutees a une file FreeRTOS et recuperees dans `main.cpp` par `otaReadCommand(commande)`.
- File de 10 commandes maximum ; si elle est pleine, la requete recoit une erreur HTTP 503. Les commandes ont une longueur limitee a 64 octets.
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

## Recuperer les commandes dans `main.cpp`

Par exemple, saisir `bonjour` dans la barre de commande de la console Web :

```cpp
void loop()
{
    String commande;
    if (otaReadCommand(commande))
    {
        otaLog("Recu : " + commande);
        if (commande == "bonjour")
            otaLog("Salut depuis l'ESP32 !");
    }
    delay(10);
}
```

`otaReadCommand()` est non bloquant et consomme une commande par appel.
Les commandes systeme (`status`, `restart`, etc.) sont gerees directement par `OTA.cpp` ; elles ne sont pas envoyees au `main.cpp`.
Les commandes utilisateur conservent leur casse ; par exemple `Bonjour` est different de `bonjour`.

Si l'application a beaucoup de traitements, appeler regulierement `otaReadCommand()` dans
`loop()` pour eviter de remplir la file. La boucle reste libre pour le reste du programme.
