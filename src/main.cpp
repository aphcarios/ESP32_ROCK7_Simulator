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
void processGateway(String message);
void replyToGateway(void);
void postData(char data[]);

bool binarySession = false;
char data[5] = "";
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
  char mydata[5] = "01t9";
  postData(mydata);
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
    // data = message.substring(0, dataLength);
    message.substring(0, dataLength).toCharArray(data, dataLength);
    Serial2.print("\r\n0\r\n");
    Serial2.print("\r\nOK\r\n");
    Serial.print("Data: ");
    Serial.println(data);
    Serial.println("Reply: OK\n");
  }
}

void callback(char* topic, byte* message, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;
    
    for (int i = 0; i < length; i++) {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println(messageTemp);

    if(String(topic) == "tracking/actual_position"){
        
    }
}

void initMQTT(String token){
  // MQTT Broker
  const char *mqttBroker = "134.122.92.235";
  // const char *mqttBroker = "whiskershub.alpha.aphcarios.com";
  const char *mqttClientId = "esp32Sim";
  const int mqttPort = 1883;
  const char *user = "BrNjFhYr9g7kbTrw43g7"; 
  const char *password = ""; 

  mqttClient.setServer(mqttBroker, mqttPort);
  mqttClient.setCallback(callback);

  while (!mqttClient.connected()) {
    Serial.printf("The client %s connects to the public mqtt broker\n", mqttClientId);
    if (mqttClient.connect(mqttClientId, token.c_str(), password)) {
        Serial.println("Public emqx mqtt broker connected");
        mqttClient.subscribe("tracking/actual_position");
    } else {
        Serial.print("failed with state ");
        Serial.print(mqttClient.state());
        delay(2000);
    }
  }
}
void postData(char data[])
{
  String device = "";
  String accessToken = "";
  char sender = data[0] - '0';
  Serial.println("Sender " + String(sender));

  if(sender == 0){
    device = "S0006";
    accessToken = "scscscdwrrerer";
  }
  else if (sender == 1){
    device = "PV0010";
    accessToken = "BrNjFhYr9g7kbTrw43g7";
  }
  else
    return;

  initMQTT(accessToken);
  Serial.println("Device " + device);
  Serial.println("Token " + accessToken);

  bool trigger;

  if(data[1] - '0' == 0) trigger = false;
  else if(data[1] - '0' == 1) trigger = true;

  String deviceJson = "{\"device\":" + device + "}";
  String telemetryJson =  "{\"trigger\":" + String(trigger) + 
                          ",\"temperature\":" + String((int)data[2]) +
                          ",\"battery_level\":" + String((int)data[3]) + 
                          "}";
  
  bool res = mqttClient.publish("v1/gateway/connect", deviceJson.c_str());
  Serial.printf("Device connected? %i\n", res);

  res = mqttClient.publish("v1/devices/me/telemetry", telemetryJson.c_str());
  Serial.printf("Data sent? %i\n", res);

  res = mqttClient.publish("v1/gateway/disconnect", deviceJson.c_str());
  Serial.printf("Device disconnected? %i\n", res);

  if (mqttClient.connected()) mqttClient.disconnect();
}

// int postData(char data[5])
// {
//   String requestBody = "[{\"data\":\"" + data + "\"}]";
//   Serial.println(data);
//   int httpResponseCode = http.POST(requestBody);

//   if (httpResponseCode > 0)
//   {
//     String response = http.getString();

//     Serial.println(httpResponseCode);
//     Serial.println(response);
//   }

//   return httpResponseCode;
// }