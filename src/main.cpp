// bme280

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiClient espClient;
PubSubClient client(espClient);

//------------------------------------------------
const char *ssid = "link";
// const char *ssid = "MkroTik-2";
const char *pass = "dkfgf#*12091997";

const char *mqtt_client = "septic-001";
const char *mqtt_client2 = "Villa_septic";
const char *mqtt_user = "mqtt";
const char *mqtt_pass = "qwe#*1243";
const char *mqtt_server = "178.20.46.157";
const char *mqtt_port = "1883";

const char *outTopicTemp = "/Compressor";

const uint8_t pinRelay = 3;
const uint32_t utcOffsetSeconds = 10800;
const uint8_t timeBeginning = 23;
const uint8_t timeEnd = 7;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp3.vniiftri.ru", utcOffsetSeconds);

//------------------------------------------------
uint8_t countConnect = 20;        //  кол-во попыток соединения
uint16_t countPause = 500;        //  пауза между попытками
uint32_t timeSleep = 30000000;    //  время сна
uint16_t TimeBeforeBedtime = 500; //  время до засыпания
uint8_t countMaxSleep = 120;      //  не передавать данные не более
                                  //      countMaxSleep х timeSleep (60мин)

RTC_DATA_ATTR struct
{
  uint8_t c = 0;          //  компрессор
  uint8_t countSleep = 0; //  счетчик предельного кол-ва циклов сна
} data;

RTC_DATA_ATTR bool flagNotWork = false;

//-----------------------------------
inline bool mqtt_subscribe(PubSubClient &client, const String &topic)
{
  Serial.print("Subscribing to: ");
  Serial.println(topic);
  return client.subscribe(topic.c_str());
}

//-----------------------------------
inline bool mqtt_publish(PubSubClient &client, const String &topic, const String &value)
{
  Serial.print(topic);
  Serial.print(" = ");
  Serial.println(value);
  return client.publish(topic.c_str(), value.c_str());
}

//-----------------------------------
void mqttDataOut(uint8_t work)
{
  String topic = "/";
  topic += mqtt_client2;
  topic += outTopicTemp;
  Serial.print("Mqtt data out: ");
  while (!mqtt_publish(client, topic, (String)work) && countConnect--)
  {
    Serial.print(countConnect);
    Serial.print('>');
    delay(countPause);
  }
  Serial.println();
}

//-----------------------------------
bool reconnect()
{
  client.setServer(mqtt_server, String(mqtt_port).toInt());

  Serial.print("MQTT connect : ");
  Serial.println(mqtt_server);

  while (!(client.connect(mqtt_client, mqtt_user, mqtt_pass)) && countConnect--)
  {
    Serial.print(countConnect);
    Serial.print('>');
    delay(countPause);
  }

  if (client.connected())
  {
    Serial.println("MQTT connected - OK !");
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------
bool setupWiFi(const char *wifi_ssid, const char *wifi_pass)
{
  WiFi.begin(wifi_ssid, wifi_pass);

  Serial.println("=================");
  Serial.print("Setup WiFi: ");
  Serial.println(ssid);

  while ((WiFi.status() != WL_CONNECTED) && countConnect--)
  {
    Serial.print(countConnect);
    Serial.print('>');
    delay(countPause);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    // индикация IP
    Serial.print("\nWiFi connected - OK !\n");
    Serial.println(WiFi.localIP());
    // индикация силы сигнала
    int8_t dBm = WiFi.RSSI();
    Serial.print("RSSI dBm = ");
    Serial.println(dBm);
    uint8_t quality_RSSI = 2 * (dBm + 100);
    if (quality_RSSI >= 100)
      quality_RSSI = 100;
    Serial.print("RSSI % = ");
    Serial.println(quality_RSSI);
    Serial.println("-----------------");
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------
bool work()
{
  timeClient.begin();
  timeClient.update();
  String str = "";
  Serial.println();

  str += timeClient.getFormattedTime();
  Serial.println(str);

  if (str.length() < 10)
  {
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println(millis());
  pinMode(pinRelay, OUTPUT);
  digitalWrite(pinRelay, LOW);

  if (!setupWiFi(ssid, pass))
  {
    flagNotWork = true;
    esp_deep_sleep(timeSleep);
  }
  else
  {
    flagNotWork = false;
  }

  if (!reconnect())
  {
    flagNotWork = true;
    esp_deep_sleep(timeSleep);
  }
  else
  {
    flagNotWork = false;
  }

  if (!work())
  {
   flagNotWork = true;
    esp_deep_sleep(timeSleep);
  }
  else
  {
    flagNotWork = false;
  }

  // mqttDataOut(data.c);

  Serial.println("=================");
  Serial.println(millis());
  Serial.flush();

  delay(TimeBeforeBedtime);
  esp_deep_sleep(timeSleep);
}

void loop() {}