#include <Arduino.h>
#include "OTA.h"
int compteur = 0;
float temperature = 25.5;
bool moteurActif = true;

void setup()
{
    Serial.begin(115200);
    setupOTA();
}

void loop()
{
    String commande;
    if (otaReadCommand(commande))
    {
        // otaLog("Commande recue : " + commande);
        if (commande == "bonjour")
        {
            otaLog("Salut depuis l'ESP32 !");
        }
        else if (commande == "moteur_on")
        {
            otaLog("Activation du moteur (exemple : ajouter votre code ici)");
        }
        else if (commande == "moteur_off")
        {
            otaLog("Arret du moteur (exemple : ajouter votre code ici)");
        }
        // else
        // {
        //     otaLog("Commande applicative non reconnue : " + commande);
        // }

        // delay(10000);
    }
    compteur++;

    // Afficher un entier
    otaLog("Compteur : " + String(compteur));

    // Afficher un float avec 2 decimales
    otaLog("Temperature : " + String(temperature, 2) + " C");

    // Afficher un booleen
    otaLog("Moteur : " + String(moteurActif ? "ON" : "OFF"));

    delay(1000);
}
