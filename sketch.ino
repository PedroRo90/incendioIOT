#include "DHTesp.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include <WiFi.h>
#include <PubSubClient.h>

const int DHT_PIN = 15;         
const int SMOKE_PIN = 34;  
const int LED_PIN = 21;
const int BUZZER_PIN = 22;
bool Alarm = false;

DHTesp dhtSensor;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "broker.emqx.io";
const char* mqtt_topic = "IPB/IoT/54448_54944/ponto1";
const char* mqtt_topic_subscribe = "IPB/IoT/54448_54944/ponto1/control"; 

WiFiClient espClient;
PubSubClient client (espClient);

void callback(char* topic, byte* payload, unsigned int length);

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Ligando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("A ligar ao broker MQTT...");
if (client.connect("ESP32Client-Wokwi")) {
      Serial.println(" conectado!");
      client.subscribe(mqtt_topic_subscribe); 
      Serial.print("Subscrito a: ");
      Serial.println(mqtt_topic_subscribe);
    } else {
      Serial.print(" erro, rc=");
      Serial.print(client.state());
      Serial.println(" a tentar novamente...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(SMOKE_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  connectWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  connectMQTT();
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.println("Recebi algo no callback MQTT!");

  Serial.print("Mensagem RAW recebida: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
    }
    Serial.println();

  Serial.print("Mensagem recebida: ");
  Serial.println(topic);
  String message;
  for(int i = 0; i < length; i++){
    message +=(char)payload[i];
  }
  message.trim();
  Serial.print("Mensagem: ");
  Serial.println(message);

  String topicStr = String(topic);
  if (String(topic) == "IPB/IoT/54448_54944/ponto1/control") {
    if (message.indexOf("ALARM_ON") >= 0) { //procura a palavra chave na mensagem
      Alarm = true;
    } else if (message.indexOf("ALARM_OFF") >= 0) {
      Alarm = false;
    } else {
      Serial.println(">> Nenhum comando ALARM reconhecido na mensagem.");
    }
  }
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  int sensorFumo = analogRead(SMOKE_PIN);

  //Ligar/desligar Led e Buzzer
  if (Alarm) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println(">> Alarme ativo: LED e buzzer ligados");
  } else {
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println(">> Alarme inativo: LED e buzzer desligados");
  }

  // Construir JSON
  String json = "{";
  json += "\"temperatura\":" + String(data.temperature, 2) + ",";
  json += "\"humidade\":" + String(data.humidity, 1) + ",";
  json += "\"gas\":" + String(sensorFumo) + ",";
  json += "\"lat\":41.122,";   
  json += "\"lon\":-8.605";   
  json += "}";

  // Publicar
  client.publish(mqtt_topic, json.c_str());
  Serial.println("Publicado: " + json);
  delay(5000);  
}
