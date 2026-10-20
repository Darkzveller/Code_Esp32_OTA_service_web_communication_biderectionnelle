#pragma once

#include <Arduino.h>

// Demarre le Wi-Fi, ArduinoOTA et la console Web, chacun dans ses taches FreeRTOS.
// Aucun traitement n'est necessaire dans loop().
void setupOTA();

// Affiche la ligne dans le moniteur serie ET la console Web.
// A utiliser pour rendre les messages de vos futurs modules visibles a distance.
void otaLog(const String& message);

// Lit sans attendre une commande envoyee depuis le navigateur.
// Les commandes integrees (help, status, ip, heap, uptime, clear, restart)
// sont gerees par la bibliotheque et ne sont pas transmises au main.
// Renvoie true et renseigne `command` si une commande est disponible.
bool otaReadCommand(String& command);
