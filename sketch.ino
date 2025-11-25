#include <WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// =======================
// Configurações WiFi/MQTT
// =======================
const char* WIFI_SSID   = "Wokwi-GUEST";
const char* WIFI_PASS   = "";
const char* MQTT_SERVER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

const char* TOPIC_TEMP   = "vacinas/box01/temp";
const char* TOPIC_ALARME = "vacinas/box01/alarme";
const char* TOPIC_CMD    = "vacinas/box01/cmd";
const char* CLIENT_ID    = "esp32_box01";

// =======================
// PINAGEM
// =======================
#define ONE_WIRE_PIN 13
#define LED_OK_PIN 2
#define LED_ALERT_PIN 4
#define BUZZER_PIN 16

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
WiFiClient espClient;
PubSubClient client(espClient);

bool sensor_detected = false;
DeviceAddress tempSensorAddress;

// =======================
// Regras de temperatura
// =======================
float TEMP_LOW        = 2.0;
float TEMP_HIGH       = 8.0;
float CRITICAL_HIGH   = 10.0;
float HISTERESE       = 0.5;

// =======================
// Controle de leitura
// =======================
unsigned long lastSample = 0;
const unsigned long SAMPLE_INTERVAL   = 30000;
const unsigned long CONVERSION_DELAY  = 1000;

// Tempo total de funcionamento
unsigned long startTime;

// =========================================================
// FUNÇÃO DE LEITURA DO SENSOR + PUBLICAÇÃO NO MQTT
// =========================================================
void performSamplingAndPublish() {

  if (!sensor_detected) return;

  Serial.println("[SENSOR] Solicitando leitura...");
  sensors.requestTemperatures();
  delay(CONVERSION_DELAY);

  float tempC = sensors.getTempCByIndex(0);

  // Cálculo do tempo total de execução
  unsigned long uptimeSec = (millis() - startTime) / 1000;

  // Verificação de erro de leitura
  if (tempC == DEVICE_DISCONNECTED_C || tempC == 85.0) {
    Serial.println("[ERRO] Sensor desconectado ou leitura inválida.");
    client.publish(TOPIC_ALARME, "ERRO_SENSOR");
    return;
  }

  // Montagem do JSON publicado
  char payload[200];
  snprintf(payload, sizeof(payload),
           "{\"tC\": %.2f, \"uptime_s\": %lu}",
           tempC, uptimeSec);

  client.publish(TOPIC_TEMP, payload, true);

  // =======================
  // Avaliação da temperatura
  // =======================

  // Crítica (>10°C)
  if (tempC >= CRITICAL_HIGH) {
    Serial.printf("[CRÍTICO] %.2f°C 🚨 ACIMA DE 10°C – RISCO CRÍTICO!\n", tempC);

    digitalWrite(LED_OK_PIN, LOW);
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    client.publish(TOPIC_ALARME, "CRITICO");
    return;
  }

  // Muito baixa (<2°C)
  if (tempC < (TEMP_LOW - HISTERESE)) {
    Serial.printf("[ALERTA] %.2f°C ❄ MUITO BAIXA – risco de congelamento!\n", tempC);

    digitalWrite(LED_OK_PIN, LOW);
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    client.publish(TOPIC_ALARME, "BAIXO");
    return;
  }

  // Alta (>8°C)
  if (tempC > (TEMP_HIGH + HISTERESE)) {
    Serial.printf("[ALERTA] %.2f°C ⚠ ALTA – vacinas em risco!\n", tempC);

    digitalWrite(LED_OK_PIN, LOW);
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);

    client.publish(TOPIC_ALARME, "ALTO");
    return;
  }

  // Temperatura ideal
  Serial.printf(
    "[OK] %.2f°C 🌡 Temperatura ideal – vacinas seguras! (Uptime: %lus)\n",
    tempC, uptimeSec
  );

  digitalWrite(LED_OK_PIN, HIGH);
  digitalWrite(LED_ALERT_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  client.publish(TOPIC_ALARME, "OK");
}

// =========================================================
// CALLBACK DO MQTT
// =========================================================
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  Serial.print("[MQTT CMD] Mensagem recebida: ");
  Serial.println(msg);

  if (String(topic) == TOPIC_CMD) {

    if (msg == "ALARM_ON") {
      digitalWrite(LED_ALERT_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      client.publish(TOPIC_ALARME, "FORCADO_ON");
      Serial.println("[CMD] Alarme acionado via MQTT!");
    }

    if (msg == "ALARM_OFF") {
      digitalWrite(LED_ALERT_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      client.publish(TOPIC_ALARME, "FORCADO_OFF");
      Serial.println("[CMD] Alarme desligado via MQTT!");
    }
  }
}

// =========================================================
// RECONEXÃO AO MQTT
// =========================================================
void reconnect() {
  while (!client.connected()) {
    Serial.println("[MQTT] Conectando ao broker...");

    if (client.connect(CLIENT_ID)) {
      Serial.println("[MQTT] Conectado!");
      client.subscribe(TOPIC_CMD);
      Serial.println("[MQTT] Inscrito no tópico de comando.");
    } else {
      Serial.println("[MQTT] Falha, tentando novamente em 2s...");
      delay(2000);
    }
  }
}

// =========================================================
// SETUP
// =========================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("[SENSOR] Inicializando DS18B20...");
  sensors.begin();

  if (sensors.getDeviceCount() > 0) {
    sensor_detected = true;
    sensors.getAddress(tempSensorAddress, 0);
    sensors.setResolution(tempSensorAddress, 12);
    Serial.println("[SENSOR] Sensor detectado e configurado.");
  } else {
    Serial.println("[ERRO] Nenhum sensor detectado.");
  }

  Serial.printf("[WIFI] Conectando a %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\n[WIFI] Conectado!");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  reconnect();

  // Marca o início do sistema
  startTime = millis();

  lastSample = millis() - SAMPLE_INTERVAL;
}

// =========================================================
// LOOP PRINCIPAL
// =========================================================
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;
    performSamplingAndPublish();
  }
}
