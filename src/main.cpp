#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID "Aphcarios"
#define WIFI_PASSWORD "aphcarios2019"

WiFiClient client;
HTTPClient http;
PubSubClient mqttClient(client);

void initWiFi();
void initMQTT(String token);
void processGateway(char message[]);
void replyToGateway(void);
int postData(String data);

bool binarySession = false;
int dataLength = 0;

String api = "http://192.168.1.129:8080/api/whiskers/Device/7lE8dNiruTufKHhOBHhs";
String hexData = "";

int startCounter = 0;

void setup()
{
  Serial.begin(9600);
  Serial2.begin(9600);

  initWiFi();
  http.begin(api);
  http.addHeader("Content-Type", "application/json");
}

void loop()
{
  char message[128] = "";
  char temp;
  for (int i = 0; i < sizeof(message); i++)
  {
    while (!Serial2.available())
    {
      if (millis() - startCounter == 1000)
      {
        if (binarySession)
        {
          binarySession = false;
        }
      }
    }
    Serial2.readBytes(&temp, 1);

    if (!binarySession)
    {
      if (isAscii(temp))
      {
        if (temp == 0x0D)
        {
          message[i] = 0x00;
          break;
        }
        message[i] = temp;
      }
    }
    else if (binarySession)
    {
      if (temp == 0x0D)
      {
        break;
      }
      message[i] = temp;
    }
  }

  processGateway(message);
}

void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }

  Serial.print("\nCONNECTED...");
  Serial.println(WiFi.localIP());
}

void processGateway(char msg[])
{
  if (!binarySession)
  {
    String message = msg;
    if (message == "AT" || message == "AT&K0")
    {
      Serial.println("Received: " + message);

      Serial2.print("\r\nOK\r\n");
      Serial.println("Reply: OK\n");
    }
    else if (message.substring(0, 9) == "AT+SBDWB=")
    {
      Serial.println("Received: " + message);

      binarySession = true;
      dataLength = message.substring(9).toInt();
      Serial.print("Data Length: ");
      Serial.println(dataLength);
      Serial2.print("\r\nREADY\r\n");
      Serial.println("Reply: READY\n");

      startCounter = millis();
    }
    else if (message == "AT+SBDIX")
    {
      Serial.println("Received: " + message);

      postData(hexData);
      Serial2.print(F("\r\n+SBDIX= 0, 0, 0, 0, 0, 0\r\n"));
      Serial.println("Reply: +SBDIX= 0, 0, 0, 0, 0, 0\n");
    }
    else
    {
    }
  }
  else
  {
    binarySession = false;

    char data[] = {0x00, 0x00, 0x00};

    hexData = "";

    Serial.println("Entering For Loop");

    for (int i = 0; i < dataLength; i++)
    {
      sprintf(data, "%02X", msg[i]);
      hexData = hexData + String(data);
    }

    Serial2.print("\r\n0\r\n");
    Serial2.print("\r\nOK\r\n");
    Serial.print("Data: ");
    Serial.println(hexData);
    Serial.println("Reply: OK\n");
  }
}

int postData(String data)
{
  String requestBody = "[{\"data\":\"" + data + "\"}]";
  Serial.println(requestBody);
  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0)
  {
    String response = http.getString();

    Serial.println(httpResponseCode);
    Serial.println(response);
  }

  return httpResponseCode;
}