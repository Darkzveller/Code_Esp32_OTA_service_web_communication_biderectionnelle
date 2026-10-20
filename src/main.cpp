
#include <Arduino.h>
#include "OTA.h"

int compteur = 0;
float temperature = 25.5;
bool moteurActif = true;

void setup()
{
    Serial.begin(115200);
    setupOTA();

    otaLog("ESP32 demarre !");
}

void loop()
{
    compteur++;

    // Afficher un entier
    otaLog("Compteur : " + String(compteur));

    // Afficher un float avec 2 decimales
    otaLog("Temperature : " + String(temperature, 2) + " C");

    // Afficher un booleen
    otaLog("Moteur : " + String(moteurActif ? "ON" : "OFF"));

    delay(1000);
}
