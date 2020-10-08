#include "Arduino.h"

// AutoConnect
#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WEBServer;
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
typedef WebServer WEBServer;
#endif
#include <FS.h>
#include <AutoConnect.h>

// MQTT
#include <ArduinoJson.h>
#include <PubSubClient.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

#include <queue>

#include "config.h"
#include "DisplayManager.h"
#include "Packet.h"

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

#define RECONNECT_RATE 5 * 1000 // 5 sec
#define SEND_RATE 30 * 1000		// 30 sec

// MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Portal - SmartConfig
WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config;

unsigned long nextReconnectAttempt = 0;
char id[8];
unsigned long lastUpdate = 0;
std::queue<InovaBee::Packet> stack;

void onReceive(int packetSize);
void brokerConnect();
void sendData();
void sendPacket(InovaBee::Packet &packet);

void setup()
{
#if DEBUG
	displaySetup();

	//initialize Serial Monitor
	Serial.begin(115200);
	Serial.println("LoRa Receiver Test");
#endif

	//SPI LoRa pins
	SPI.begin(SCK, MISO, MOSI, SS);
	//setup LoRa transceiver module
	LoRa.setPins(SS, RST, DIO0);

	if (!LoRa.begin(BAND))
	{
#if DEBUG
		Serial.println("Starting LoRa failed!");
#endif
		// If LoRa was not enabled, we restart
		ESP.restart();
	}

#if DEBUG
	Serial.println("LoRa Initializing OK!");
#endif

	displayShowOK();

	// register the receive callback
	LoRa.onReceive(onReceive);

	// put the radio into receive mode
	LoRa.receive();

	lastUpdate = millis();

	// Setup MQTT
	client.setServer(MQTT_BROKER, MQTT_PORT);

	// Setup Portal
	Config.title = PORTAL_TITLE;
	Config.apid = PORTAL_TITLE + WiFi.macAddress();
	Config.psk = PORTAL_PW;
	Config.autoReconnect = true;
	Config.autoReset = true;

	Portal.config(Config);

	if (Portal.begin())
	{
#if DEBUG
		displayShowPortalInfo(true, WiFi.localIP().toString());
#endif
	}
	else
	{
#if DEBUG
		displayShowPortalInfo(false, "error");
#endif
	}
}

void onReceive(int packetSize)
{
	if (packetSize == 10)
	{
		LoRa.readBytes(id, 7);

		byte v1 = LoRa.read();
		byte v2 = LoRa.read();
		byte v3 = LoRa.read();

		InovaBee::Packet pack;

		pack.deviceID = String(id);
		pack.internalTemp = v1;
		pack.externalTemp = v2;
		pack.humidity = v3;

		stack.push(pack);

#if DEBUG
		Serial.println("new msg");
#endif
	}
}

void loop()
{
	Portal.handleClient();

	brokerConnect();
	client.loop();

	if (client.connected())
	{
		sendData();
	}
}

void sendData()
{
	if (lastUpdate < millis())
	{
#if DEBUG
		Serial.print("sending msgs: ");
		Serial.println(stack.size());
#endif

		while (stack.empty() == false)
		{
			InovaBee::Packet pack = stack.front();

#if DEBUG
			Serial.println(pack.deviceID);
			Serial.println(pack.internalTemp);
			Serial.println(pack.externalTemp);
			Serial.println(pack.humidity);
#endif

			sendPacket(pack);
			stack.pop();
		}

		lastUpdate = millis() + SEND_RATE;
	}
}

void sendPacket(InovaBee::Packet &packet)
{
	const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(4);
	DynamicJsonDocument doc(capacity);

	JsonArray deviceName = doc.createNestedArray(packet.deviceID);

	JsonObject deviceName_0 = deviceName.createNestedObject();
	deviceName_0["h0"] = packet.humidity;
	deviceName_0["t0"] = packet.externalTemp;
	deviceName_0["t1"] = packet.internalTemp;

	String data;
	serializeJson(doc, data);

#if DEBUG
	Serial.println(data);
#endif

	client.publish(DATA_GATEWAY_TOPIC, data.c_str());
}

void brokerConnect()
{
	if (client.connected() || nextReconnectAttempt > millis())
	{
		return;
	}

#if DEBUG
	Serial.println("Connecting to Broker");
#endif

	if (client.connect(MQTT_ID, MQTT_USER, NULL))
	{
#if DEBUG
		Serial.println("Connected to Broker");
#endif
	}
	else
	{
#if DEBUG
		Serial.print("Error while connecting to broker: ");
		Serial.println(client.state());
#endif
	}

	nextReconnectAttempt = millis() + RECONNECT_RATE;
}