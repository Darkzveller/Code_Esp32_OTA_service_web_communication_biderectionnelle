#include "OTA.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <WiFi.h>

#include "WebTerminalPage.h"
#include "terminal_credentials.h"
#include "wifi_credentials.h"

namespace
{
    constexpr const char* OTA_HOSTNAME = "espmaitre";
    constexpr size_t MAX_LOG_CHARS = 8192;
    constexpr uint32_t RECONNECT_INTERVAL_MS = 15000;

    WebServer webServer(80);
    SemaphoreHandle_t logMutex = nullptr;
    String logText;
    uint32_t logRevision = 0;
    bool restartPending = false; // Modifie uniquement par webTask().
    uint32_t restartAt = 0;

    bool requireAuth()
    {
        // Authentification HTTP Digest : mot de passe non envoye en clair.
        // ATTENTION : HTTP n'est pas chiffre ; utiliser uniquement sur un reseau de confiance.
        if (webServer.authenticate(TERMINAL_USERNAME, TERMINAL_PASSWORD))
            return true;
        webServer.requestAuthentication(DIGEST_AUTH, "ESP32 console");
        return false;
    }

    void setNoCache()
    {
        webServer.sendHeader("Cache-Control", "no-store, max-age=0");
    }

    void handleLogs()
    {
        if (!requireAuth()) return;
        String snapshot;
        uint32_t revision;
        if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) != pdTRUE)
        {
            webServer.send(503, "text/plain", "Console occupee");
            return;
        }
        revision = logRevision;
        if (webServer.arg("rev") != String(revision))
            snapshot = logText;
        xSemaphoreGive(logMutex);

        setNoCache();
        webServer.sendHeader("X-Log-Revision", String(revision));
        if (webServer.arg("rev") == String(revision))
            webServer.send(204);
        else
            webServer.send(200, "text/plain; charset=utf-8", snapshot);
    }

    void clearLogs()
    {
        if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            logText = "";
            ++logRevision;
            xSemaphoreGive(logMutex);
        }
    }

    void handleCommand()
    {
        if (!requireAuth()) return;
        String cmd = webServer.arg("cmd");
        cmd.trim();
        cmd.toLowerCase();
        if (cmd.length() > 64)
        {
            webServer.send(400, "text/plain", "Commande trop longue");
            return;
        }
        if (cmd.length() == 0)
        {
            webServer.send(400, "text/plain", "Commande vide");
            return;
        }

        otaLog("> " + cmd);
        if (cmd == "help" || cmd == "aide")
        {
            otaLog("Commandes : help, status, ip, heap, uptime, clear, restart");
        }
        else if (cmd == "status" || cmd == "etat")
        {
            otaLog("Wi-Fi : " + String(WiFi.status() == WL_CONNECTED ? "connecte" : "deconnecte"));
            otaLog("IP : " + WiFi.localIP().toString());
            otaLog("Signal : " + String(WiFi.RSSI()) + " dBm");
            otaLog("Memoire libre : " + String(ESP.getFreeHeap()) + " octets");
            otaLog("Uptime : " + String(millis() / 1000UL) + " secondes");
        }
        else if (cmd == "ip")
            otaLog("IP : " + WiFi.localIP().toString());
        else if (cmd == "heap")
            otaLog("Memoire libre : " + String(ESP.getFreeHeap()) + " octets");
        else if (cmd == "uptime")
            otaLog("Uptime : " + String(millis() / 1000UL) + " secondes");
        else if (cmd == "clear")
            clearLogs();
        else if (cmd == "restart")
        {
            otaLog("Redemarrage demande. La console va se deconnecter.");
            restartAt = millis() + 1000UL;
            restartPending = true;
        }
        else
            otaLog("Commande inconnue : " + cmd + " (taper help)");

        setNoCache();
        webServer.send(200, "text/plain", "OK");
    }

    void setupWebServer()
    {
        webServer.on("/", HTTP_GET, []()
        {
            if (!requireAuth()) return;
            setNoCache();
            webServer.send_P(200, "text/html; charset=utf-8", WEB_TERMINAL_HTML);
        });
        webServer.on("/api/logs", HTTP_GET, handleLogs);
        webServer.on("/api/command", HTTP_POST, handleCommand);
        webServer.onNotFound([]()
        {
            if (!requireAuth()) return;
            webServer.send(404, "text/plain", "Page introuvable");
        });
        webServer.begin();
    }

    void otaTask(void* parameter)
    {
        (void)parameter;
        for (;;)
        {
            ArduinoOTA.handle();
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    void webTask(void* parameter)
    {
        (void)parameter;
        bool wasConnected = WiFi.status() == WL_CONNECTED;
        uint32_t lastReconnectTry = millis();
        for (;;)
        {
            const bool connected = WiFi.status() == WL_CONNECTED;
            if (connected)
            {
                if (!wasConnected)
                {
                    webServer.close();
                    webServer.begin();
                    otaLog("Wi-Fi retabli. Terminal : http://" + WiFi.localIP().toString() + "/");
                }
                webServer.handleClient();
            }
            else if (millis() - lastReconnectTry >= RECONNECT_INTERVAL_MS)
            {
                lastReconnectTry = millis();
                WiFi.reconnect();
            }
            wasConnected = connected;
            if (restartPending && static_cast<int32_t>(millis() - restartAt) >= 0)
                ESP.restart();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void otaLog(const String& message)
{
    // Tous les messages passes par otaLog() sont egalement visibles sur USB.
    Serial.println(message);

    if (logMutex == nullptr) return;
    const String entry = "[" + String(millis() / 1000UL) + " s] " + message + "\n";
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;

    logText += entry;
    while (logText.length() > MAX_LOG_CHARS)
    {
        int firstLineEnd = logText.indexOf('\n');
        if (firstLineEnd < 0) { logText.remove(0, logText.length() - MAX_LOG_CHARS); break; }
        logText.remove(0, firstLineEnd + 1);
    }
    ++logRevision;
    xSemaphoreGive(logMutex);
}

void setupOTA()
{
    logMutex = xSemaphoreCreateMutex();
    if (logMutex == nullptr)
    {
        Serial.println("Erreur critique : allocation de la console impossible");
        return;
    }
    logText.reserve(MAX_LOG_CHARS + 64);

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(OTA_HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    otaLog("Connexion au Wi-Fi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    otaLog("Wi-Fi connecte");
    otaLog("Adresse IP : " + WiFi.localIP().toString());

    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.onStart([]() { otaLog("Debut de la mise a jour OTA"); });
    ArduinoOTA.onEnd([]() { otaLog("Mise a jour OTA terminee"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        if (total == 0) return;
        const unsigned int percentage = (progress * 100U) / total;
        static unsigned int previousBucket = 999;
        const unsigned int bucket = percentage / 10U;
        if (bucket != previousBucket)
        {
            previousBucket = bucket;
            otaLog("OTA : " + String(percentage) + "%");
        }
    });
    ArduinoOTA.onError([](ota_error_t error)
    {
        otaLog("Erreur OTA : " + String(static_cast<unsigned int>(error)));
    });
    ArduinoOTA.begin();
    otaLog("OTA pret : " + String(OTA_HOSTNAME) + ".local");

    setupWebServer();
    otaLog("Console Web : http://" + WiFi.localIP().toString() + "/");
    otaLog("Saisir le compte defini dans include/terminal_credentials.h");

    if (xTaskCreate(otaTask, "OTA", 8192, nullptr, 1, nullptr) != pdPASS)
        otaLog("ERREUR : impossible de demarrer la tache OTA");
    if (xTaskCreate(webTask, "WEB", 8192, nullptr, 1, nullptr) != pdPASS)
        otaLog("ERREUR : impossible de demarrer la tache Web");
}
