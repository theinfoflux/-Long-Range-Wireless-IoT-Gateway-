/*
====================================================
 Arduino UNO (Slave)
 DHT11 + Relay + NRF24L01
====================================================

NRF24 Connections

VCC  -> 3.3V (or 5V to adapter input)
GND  -> GND
CE   -> D9
CSN  -> D10
SCK  -> D13
MOSI -> D11
MISO -> D12

Relay -> D6
DHT11 -> D4

*/

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define RELAY_PIN 6

RF24 radio(9,10);
DHT dht(DHTPIN, DHTTYPE);

// NodeMCU -> Arduino
const byte rxAddr[6] = "NODE1";

// Arduino -> NodeMCU
const byte txAddr[6] = "NODE2";

struct __attribute__((packed)) Packet
{
  int16_t id;
  float temperature;
  uint8_t humidity;
  uint8_t relayStatus;
  uint8_t buttonState;
};

Packet rxPacket;
Packet txPacket;

unsigned long previousDHT = 0;

float temperature = 0;
uint8_t humidity = 0;

void setup()
{
  Serial.begin(115200);

  pinMode(RELAY_PIN,OUTPUT);

  digitalWrite(RELAY_PIN,HIGH);
Serial.print("Packet Size = ");
Serial.println(sizeof(Packet));
  dht.begin();

  if(!radio.begin())
  {
    Serial.println("NRF24 NOT Detected");

    while(1);
  }

  radio.setPALevel(RF24_PA_LOW);

  radio.setDataRate(RF24_250KBPS);

  radio.setChannel(76);

  radio.setRetries(5,15);

  radio.setAutoAck(true);
  radio.openReadingPipe(1,rxAddr);

  radio.openWritingPipe(txAddr);

  radio.startListening();

  Serial.println("Arduino Ready");
  
}

void loop()
{
  if(millis()-previousDHT>2000)
  {
    previousDHT=millis();

    float t=dht.readTemperature();

    float h=dht.readHumidity();

    if(!isnan(t))
      temperature=t;

    if(!isnan(h))
      humidity=h;
  }

  if(radio.available())
  {
    while(radio.available())
    {
      radio.read(&rxPacket,sizeof(rxPacket));
    }

    Serial.println("Received");

    Serial.print("Button : ");

    Serial.println(rxPacket.buttonState);

    if(rxPacket.buttonState==LOW)
    {
      digitalWrite(RELAY_PIN,LOW);

      txPacket.relayStatus=1;
    }
    else
    {
      digitalWrite(RELAY_PIN,HIGH);

      txPacket.relayStatus=0;
    }

    txPacket.id++;

    txPacket.temperature=temperature;

    txPacket.humidity=humidity;
       Serial.println("------------------------");
    Serial.print("ID          : ");
    Serial.println(txPacket.id);

    Serial.print("Temperature : ");
    Serial.println(txPacket.temperature);

    Serial.print("Humidity    : ");
    Serial.println(txPacket.humidity);

    Serial.print("Relay       : ");
    Serial.println(txPacket.relayStatus);

    radio.stopListening();

    bool ok = radio.write(&txPacket, sizeof(txPacket));

    if (ok)
      Serial.println("Reply Sent");
    else
      Serial.println("Reply Failed");

    radio.startListening();

    Serial.println("------------------------");
  }
}
