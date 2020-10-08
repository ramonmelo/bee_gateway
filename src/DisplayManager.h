//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#include <Arduino.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void displaySetup()
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
}

void displayShowOK()
{
	display.setCursor(0, 10);
	display.println("LoRa Initializing OK!");
	display.display();
}

void displayShowPortalInfo(bool ok, String msg)
{
	display.setCursor(0, 20);
	if (ok)
	{
		display.println("Portal OK!");
		display.setCursor(0, 30);
		display.println(msg);
	}
	else
	{
		display.println("Portal NOT OK!");
	}

	display.display();
}