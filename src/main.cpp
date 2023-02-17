//---------------------------------------------------------------------------------------------------------------------
// main.cpp
// DHT11 weather node for firebase.
// Style: Procedural
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <DFRobot_DHT11.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>

//---------------------------------------------------------------------------------------------------------------------
// Local Includes
//---------------------------------------------------------------------------------------------------------------------
#include "auth.hpp"

//---------------------------------------------------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------------------------------------------------
#define DHT11_PIN                                                                                                      \
    14 // For ESP32 WEMOS D1 board, this is connected to the D15/SCL/D3 pin and
       // somehow it is called pin 5...
// State Machine.
#define STATE_DELAY_MS 50
//---------------------------------------------------------------------------------------------------------------------
// Class
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// Global Variables
//---------------------------------------------------------------------------------------------------------------------
DFRobot_DHT11 DHT;

float current_temperature = 0.0;
float current_humidity = 0.0;

uint32_t previous_millis = 0;
bool time_to_gather = false;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool anon_sign_up = false;
//---------------------------------------------------------------------------------------------------------------------
// Functions Declaration
//---------------------------------------------------------------------------------------------------------------------
void update_temperature();
void update_humidity();
//---------------------------------------------------------------------------------------------------------------------
// Setup and Loop
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
void connect_wifi()
{
    WiFi.mode(WIFI_STA);
    WiFi.hostname(Authentication::ESP_HOST_NAME);

    WiFi.begin(Authentication::WIFI_SSID, Authentication::WIFI_PASSWORD);
    Serial.print("Connecting to ");
    Serial.println(Authentication::WIFI_SSID);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected!");
    Serial.println(WiFi.localIP());
    Serial.println();

    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

//---------------------------------------------------------------------------------------------------------------------
void connect_firebase()
{
    config.api_key = Authentication::FB_WEB_API_KEY;
    config.database_url = Authentication::FB_RTDB_URL;
    // Anonymous access, sign up everytime.
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("ok");
        anon_sign_up = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

//---------------------------------------------------------------------------------------------------------------------
void setup()
{
    Serial.begin(9600);
    delay(1500);

    DHT.read(DHT11_PIN);
    update_temperature();
    update_humidity();

    connect_wifi();
    connect_firebase();
}

//---------------------------------------------------------------------------------------------------------------------
void loop()
{
    delay(STATE_DELAY_MS);
    if (!anon_sign_up)
    {
        return;
    }

    static int count = 0;
    static const uint32_t THIRTY_SECONDS = 30UL * 1000UL;
    uint32_t current_millis = millis();

    // A simple timer actually for a minute...
    if (current_millis - previous_millis > THIRTY_SECONDS)
    {
        // Save the last time tick.
        previous_millis = current_millis;

        DHT.read(DHT11_PIN);
        update_temperature();
        update_humidity();

        // push to firebase.
        if (Firebase.ready())
        {
            if (Firebase.RTDB.setFloat(&fbdo, "test/temperature", current_temperature))
            {
                Serial.println("PASSED");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
            }
            else
            {
                Serial.println("FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }

            if (Firebase.RTDB.setFloat(&fbdo, "test/humidity", current_humidity))
            {
                Serial.println("PASSED");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
            }
            else
            {
                Serial.println("FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void update_temperature()
{
    float new_temp = DHT.temperature;
    if (isnan(new_temp))
    {
        Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
        current_temperature = new_temp;
        Serial.print("Current temperature: ");
        Serial.println(current_temperature);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void update_humidity()
{
    float new_humidity = DHT.humidity;
    if (isnan(new_humidity))
    {
        Serial.println("Failed to read from DHT sensor!");
    }
    else
    {
        current_humidity = new_humidity;
        Serial.print("Current humidity: ");
        Serial.println(current_humidity);
    }
}
