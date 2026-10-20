# Carte maître ESP32 — Mise à jour OTA et console Web

## I. Présentation du projet

Ce projet a pour objectif de mettre en place une communication Wi-Fi avec une carte ESP32 afin de pouvoir consulter les informations de son programme et intervenir dessus sans devoir systématiquement utiliser une connexion USB. Le développement est réalisé avec Visual Studio Code et PlatformIO, en utilisant le framework Arduino.

Dans un premier temps, j'ai souhaité conserver uniquement la partie OTA (*Over-The-Air*) du projet initial. L'OTA permet de téléverser un nouveau programme sur l'ESP32 à travers le réseau Wi-Fi. Cela évite d'avoir à raccorder la carte à l'ordinateur à chaque modification, notamment lorsque celle-ci est intégrée dans un système plus important.

Par la suite, j'ai ajouté une console accessible depuis un navigateur Web afin de pouvoir afficher les valeurs du programme et envoyer des commandes à l'ESP32. J'ai choisi de séparer cette partie du programme principal : la gestion du Wi-Fi, de l'OTA et de l'interface Web se trouve dans la bibliothèque `OTA`, tandis que le fichier `main.cpp` reste disponible pour le fonctionnement de l'application.

## II. Passage de Telnet à une interface Web

Dans une première version du projet, j'utilisais Telnet pour accéder à une console distante. Cependant, lors de l'utilisation, j'avais parfois des problèmes de connexion. J'ai donc souhaité passer à une page Web pour des raisons de simplicité d'utilisation.

Ce choix présente également un avantage concernant la saisie des commandes. Avec la page Web, je peux écrire une commande dans un champ de texte, vérifier ce que j'ai réellement tapé, puis seulement ensuite appuyer sur **Envoyer**. Cela évite notamment d'envoyer une commande contenant une erreur de saisie. La commande envoyée apparaît ensuite dans les journaux avec le symbole `>` devant celle-ci.

La page est directement hébergée sur l'ESP32. Elle est composée d'une partie HTML pour la structure, de CSS pour l'affichage et de JavaScript pour les échanges avec le programme. Il n'est donc pas nécessaire d'installer un logiciel de terminal sur l'ordinateur : il suffit d'ouvrir l'adresse IP de la carte dans un navigateur, depuis le même réseau.

Contrairement à une communication Telnet, l'interface utilise des requêtes HTTP. Les messages affichés dans la console sont récupérés périodiquement, toutes les secondes. Lorsque la connexion est temporairement interrompue, la page poursuit ses tentatives de récupération des messages.

## III. Organisation des fichiers

Pour éviter de mélanger le programme principal et les fonctions de communication, j'ai organisé les fichiers de la manière suivante :

```text
Carte_Maitre_ESP32E_OTA/
├── include/
│   ├── wifi_credentials.h
│   └── terminal_credentials.h
├── lib/
│   └── OTA/
│       ├── OTA.h
│       ├── OTA.cpp
│       └── WebTerminalPage.h
├── src/
│   └── main.cpp
├── platformio.ini
└── README.md
```

Le fichier `main.cpp` contient le code exécuté par l'application. Le fichier `OTA.h` déclare les fonctions qui peuvent être appelées depuis ce programme, et `OTA.cpp` contient leur implémentation ainsi que les fonctions internes de communication. Enfin, `WebTerminalPage.h` contient la page Web embarquée directement dans la mémoire flash de l'ESP32.

Les fichiers `wifi_credentials.h` et `terminal_credentials.h` regroupent respectivement les identifiants du réseau Wi-Fi et ceux de la console Web. La séparation permet de modifier ces informations sans avoir à les écrire directement dans le code de la bibliothèque.

## IV. Initialisation du programme

### A. Le fichier `main.cpp`

L'initialisation nécessite seulement les instructions suivantes :

```cpp
#include <Arduino.h>
#include "OTA.h"

void setup()
{
    Serial.begin(115200);
    setupOTA();
}

void loop()
{
}
```

L'instruction `Serial.begin(115200)` initialise la liaison série USB avec un débit de 115200 bauds. La fonction `setupOTA()` initialise ensuite le Wi-Fi, l'OTA et la console Web. La boucle `loop()` peut rester vide, puisque la gestion du réseau est déjà assurée par des tâches FreeRTOS créées dans la bibliothèque.

Dans la version fournie du projet, `main.cpp` contient également des exemples de variables et de commandes afin de montrer comment exploiter la console. Ces exemples peuvent être remplacés par le code de l'application.

### B. Le fichier `platformio.ini`

La configuration actuelle est la suivante :

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags =
    -I include
```

L'environnement `esp32dev` indique la carte cible. `monitor_speed` définit la vitesse du moniteur série. L'option `-I include` permet aux fichiers de la bibliothèque `OTA` d'accéder aux fichiers d'en-tête présents dans le dossier `include`, notamment ceux contenant les identifiants Wi-Fi et Web.

Pour la première installation, je téléverse le programme en USB. Une fois la carte connectée au réseau, les téléversements suivants peuvent être réalisés par Wi-Fi en activant les lignes suivantes dans `platformio.ini` :

```ini
upload_protocol = espota
upload_port = espmaitre.local
```

Si le nom `espmaitre.local` n'est pas résolu sur le réseau, il est également possible de renseigner l'adresse IP de l'ESP32 comme valeur de `upload_port`.

## V. Fonctionnement des fonctions de la bibliothèque OTA

### A. `setupOTA()` : démarrage des services

Cette fonction se trouve dans `OTA.cpp` et constitue le point d'entrée principal de la bibliothèque. Elle est appelée une fois depuis `setup()`.

Dans un premier temps, elle crée un mutex pour protéger les messages enregistrés dans la console, puis une file FreeRTOS permettant de stocker les commandes reçues depuis le navigateur. Elle configure ensuite l'ESP32 en mode station Wi-Fi avec `WiFi.mode(WIFI_STA)` et démarre la connexion avec `WiFi.begin(WIFI_SSID, WIFI_PASSWORD)`.

Une fois la connexion établie, `setupOTA()` configure ArduinoOTA, démarre le serveur Web et crée les deux tâches chargées de faire fonctionner ces services. Dans le code actuel, l'attente de la connexion Wi-Fi se fait dans une boucle : tant que la connexion n'est pas établie, l'exécution reste dans `setupOTA()`.

Les fonctions `ArduinoOTA.onStart()`, `onEnd()`, `onProgress()` et `onError()` sont également configurées à cet endroit. Elles servent respectivement à afficher le début d'une mise à jour, sa fin, sa progression et les éventuelles erreurs. Les messages de progression sont espacés par paliers afin de ne pas remplir la console avec un affichage à chaque pourcentage.

### B. `otaLog()` : affichage des informations

La fonction `otaLog(const String& message)` permet d'afficher un message à la fois dans le moniteur série USB et dans la console Web.

Par exemple, dans `main.cpp` :

```cpp
int compteur = 12;
float temperature = 25.5;
bool moteurActif = true;

otaLog("Compteur : " + String(compteur));
otaLog("Temperature : " + String(temperature, 2) + " C");
otaLog("Moteur : " + String(moteurActif ? "ON" : "OFF"));
```

Le résultat obtenu dans la console est de la forme :

```text
[8 s] Compteur : 12
[8 s] Temperature : 25.50 C
[8 s] Moteur : ON
```

Dans le premier cas, `String(compteur)` permet de convertir l'entier en texte pour pouvoir l'ajouter au message. Pour la température, le second paramètre de `String(temperature, 2)` indique que je souhaite afficher deux chiffres après la virgule. Enfin, l'opérateur conditionnel `? :` permet d'afficher `ON` lorsque le booléen vaut `true`, et `OFF` dans le cas contraire.

À l'intérieur de `otaLog()`, le message est d'abord envoyé par `Serial.println()`, puis ajouté à une chaîne contenant l'historique destiné au navigateur. Le temps écoulé depuis le démarrage est ajouté devant chaque ligne. Le mutex évite que plusieurs tâches modifient simultanément cet historique.

La taille de cet historique est limitée à environ 8 Ko : lorsque la limite est dépassée, les premières lignes sont supprimées pour conserver les messages les plus récents. Les appels directs à `Serial.print()` ou `Serial.println()` dans le reste du programme ne sont pas automatiquement repris dans la console Web. Pour afficher une information aux deux endroits, j'utilise donc `otaLog()`.

### C. `otaReadCommand()` : récupération d'une commande

La fonction `otaReadCommand(String& command)` permet de récupérer, dans `main.cpp`, les commandes personnalisées envoyées depuis la page Web.

Elle renvoie un booléen. Si une commande est disponible, elle place cette commande dans la variable passée en argument et renvoie `true`. Dans le cas contraire, elle renvoie `false`. La lecture est non bloquante : si aucune commande n'est présente, le programme peut continuer immédiatement.

Voici un exemple d'utilisation :

```cpp
void loop()
{
    String commande;

    if (otaReadCommand(commande))
    {
        otaLog("Commande recue : " + commande);

        if (commande == "bonjour")
        {
            otaLog("Salut depuis l'ESP32 !");
        }
        else if (commande == "moteur_on")
        {
            otaLog("Activation du moteur");
        }
        else if (commande == "moteur_off")
        {
            otaLog("Arret du moteur");
        }
    }
}
```

Par exemple, lorsque je saisis `bonjour` sur la page Web puis que j'appuie sur **Envoyer**, le texte est transmis à l'ESP32. La fonction `otaReadCommand()` récupère cette chaîne, puis la condition `commande == "bonjour"` permet d'exécuter la partie du programme correspondante.

Dans cet exemple, les commandes `moteur_on` et `moteur_off` affichent uniquement des messages. Elles ne commandent pas réellement un moteur : les instructions de pilotage devront être ajoutées à cet emplacement lorsque le matériel concerné sera intégré.

Les commandes personnelles sont sensibles aux majuscules et minuscules : `bonjour` et `Bonjour` sont deux chaînes différentes. Chaque appel à `otaReadCommand()` récupère au maximum une commande de la file.

### D. `requireAuth()` : authentification de la console

Cette fonction vérifie les identifiants fournis lors de l'ouverture de la page Web. Si l'authentification est correcte, elle renvoie `true`. Sinon, elle demande au navigateur de présenter une authentification HTTP Digest et renvoie `false`.

Les identifiants sont définis dans `include/terminal_credentials.h`. L'authentification évite que n'importe quel utilisateur présent sur le réseau puisse utiliser directement la console. La communication restant en HTTP, elle n'est cependant pas chiffrée.

### E. `setNoCache()` : gestion du cache HTTP

Cette fonction ajoute l'en-tête `Cache-Control: no-store, max-age=0` aux réponses HTTP. Son objectif est d'éviter que le navigateur réutilise une ancienne réponse alors que de nouveaux messages ou un changement d'état sont disponibles sur l'ESP32.

### F. `handleLogs()` : récupération des messages

Cette fonction est appelée lorsque le navigateur effectue une requête HTTP `GET` sur `/api/logs`.

Elle commence par vérifier l'authentification, puis récupère l'historique des messages. Pour éviter de renvoyer inutilement le même contenu à chaque actualisation, la bibliothèque utilise une variable `logRevision`, qui augmente lorsqu'un message est ajouté ou que les messages sont effacés.

Si la révision connue du navigateur est toujours la bonne, l'ESP32 répond avec le code HTTP `204`, ce qui signifie qu'il n'y a pas de nouveau contenu à afficher. Dans le cas contraire, il retourne le texte des messages avec un code `200` et la nouvelle révision. Cela permet de limiter les données échangées sur le réseau.

### G. `clearLogs()` : effacement de la console

Cette fonction vide la chaîne contenant l'historique des messages destinés à la page Web et incrémente `logRevision` pour que le navigateur soit informé du changement.

Elle est notamment appelée avec la commande `clear` ou avec le bouton **Effacer** de l'interface. Elle n'efface pas les messages qui ont déjà été affichés dans le moniteur série USB.

### H. `handleCommand()` : traitement des commandes reçues

Lorsqu'une commande est envoyée, la page effectue une requête HTTP `POST` sur `/api/command`. La fonction `handleCommand()` récupère le paramètre `cmd`, supprime les espaces présents au début et à la fin, puis vérifie sa longueur.

Si la commande correspond à une fonction intégrée, elle est exécutée directement dans `OTA.cpp`. Sinon, elle est copiée dans une file FreeRTOS afin de pouvoir être récupérée ultérieurement dans `main.cpp` avec `otaReadCommand()`.

Par exemple, les commandes intégrées sont :

| Commande | Fonctionnement |
| --- | --- |
| `help` | Affiche les commandes disponibles. |
| `status` | Affiche la connexion Wi-Fi, l'adresse IP, le signal, la mémoire libre et le temps de fonctionnement. |
| `ip` | Affiche l'adresse IP de l'ESP32. |
| `heap` | Affiche la quantité de mémoire dynamique libre. |
| `uptime` | Affiche le temps écoulé depuis le démarrage. |
| `clear` | Efface l'historique de la console Web. |
| `restart` | Demande un redémarrage de l'ESP32. |

Les alias `aide` et `etat` sont également reconnus. Les commandes intégrées sont comparées sans tenir compte de leur casse. Elles sont exécutées par la bibliothèque et ne sont donc pas transmises au `main.cpp`.

La longueur d'une commande est limitée à 64 octets, et la file peut contenir 10 commandes en attente. Si cette file est pleine, la requête reçoit une erreur HTTP `503`. Une commande vide ou trop longue entraîne une réponse `400`.

### I. `setupWebServer()` : création des routes HTTP

Cette fonction définit les adresses auxquelles le serveur Web doit répondre. Elle associe chaque route à une fonction précise :

```cpp
webServer.on("/api/logs", HTTP_GET, handleLogs);
webServer.on("/api/command", HTTP_POST, handleCommand);
```

La première ligne indique qu'une requête `GET /api/logs` doit déclencher `handleLogs()`. La seconde indique qu'une requête `POST /api/command` doit déclencher `handleCommand()`.

La route `/` renvoie le contenu HTML défini dans `WebTerminalPage.h`. Si aucune route ne correspond à la demande, le serveur retourne une erreur `404`. Enfin, `webServer.begin()` démarre l'écoute sur le port HTTP 80.

### J. `otaTask()` : gestion des mises à jour OTA

La fonction `otaTask(void* parameter)` fonctionne dans une boucle FreeRTOS indépendante de `loop()`. Elle appelle régulièrement `ArduinoOTA.handle()` pour traiter les demandes de mise à jour, puis attend environ 20 ms avec `vTaskDelay()`.

Cette organisation permet de conserver le traitement OTA même lorsque le fichier `main.cpp` ne contient aucun appel à `ArduinoOTA.handle()`.

### K. `webTask()` : gestion du serveur Web et du Wi-Fi

Cette fonction constitue la tâche chargée de recevoir et traiter les requêtes Web. Elle vérifie régulièrement la connexion Wi-Fi et appelle `webServer.handleClient()` lorsque le réseau est disponible.

Si la connexion Wi-Fi est perdue, une nouvelle tentative est effectuée à intervalles de 15 secondes. Lorsque la connexion revient, le serveur Web est relancé et un message indique sa nouvelle adresse. La tâche vérifie également si un redémarrage a été demandé avec la commande `restart`, puis appelle `ESP.restart()` au moment prévu.

Elle utilise une temporisation d'environ 10 ms à chaque passage dans la boucle, afin de laisser du temps d'exécution aux autres tâches du système.

## VI. Fonctionnement de la page Web

L'interface se trouve dans `WebTerminalPage.h`, sous la forme d'une chaîne HTML stockée en mémoire flash avec `PROGMEM`. Le programme utilise `webServer.send_P()` pour renvoyer cette page au navigateur, sans avoir besoin de créer un système de fichiers SPIFFS ou LittleFS.

La page comprend une zone d'affichage des messages, un champ de saisie, un bouton d'envoi ainsi que plusieurs boutons pour les commandes les plus utilisées.

### A. La fonction JavaScript `poll()`

Elle réalise une requête sur `/api/logs?rev=...` pour vérifier si de nouveaux messages sont disponibles. La variable `revision` contient le numéro de version du dernier historique reçu. En cas de nouveaux messages, la zone d'affichage est mise à jour. La fonction est relancée automatiquement après une seconde.

### B. La fonction JavaScript `sendCommand(cmd)`

Elle envoie une commande à l'ESP32 par HTTP POST en utilisant le paramètre `cmd`.

Par exemple, si je saisis `bonjour`, la page transmet l'équivalent de la requête suivante :

```http
POST /api/command HTTP/1.1
Content-Type: application/x-www-form-urlencoded

cmd=bonjour
```

Le serveur récupère ensuite cette commande dans `handleCommand()`. Si elle n'est pas intégrée à la bibliothèque, elle est placée dans la file FreeRTOS pour être traitée par `main.cpp`.

### C. Les fonctions JavaScript `connected()` et `reboot()`

`connected()` met à jour l'indicateur de connexion affiché en haut de la page, en fonction du résultat des requêtes HTTP.

`reboot()` demande une confirmation avant d'envoyer la commande `restart`. Cela évite de redémarrer la carte à la suite d'un simple clic involontaire.

Enfin, lorsque je valide le formulaire avec la touche Entrée ou le bouton **Envoyer**, le JavaScript récupère le texte du champ de saisie, puis appelle `sendCommand()`. Le champ peut donc être relu et corrigé avant l'envoi.

## VII. Communication entre les différentes parties

Le fonctionnement général repose sur deux sens de communication.

Dans le premier sens, le programme de l'ESP32 envoie des informations vers la console avec `otaLog()`. Les messages sont conservés temporairement en mémoire, puis consultés par le navigateur grâce à `GET /api/logs`.

Dans le second sens, je saisis une commande dans le navigateur. La page l'envoie avec `POST /api/command`. La bibliothèque la traite directement si elle correspond à une commande système ; autrement, elle la place dans une file d'attente. Le programme principal peut alors la récupérer avec `otaReadCommand()`.

```text
             Ordinateur / navigateur
                      |
            Wi-Fi, requêtes HTTP
                      |
              Serveur Web ESP32
                      |
           +----------+----------+
           |                     |
        Messages              Commandes
       otaLog()            handleCommand()
           |                     |
      /api/logs          Commande intégrée ?
           |                  /         \
     Affichage Web           Oui        Non
                             |          |
                         OTA.cpp   File FreeRTOS
                                        |
                                 otaReadCommand()
                                        |
                                    main.cpp
```

Cette séparation permet de conserver les fonctions de communication dans une bibliothèque et de gérer les actions propres à l'application dans `main.cpp`.

## VIII. Exemples d'utilisation dans le programme principal

### A. Afficher une variable régulièrement

Dans le code actuel, j'utilise un compteur, une température et un booléen pour vérifier l'affichage de différents types de variables :

```cpp
int compteur = 0;
float temperature = 25.5;
bool moteurActif = true;

void loop()
{
    compteur++;

    otaLog("Compteur : " + String(compteur));
    otaLog("Temperature : " + String(temperature, 2) + " C");
    otaLog("Moteur : " + String(moteurActif ? "ON" : "OFF"));

    delay(1000);
}
```

L'appel `delay(1000)` permet ici d'espacer les affichages d'une seconde, ce qui facilite leur lecture. Les valeurs de température et de moteur sont uniquement des variables d'exemple : elles ne proviennent pas d'un capteur ou d'un actionneur raccordé à la carte.

### B. Envoyer une valeur depuis la console Web

Il est également possible de prévoir une commande contenant une valeur. Par exemple, je peux saisir `vitesse=150` dans le champ de texte, puis récupérer cette valeur dans `main.cpp` :

```cpp
String commande;

if (otaReadCommand(commande))
{
    if (commande.startsWith("vitesse="))
    {
        String valeurTexte = commande.substring(8);
        int vitesse = valeurTexte.toInt();

        otaLog("Vitesse recue : " + String(vitesse));
    }
}
```

La fonction `startsWith("vitesse=")` permet de reconnaître le début de la commande. L'instruction `substring(8)` récupère la partie située après les huit caractères de `vitesse=`, puis `toInt()` transforme cette chaîne en entier. Dans cet exemple, la valeur est uniquement affichée : elle pourra ensuite être utilisée pour piloter un équipement.

### C. Différence entre `Serial.println()` et `otaLog()`

```cpp
Serial.println("Message USB uniquement");
otaLog("Message USB et console Web");
```

Dans le premier cas, le message apparaît seulement sur le moniteur série branché en USB. Dans le second, le même message est envoyé sur le port série et ajouté à l'historique consultable depuis le navigateur.

## IX. Mise en service

Pour lancer le projet, j'ouvre le dossier contenant `platformio.ini` avec VS Code et PlatformIO. Je renseigne le SSID et le mot de passe de mon réseau dans `include/wifi_credentials.h`, puis les identifiants d'accès à la console dans `include/terminal_credentials.h`.

Je téléverse ensuite une première fois le programme en USB et j'ouvre le moniteur série à 115200 bauds. Lorsque la connexion Wi-Fi est établie, l'ESP32 affiche son adresse IP et celle de la console Web. En saisissant cette adresse dans le navigateur d'un appareil connecté au même réseau, je peux accéder à la console après authentification.

Une fois l'ESP32 accessible sur le réseau, il est possible d'utiliser le téléversement OTA avec la configuration prévue dans `platformio.ini`. Le terminal Web et le service ArduinoOTA fonctionnent dans deux tâches distinctes.

Le projet utilise les bibliothèques `WiFi.h`, `ArduinoOTA.h` et `WebServer.h` fournies avec le framework Arduino pour ESP32. Aucune dépendance PlatformIO externe n'est déclarée pour la console.

Les identifiants Wi-Fi et Web ne sont pas destinés à être publiés avec le projet : les deux fichiers correspondants sont exclus par `.gitignore`. La console est prévue pour une utilisation sur un réseau local de confiance, puisque les échanges HTTP ne sont pas chiffrés.
