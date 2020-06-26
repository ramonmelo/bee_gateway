#include "Arduino.h"

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <queue>

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

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define DELAY 30 * 1000 // 30 seg

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

char id[8];
unsigned long lastUpdate = 0;
std::queue<InovaBee::Packet> stack;

void onReceive(int packetSize);

void setup()
{
	//reset OLED display via software
	pinMode(OLED_RST, OUTPUT);
	digitalWrite(OLED_RST, LOW);
	delay(20);
	digitalWrite(OLED_RST, HIGH);

	//initialize OLED
	Wire.begin(OLED_SDA, OLED_SCL);
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false))
	{ // Address 0x3C for 128x32
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}

	display.clearDisplay();
	display.setTextColor(WHITE);
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.print("LORA RECEIVER ");
	display.display();

	//initialize Serial Monitor
	Serial.begin(115200);

	Serial.println("LoRa Receiver Test");

	//SPI LoRa pins
	SPI.begin(SCK, MISO, MOSI, SS);
	//setup LoRa transceiver module
	LoRa.setPins(SS, RST, DIO0);

	if (!LoRa.begin(BAND))
	{
		Serial.println("Starting LoRa failed!");
		while (1)
			;
	}
	Serial.println("LoRa Initializing OK!");
	display.setCursor(0, 10);
	display.println("LoRa Initializing OK!");
	display.display();

	// register the receive callback
	LoRa.onReceive(onReceive);

	// put the radio into receive mode
	LoRa.receive();

	lastUpdate = millis();
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

		Serial.println("new msg");
	}
}

void loop()
{
	if (lastUpdate < millis())
	{
		Serial.print("sending msgs: ");
		Serial.println(stack.size());
		
		while (stack.empty() == false)
		{
			InovaBee::Packet pack = stack.front();

			Serial.println(pack.deviceID);
			Serial.println(pack.internalTemp);
			Serial.println(pack.externalTemp);
			Serial.println(pack.humidity);

			stack.pop();
		}

		lastUpdate = millis() + DELAY;
	}

	//try to parse packet
	// int packetSize = LoRa.parsePacket();
	// if (packetSize)
	// {
	// //received a packet
	// Serial.print("Received packet ");

	// int len = LoRa.available();

	// if (len > 0)
	// {
	// 	byte *buffer = new byte[len];
	// 	LoRa.readBytes(buffer, len);

	// 	for (size_t i = 0; i < len; i++)
	// 	{
	// 		Serial.print("Value ");
	// 		Serial.print(i);
	// 		Serial.print(" : ");
	// 		Serial.println(buffer[i]);
	// 	}
	// }

	// //print RSSI of packet
	// int rssi = LoRa.packetRssi();
	// Serial.print(" with RSSI ");
	// Serial.println(rssi);

	// // Dsiplay information
	// display.clearDisplay();
	// display.setCursor(0, 0);
	// display.print("LORA RECEIVER");
	// display.setCursor(0, 20);
	// display.print("Received packet:");
	// display.setCursor(0, 30);
	// display.print(LoRaData);
	// display.setCursor(0, 40);
	// display.print("RSSI:");
	// display.setCursor(30, 40);
	// display.print(rssi);
	// display.display();
	// }
}