#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h> 
//-----------------------------------------
#define RST_PIN  D3
#define SS_PIN   D4
#define BUZZER   D8
//-----------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;      
//-----------------------------------------
int blockNum = 4;  
byte bufferLen = 18;
byte readBlockData[18];
//-----------------------------------------
String card_holder_name;
const String sheet_url = "https://script.google.com/macros/s/AKfycbyhn-j8AvFUooVuRRiZaMo4nu0drUI5M-UdssvwVbrNO12ujRxdnZSJhVnRRqWKtUxmvQ/exec?name=";  //Enter Google Script URL
 
//-----------------------------------------
#define WIFI_SSID "qwerty"  //Enter WiFi Name
#define WIFI_PASSWORD "12345678"  //Enter WiFi Password
//-----------------------------------------

// Initialize the LCD display with PCF8574 library
LiquidCrystal_PCF8574 lcd(0x27); // Change to 0x27 if 0x3F doesn't work

/****************************************************************************************************
 * setup() function
 ****************************************************************************************************/
void setup()
{
  //--------------------------------------------------
  Serial.begin(9600);

  // Initialize the LCD
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Initializing  ");
  for (int a = 5; a <= 10; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(500);
  }
  
  //--------------------------------------------------
  // WiFi Connectivity
  Serial.println();
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //--------------------------------------------------
  pinMode(BUZZER, OUTPUT);
  SPI.begin();
}

/****************************************************************************************************
 * loop() function
 ****************************************************************************************************/
void loop()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Scan your Card ");
  mfrc522.PCD_Init();
  if (!mfrc522.PICC_IsNewCardPresent()) {return;}
  if (!mfrc522.PICC_ReadCardSerial()) {return;}

  Serial.println(F("Reading last data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);

  Serial.println();
  Serial.print(F("Last data in RFID:"));
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Hello ");
  lcd.print(String((char*)readBlockData));
  
  digitalWrite(BUZZER, HIGH);
  delay(200);
  digitalWrite(BUZZER, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();
    card_holder_name = sheet_url + String((char*)readBlockData);
    card_holder_name.trim();
    Serial.println(card_holder_name);

    HTTPClient https;
    if (https.begin(*client, card_holder_name)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        lcd.setCursor(0, 1);
        lcd.print(" Data Recorded ");
        delay(2000);
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

/****************************************************************************************************
 * ReadDataFromBlock() function
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
}
