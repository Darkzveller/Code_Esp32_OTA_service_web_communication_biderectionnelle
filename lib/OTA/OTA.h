#pragma once

#include <Arduino.h>

// Demarre le Wi-Fi, ArduinoOTA et la console Web, chacun dans ses taches FreeRTOS.
// Aucun traitement n'est necessaire dans loop().
void setupOTA();

// Affiche la ligne dans le moniteur serie ET la console Web.
// A utiliser pour rendre les messages de vos futurs modules visibles a distance.
void otaLog(const String& message);
