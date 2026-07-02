/*
====================================================
      NodeMCU ESP8266 RECEIVER
====================================================

NRF24 Connections

VCC  -> 3.3V
GND  -> GND
CE   -> D4
CSN  -> D8
SCK  -> D5
MOSI -> D7
MISO -> D6

Touch Sensor -> D0
SDA -> D2
SCL -> D1
*/

#define BLYNK_TEMPLATE_ID "TMPL6y3RitiNO"
#define BLYNK_TEMPLATE_NAME "NRF Gateway"
#define BLYNK_AUTH_TOKEN "BKXPnxsl5PmOWyoVBk0AjePz3BWckePL"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>


#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "Xiaomi13T";
char pass[] = "12345678";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define TOUCH_PIN D0

RF24 radio(D4, D8);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

BlynkTimer timer;

// NodeMCU ----> Arduino
const byte txAddr[6] = "NODE1";

// Arduino ----> NodeMCU
const byte rxAddr[6] = "NODE2";

struct __attribute__((packed))Packet
{
  int16_t id;
  float temperature;
  uint8_t humidity;
  uint8_t relayStatus;
  uint8_t buttonState;
};

Packet txPacket;
Packet rxPacket;

bool relayCommand = false;

bool lastTouchState = HIGH;

unsigned long packetCounter = 0;

//--------------------------------------------------
// Receive command from Blynk Button
//--------------------------------------------------

BLYNK_WRITE(V2)
{
  relayCommand = !param.asInt();
}

//--------------------------------------------------
// Called after Blynk reconnects
//--------------------------------------------------

BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V2);
}

//--------------------------------------------------
// OLED
//--------------------------------------------------

void updateOLED()
{

  display.clearDisplay();

  display.setTextColor(WHITE);

  display.setTextSize(2);

  display.setCursor(0,0);

  display.print("T:");
  display.print(rxPacket.temperature,1);
  display.print((char)247);
  display.print("C");

  display.setCursor(0,22);

  display.print("H:");
  display.print(rxPacket.humidity);
  display.print("%");

  display.setCursor(0,44);

  display.print("R:");

  if(rxPacket.relayStatus)
      display.print("ON");
  else
      display.print("OFF");

  display.display();

}

//--------------------------------------------------
// Send Values to Blynk
//--------------------------------------------------

void updateBlynk()
{

  Blynk.virtualWrite(V0, rxPacket.temperature);

  Blynk.virtualWrite(V1, rxPacket.humidity);

  Blynk.virtualWrite(V3, rxPacket.relayStatus);

}

//--------------------------------------------------
// Touch Sensor
//--------------------------------------------------

void checkTouch()
{

  bool currentTouch = digitalRead(TOUCH_PIN);

  if(lastTouchState==HIGH && currentTouch==LOW)
  {

      relayCommand = !relayCommand;

      Blynk.virtualWrite(V2, relayCommand);

      Serial.println("Touch Detected");

  }

  lastTouchState = currentTouch;

}

//--------------------------------------------------
// Send Relay Command
//--------------------------------------------------

void sendCommand()
{

  txPacket.id++;

  txPacket.buttonState = relayCommand;

  radio.stopListening();

  bool ok = radio.write(&txPacket,sizeof(txPacket));

  if(ok)
      Serial.println("Command Sent");
  else
      Serial.println("Send Failed");

  radio.startListening();

}

//--------------------------------------------------
// Receive Sensor Data
//--------------------------------------------------

void receiveData()
{

  if(radio.available())
  {

      radio.read(&rxPacket,sizeof(rxPacket));

      Serial.println("--------------------------------");

      Serial.print("Temperature : ");
      Serial.println(rxPacket.temperature);

      Serial.print("Humidity : ");
      Serial.println(rxPacket.humidity);

      Serial.print("Relay : ");
      Serial.println(rxPacket.relayStatus);

      updateOLED();

      updateBlynk();

  }

}

//--------------------------------------------------
// Communication Task
//--------------------------------------------------

void radioTask()
{

  checkTouch();

  sendCommand();

  receiveData();

}

//--------------------------------------------------
// SETUP
//--------------------------------------------------

void setup()
{

  Serial.begin(115200);

  pinMode(TOUCH_PIN,INPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C))
  {
      while(1);
  }

  display.clearDisplay();
  display.display();

  if(!radio.begin())
  {
      Serial.println("NRF24 Not Found");

      while(1);
  }

  radio.setPALevel(RF24_PA_LOW);

  radio.setDataRate(RF24_250KBPS);

  radio.setChannel(76);

  radio.setRetries(5,15);

  radio.setAutoAck(true);

  radio.disableDynamicPayloads();

  radio.openWritingPipe(txAddr);

  radio.openReadingPipe(1,rxAddr);

  radio.startListening();

  WiFi.begin(ssid,pass);

  Blynk.config(auth);

  while(WiFi.status()!=WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }

  Blynk.connect();

  timer.setInterval(100L, radioTask);

  Serial.println();

  Serial.println("Receiver Ready");

}
//--------------------------------------------------
// Check WiFi Connection
//--------------------------------------------------

void checkWiFi()
{

    if (WiFi.status() != WL_CONNECTED)
    {

        Serial.println("WiFi Lost...");

        WiFi.disconnect();

        WiFi.begin(ssid, pass);

    }

}

//--------------------------------------------------
// Check Blynk Connection
//--------------------------------------------------

void checkBlynk()
{

    if (!Blynk.connected())
    {

        Serial.println("Connecting Blynk...");

        Blynk.connect(1000);

    }

}

//--------------------------------------------------
// Display Startup Screen
//--------------------------------------------------

void splashScreen()
{

    display.clearDisplay();

    display.setTextSize(2);

    display.setCursor(10,10);
    display.println("HOME");

    display.setCursor(10,35);
    display.println("AUTOMATION");

    display.display();

    delay(1500);

}

//--------------------------------------------------
// Display Connection Status
//--------------------------------------------------

void showConnectionStatus()
{

    static unsigned long lastBlink = 0;

    static bool visible = true;

    if (millis() - lastBlink < 500)
        return;

    lastBlink = millis();

    visible = !visible;

    display.fillRect(105,0,20,10,BLACK);

    display.setTextSize(1);

    display.setCursor(106,0);

    if (WiFi.status() == WL_CONNECTED)
        display.print("W");

    if (radio.available())
        display.print("R");

    if (Blynk.connected())
        display.print("B");

    display.display();

}

//--------------------------------------------------
// Monitor Communication Timeout
//--------------------------------------------------

void communicationWatchdog()
{

    static unsigned long lastPacket = 0;

    if (radio.available())
    {
        lastPacket = millis();
    }

    if (millis() - lastPacket > 3000)
    {

        Serial.println("No NRF24 Data");

    }

}

//--------------------------------------------------
// Main Loop
//--------------------------------------------------

void loop()
{

    Blynk.run();

    timer.run();

    checkWiFi();

    checkBlynk();

    communicationWatchdog();

    showConnectionStatus();

}
