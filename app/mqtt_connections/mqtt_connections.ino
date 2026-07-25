#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//=========================
// WIFI
//=========================
const char* ssid = "Pasca Sarjana";
const char* password = "ublkecee";

//=========================
// MQTT
//=========================
const char* mqtt_server = "t2a21271.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883;

const char* mqtt_user = "anemometer_esp1";
const char* mqtt_pass = "esp12345678";

const char* topic = "weather/anemometer";

//=========================
// LED STATUS
//=========================
#define LED_STATUS 2

int mqttFailCount = 0;
const int MQTT_FAIL_THRESHOLD = 5; // setelah 5x gagal beruntun -> LED nyala terus

//=========================
// CA CERTIFICATE
// Ganti dengan isi file .crt
//=========================
const char* root_ca =
"-----BEGIN CERTIFICATE-----\n"
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
"MrY=\n"
"-----END CERTIFICATE-----\n";

//=========================
// HC020K
//=========================
#define SENSOR_PIN 13

volatile unsigned long pulseCount = 0;

WiFiClientSecure espClient;
PubSubClient client(espClient);

//=========================
// PARAMETER
//=========================
const int SLOT = 10;

const float RADIUS = 0.044;          // meter

const float CALIBRATION = 2.40;

unsigned long lastMillis = 0;

//=========================
// INTERRUPT
//=========================
void IRAM_ATTR countPulse()
{
    pulseCount++;
}

//=========================
// LED HELPER
//=========================
void ledBlinkOnceShort()
{
    // kedip singkat -> data berhasil terkirim
    digitalWrite(LED_STATUS, HIGH);
    delay(50);
    digitalWrite(LED_STATUS, LOW);
}

void ledBlinkFail3x()
{
    // tiga kedip cepat -> pengiriman gagal
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(LED_STATUS, HIGH);
        delay(100);
        digitalWrite(LED_STATUS, LOW);
        delay(100);
    }
}

//=========================
// WIFI
//=========================
void connectWiFi()
{
    Serial.print("Connecting WiFi");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        // berkedip cepat -> sedang menghubungkan
        digitalWrite(LED_STATUS, HIGH);
        delay(100);
        digitalWrite(LED_STATUS, LOW);
        delay(100);

        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected");

    digitalWrite(LED_STATUS, LOW); // standby sementara nunggu MQTT
}

//=========================
// MQTT
//=========================
void reconnectMQTT()
{
    while (!client.connected())
    {
        Serial.print("Connecting MQTT...");

        // berkedip cepat -> sedang menghubungkan
        digitalWrite(LED_STATUS, HIGH);
        delay(100);
        digitalWrite(LED_STATUS, LOW);
        delay(100);

        if(client.connect("ESP32_Anemometer", mqtt_user, mqtt_pass))
        {
            Serial.println("Connected");

            mqttFailCount = 0;
            digitalWrite(LED_STATUS, LOW); // mati -> sistem normal, standby
        }
        else
        {
            Serial.print("Failed : ");
            Serial.println(client.state());

            mqttFailCount++;

            if (mqttFailCount >= MQTT_FAIL_THRESHOLD)
            {
                // menyala terus -> gangguan koneksi (gagal berkali-kali)
                digitalWrite(LED_STATUS, HIGH);
                delay(3000);
            }
            else
            {
                // berkedip pelan -> WiFi tersambung, MQTT belum
                digitalWrite(LED_STATUS, HIGH);
                delay(1500);
                digitalWrite(LED_STATUS, LOW);
                delay(1500);
            }
        }
    }
}

//=========================
// SETUP
//=========================
void setup()
{
    Serial.begin(115200);

    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);

    pinMode(SENSOR_PIN, INPUT_PULLUP);

    attachInterrupt(
        digitalPinToInterrupt(SENSOR_PIN),
        countPulse,
        FALLING
    );

    connectWiFi();

    espClient.setCACert(root_ca);

    client.setServer(mqtt_server, mqtt_port);
}

//=========================
// LOOP
//=========================
void loop()
{
    if(WiFi.status()!=WL_CONNECTED)
        connectWiFi();

    if(!client.connected())
        reconnectMQTT();

    client.loop();

    if(millis()-lastMillis>=1000)
    {
        noInterrupts();

        unsigned long pulse = pulseCount;

        pulseCount = 0;

        interrupts();

        //-----------------------
        // RPM
        //-----------------------
        float rpm =
            (pulse * 60.0) / SLOT;

        //-----------------------
        // Tip Speed
        //-----------------------
        float tipSpeed =
            (2.0 * PI * RADIUS * rpm) / 60.0;

        //-----------------------
        // Wind Speed
        //-----------------------
        float windSpeed =
            tipSpeed * CALIBRATION;

        float kmh =
            windSpeed * 3.6;

        //-----------------------
        // JSON
        //-----------------------
        StaticJsonDocument<256> doc;

        doc["pulse"] = pulse;
        doc["rpm"] = rpm;
        doc["speed"] = windSpeed;
        doc["speed_ms"] = windSpeed;
        doc["speed_kmh"] = kmh;

        char buffer[256];

        serializeJson(doc, buffer);

        bool sent = client.publish(topic, buffer);

        if (sent)
        {
            ledBlinkOnceShort();  // kedip singkat -> data berhasil dikirim
        }
        else
        {
            ledBlinkFail3x();     // tiga kedip cepat -> pengiriman gagal
        }

        //-----------------------
        // Serial Monitor
        //-----------------------
        Serial.println("====================");
        Serial.print("Pulse : ");
        Serial.println(pulse);

        Serial.print("RPM : ");
        Serial.println(rpm);

        Serial.print("Wind : ");
        Serial.print(windSpeed);
        Serial.println(" m/s");

        Serial.print("KMH : ");
        Serial.println(kmh);

        lastMillis = millis();
    }
}