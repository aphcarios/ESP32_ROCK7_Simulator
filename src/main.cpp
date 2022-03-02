#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#define WIFI_SSID "Aphcarios"
#define WIFI_PASSWORD "aphcarios2019"

WiFiClient client;
HTTPClient http;

void initWiFi();
void processGateway(String message);
void replyToGateway(void);
int postData(String data);

bool binarySession = false;
String data = "";
int dataLength = 0;

String api = "http://159.223.77.171:8080/api/whiskers/Device/test/6e35c4d4-9e1e-4e84-bcae-fe762cc1e0c3";

int startCounter = 0;

void setup()
{
  Serial.begin(9600);
  Serial2.begin(19200);

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

void processGateway(String message)
{
  if (!binarySession)
  {
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

      postData(data);
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
    data = message.substring(0, dataLength);
    Serial2.print("\r\n0\r\n");
    Serial2.print("\r\nOK\r\n");
    Serial.print("Data: ");
    Serial.println(data);
    Serial.println("Reply: OK\n");
  }
}

int postData(String data)
{
  String requestBody = "[{\"data\":\"" + data + "\"}]";

  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode > 0)
  {
    String response = http.getString();

    Serial.println(httpResponseCode);
    Serial.println(response);
  }

  return httpResponseCode;
}