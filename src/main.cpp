//********************************************************************
//*
//*   Tablo na WS2812b + mqtt + bme280 (2)
//*   2021.02.01 - 2024.06.10
//*
//*   A - градус
//*   В - минус
//*   С - двоеточие
//*   D - пусто
//*   E - точка
//*   F -
//********************************************************************
#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include "tablo_ws2812.h"

#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
const char *host = "esp32";

WebServer server(80);

const uint8_t TIME_DAY = 8;
const uint8_t TIME_NIGHT = 19;
// const uint8_t PIN_VCC_bme280 = 14;  // Wemos D1mini = D5,

const char ssid[] = "MikroTik-2";
// const char ssid[] = "MikroTik-2-ext";
// char ssid[] = "link";
char pass[] = "dkfgf#*12091997";
// char ssid[] = "Huawei-M2";
// char pass[] = "998877**";

const uint32_t utcOffsetInSeconds = 10800;
const uint32_t utcPeriodMseconds = 86400000; // 3сутки-259200000 // 1сутки-86400000;
WiFiUDP ntpUDP;
const char timeSite[] = "ntp3.vniiftri.ru"; //  "pool.ntp.org"
NTPClient timeClient(ntpUDP, timeSite, utcOffsetInSeconds, utcPeriodMseconds);

const char *mqtt_server = "178.20.46.157";
const uint16_t mqtt_port = 1883;
const char *mqtt_user = "mqtt";
const char *mqtt_pass = "qwe#*1243";

// const char *mqtt_client = "Tablo_Villa_011";
const char *mqtt_client = "Tablo_Villa_003";
const char *mqtt_client2 = "Villa_bme280_base";
// const char *mqtt_client2 = "Home_bme280";
const char *mqtt_client3 = "Villa_bme280_yama";

const char *inTopicTemp = "/Temp";
const char *inTopicPres = "/Pres";
const char *inTopicHum = "/Hum";
const char *inTopicTempOut = "/Temp";
const char *inTopicHumOut = "/Hum";

String outTempData = "00.00A";
String outTempData2 = "00.00A";
String outTempDataOut = "00.00A";
String outPresData = "000";
String outHumData = "00";
String outHumDataOut = "00";

WiFiClient espClient;
PubSubClient client(espClient);

uint32_t timeUpdateH = 02; //  время обновления NTP
uint32_t timeUpdateM = 05; //  время обновления NTP
String z = "";             //  строка данных для отображения

uint32_t colorTime = 16766720;   //  цвет часы (желтый)
uint32_t colorTemp = 16711935;   //  цвет температура (красный)
uint32_t colorTemp2 = 65280;     //  цвет внешняя температура (зеленый)
uint32_t colorTempOut = 9127187; //  цвет температура2 (коричневый)
uint32_t colorPress = 9830400;   //  цвет давление (розовый)
uint32_t colorWiFi = 16728935;   //  цвет WiFi (томато)
uint32_t colorHum = 52945;       //  цвет влажность (бирюзовый)
uint32_t colorHumOut = 255;      //  цвет влажность2 (синий)
uint16_t strOld = 0;
uint8_t count = 0;
uint8_t countReconnect = 20; //  кол-во попыток соединения
uint16_t countPause = 500;   //  пауза между попытками

uint32_t timeReadOld = 0;
uint32_t color = 0;

bool flagSec = true;
bool flagTemp = false;
bool flagTemp2 = false;
bool flagTempOut = false;
bool flagPress = false;
bool flagHum = false;
bool flagHumOut = false;
bool flagVis = true;

// Время отображения каждого показания:
// 0-время, 1-темп, 2-давл, 3-влажн, 4-темп2, 5-влажн2
uint8_t countData = 3;
uint16_t visData[] = {15, 5, 5, 5, 2, 2, 2};
uint32_t timeOld = 0;
uint8_t brightVis = BRIGHT_DAY;

//-----------------------------------
// Login page
const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

// Server Index Page
const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

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
    Serial.print("Publishing topic ");
    Serial.print(topic);
    Serial.print(" = ");
    Serial.println(value);
    return client.publish(topic.c_str(), value.c_str());
}

//-----------------------------------
//  Подключение к MQTT:
bool reconnect()
{
    uint8_t countMqtt = countReconnect;
    while (!(client.connect(mqtt_client, mqtt_user, mqtt_pass)) && countMqtt--)
    {
        visibleWork("0B0B0", colorWiFi, BRIGHT_DAY);
        // strip.show();
        delay(countPause / 2);
        visibleWork("B0B0B", colorWiFi, BRIGHT_DAY);
        // strip.show();
        delay(countPause / 2);
        Serial.print(countMqtt);
        Serial.print('>');
    }

    if (countMqtt)
    {
        Serial.println("MQTT connected - Ok !\n");

        String topic('/');
        topic += mqtt_client2;
        topic += inTopicTemp;
        mqtt_subscribe(client, topic);

        topic = '/';
        topic += mqtt_client2;
        topic += inTopicPres;
        mqtt_subscribe(client, topic);

        topic = '/';
        topic += mqtt_client2;
        topic += inTopicHum;
        mqtt_subscribe(client, topic);

        // topic = '/';
        // topic += mqtt_client3;
        // topic += inTopicTempOut;
        // mqtt_subscribe(client, topic);

        // topic = '/';
        // topic += mqtt_client3;
        // topic += inTopicHumOut;
        // mqtt_subscribe(client, topic);
        // Serial.println();

        visibleWork("BBBBB", colorWiFi, BRIGHT_DAY);
        // strip.show();

        return true;
    }
    else
    {
        visibleWork("BBBBB", colorWiFi, BRIGHT_DAY);
        // strip.show();
        return false;
    }
}

//-----------------------------------
//  Обновление времени:
void timeUpdate()
{
    Serial.print("Connected timeClient: ");
    Serial.println(timeSite);

    visibleWork("EBEBE", colorTime, BRIGHT_DAY);
    // strip.show();

    uint8_t countTime = countReconnect;

    while (!timeClient.update() && countTime--)
    {
        delay(countPause);
        Serial.print(countTime);
        Serial.print('.');
    }

    Serial.println();
    
    if (countTime)
    {
        Serial.println("The time has been updated !\n");
        Serial.println(timeClient.getFormattedTime());
        Serial.println();
    }
    else
    {
        Serial.println("The time has NOT been updated !!!\n");
    }
    visibleWork("BBBBB", colorTime, BRIGHT_DAY);
    // strip.show();
}

//********************************************************************
//  подключение к WiFi
bool setupWiFi()
{
    Serial.println("\n\nSetup WiFi: ");
    Serial.println(ssid);
    visibleWork("ABABA", colorWiFi, BRIGHT_DAY);
    // strip.show();

    WiFi.begin(ssid, pass);
    uint8_t countWiFi = countReconnect;

    while (WiFi.status() != WL_CONNECTED && countWiFi--)
    {
        Serial.print(countWiFi);
        Serial.print('>');
        delay(countPause);
    }

    if (countWiFi)
    {
        //-----------------------------------
        // индикация IP
        String l = WiFi.localIP().toString();
        l = l.substring(l.lastIndexOf('.') + 1, l.length());

        (l.toInt() < 100) ? l = "DDE" + l : l = "DE" + l;
        visibleWork(l, colorWiFi, BRIGHT_DAY);

        strip.show();
        Serial.print("\nWiFi connected !\n");
        Serial.println(WiFi.localIP());
        delay(2000);

        //-----------------------------------
        // индикация силы сигнала
        int16_t RSSI_MAX = -50;
        int16_t RSSI_MIN = -100;

        int16_t dBm = WiFi.RSSI();
        Serial.print("RSSI dBm = ");
        Serial.println(dBm);
        l = "DDE";
        (dBm <= RSSI_MIN) ? l += 0 : (dBm >= RSSI_MAX) ? l += 100
                                                       : l += 2 * (dBm + 100);
        Serial.print("RSSI % = ");
        Serial.println(l.substring(3, 5));
        Serial.println("\n");

        visibleWork(l, colorWiFi, BRIGHT_DAY);
        // strip.show();
        delay(2000);

        Serial.println();

        //-----------------------------------
        timeUpdate();

        if (!reconnect())
        {
            ESP.restart();
        }

        Serial.println();

        flagVis = true;
        count = 0;

        visibleWork("BBBBB", colorTemp, BRIGHT_DAY);
        // strip.show();

        return true;
    }
    else
    {
        visibleWork("BBBBB", colorTemp, BRIGHT_DAY);
        // strip.show();

        return false;
    }
}

//********************************************************************
//  MQTT
void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    // ------------------------
    // Serial.print(topic);
    // Serial.print("\t");
    // for (uint8_t i = 0; i < length; ++i)
    // {
    //     Serial.print((char)payload[i]);
    // }
    // Serial.println();
    // ------------------------

    char *topicBody = topic + strlen(mqtt_client3) + 1;
    // Serial.print("*topicBody  ");
    // Serial.println(topicBody);

    if (!strncmp(topic + 1, mqtt_client3, strlen(mqtt_client3)))
    {
        if (!strncmp(topicBody, inTopicTempOut, strlen(inTopicTempOut)))
        {
            outTempDataOut = "";
            for (uint8_t i = 0; i < length; i++)
                outTempDataOut += (char)payload[i];
            Serial.print("TempOut: ");
            Serial.println(outTempDataOut);
            Serial.println();
        }
        else if (!strncmp(topicBody, inTopicHumOut, strlen(inTopicHumOut)))
        {
            outHumDataOut = "";
            for (uint8_t i = 0; i < length; i++)
                outHumDataOut += (char)payload[i];
            Serial.print("HumOut: ");
            Serial.println(outHumDataOut);
            Serial.println();
        }
    }

    //------------------------

    // char *topicBody = topic + strlen(mqtt_client2) + 1;
    // *topicBody += *mqtt_client2;
    // if (!strncmp(topicBody, inTopicTemp2, strlen(inTopicTemp2)))
    // {
    //     outTempData2 = "";
    //     for (uint8_t i = 0; i < length; i++)
    //         outTempData2 += (char)payload[i];
    //     Serial.print("Temp2: ");
    //     Serial.println(outTempData2);
    // }
    // else
    if (!strncmp(topic + 1, mqtt_client2, strlen(mqtt_client2)))
    {

        if (!strncmp(topicBody, inTopicTemp, strlen(inTopicTemp)))
        {
            outTempData = "";
            for (uint8_t i = 0; i < length; i++)
                outTempData += (char)payload[i];
            Serial.print("Temp: ");
            Serial.println(outTempData);
        }
        else if (!strncmp(topicBody, inTopicPres, strlen(inTopicPres)))
        {
            outPresData = "";
            for (uint8_t i = 0; i < length; i++)
                outPresData += (char)payload[i];
            Serial.print("Pres: ");
            Serial.println(outPresData);
        }
        else if (!strncmp(topicBody, inTopicHum, strlen(inTopicHum)))
        {
            outHumData = "";
            for (uint8_t i = 0; i < length; i++)
                outHumData += (char)payload[i];
            Serial.print("Hum : ");
            Serial.println(outHumData);
            Serial.println();
        }
    }
}

//********************************************************************
//  пересчет millis()
void TimeMillis()
{
    color = colorTime;

    if (timeClient.getHours() < 10)
        z += "D";

    z += timeClient.getHours(); //  часы
    (flagSec) ? z += "C" : z += "D";

    if (timeClient.getMinutes() < 10)
        z += "0";

    z += timeClient.getMinutes(); //  минуты

    if ((timeClient.getHours() >= TIME_DAY) && (timeClient.getHours() < TIME_NIGHT))
    {
        brightVis = BRIGHT_DAY;
    }
    else
    {
        brightVis = BRIGHT_NIGHT;
    }
}

//********************************************************************
//  BME280
void bmeTemp()
{
    if (outTempData.charAt(0) == '-')
    {
        if (outTempData.indexOf('.') == 2)
        {
            z = "B" + outTempData.substring(1, outTempData.indexOf('.'));
            z += 'E' + outTempData.substring(3, 4);
        }
        else
        {
            z = "DB" + outTempData.substring(1, outTempData.indexOf('.'));
        }
    }
    else
    {
        if (outTempData.indexOf('.') == 2)
        {
            z = "DD" + outTempData.substring(0, outTempData.indexOf('.'));
        }
        else
        {
            z = 'D' + outTempData.substring(0, outTempData.indexOf('.'));
            z += 'E' + outTempData.substring(2, 3);
        }
    }
    outTempData.substring(0, 1);

    z += 'A';
    color = colorTemp;
    flagTemp = true;
}

// void bmeTempOut()
// {
//     if (outTempDataOut.charAt(0) == '-')
//     {
//         if (outTempDataOut.indexOf('.') == 2)
//         {
//             z = "B" + outTempDataOut.substring(1, outTempDataOut.indexOf('.'));
//             z += 'E' + outTempDataOut.substring(3, 4);
//         }
//         else
//         {
//             z = "DB" + outTempDataOut.substring(1, outTempDataOut.indexOf('.'));
//         }
//     }
//     else
//     {
//         if (outTempDataOut.indexOf('.') == 2)
//         {
//             z = "DD" + outTempDataOut.substring(0, outTempDataOut.indexOf('.'));
//         }
//         else
//         {
//             z = 'D' + outTempDataOut.substring(0, outTempDataOut.indexOf('.'));
//             z += 'E' + outTempDataOut.substring(2, 3);
//         }
//     }
//     outTempDataOut.substring(0, 1);
//     z += 'A';
//     color = colorTempOut;
//     flagTempOut = true;
// }

void bmePress()
{
    z += "DD" + outPresData.substring(0, 3);
    color = colorPress;
    flagPress = true;
}

void bmeHum()
{
    (outHumData.length() > 2) ? z = "DD" : z = "DDD";
    z += outHumData.substring(0, 2);
    color = colorHum;
    flagHum = true;
}

// void bmeHumOut()
// {
//     (outHumDataOut.length() > 2) ? z = "DD" : z = "DDD";
//     z += outHumDataOut.substring(0, 2);
//     color = colorHumOut;
//     flagHumOut = true;
// }
//********************************************************************
//  запись значений датчиков в память
// void dataWrite(uint8_t n, String str)
// {
//     data[n].num = str.substring(0, 1).toInt();
//     if (str.indexOf('%') > 0)
//         data[n].hum = str.substring(str.indexOf('%') + 1, str.indexOf('%') + 3).toInt();
//     if (str.indexOf('*') > 0)
//         data[n].temp = str.substring(str.indexOf('*') + 1, str.indexOf('*' + 5)).toFloat();
//     if (str.indexOf('$') > 0)
//         data[n].press = str.substring(str.indexOf('$') + 1, str.indexOf('$') + 4).toInt();
//     if (str.indexOf('^') > 0)
//         data[n].voltage = str.substring(str.indexOf('^') + 1, str.indexOf('^') + 4).toInt();
// }
//********************************************************************
// void parserBuf(String tmp)
// {
//     if (tmp.substring(0, 2).toInt() == sensorNum_1)
//     {
//         dataWrite(0, tmp);
//     }
//     // else if (tmp.substring(0, 2).toInt() == sensorNum_2)
//     // {
//     //     dataWrite(1, tmp);
//     // }
// }
//********************************************************************
//  визуальные эффекты
// void show_1()
// {
//     strip.clear();
//     for (uint8_t i = 0; i < PIXEL_COUNT / 2; ++i)
//     {
//         strip.setPixelColor(i, strip.Color(random(1, 255), random(1, 255), random(1, 255)));
//         strip.setPixelColor(PIXEL_COUNT - i - 1, strip.Color(random(1, 255), random(1, 255), random(1, 255)));
//         strip.show();
//         delay(5);
//     }
//     delay(500);
//     for (uint8_t i = 0; i < PIXEL_COUNT / 2; ++i)
//     {
//         strip.setPixelColor(i, 0, 0, 0);
//         strip.setPixelColor(PIXEL_COUNT - i - 1, 0, 0, 0);
//         strip.show();
//         delay(5);
//     }
//     delay(500);
//     z = "DDDDD";
// }

//********************************************************************
void setup()
{
    Serial.begin(115200);

    strip.begin();
    z = "DDDDD";
    strip.show();

    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqtt_callback);

    setupWiFi();
    timeClient.begin();

    /*use mdns for host name resolution*/
    if (!MDNS.begin(host))
    { // http://esp32.local
        Serial.println("Error setting up MDNS responder!");
        while (1)
        {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started\n");
    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, []()
              {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex); });
    server.on("/serverIndex", HTTP_GET, []()
              {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex); });
    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, []()
              {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []()
              {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });

    server.begin();

    timeOld = millis();
}

//********************************************************************
void loop()
{
    if (WiFi.status() != WL_CONNECTED || (!client.connected()))
    {
        setupWiFi();
    }

    if (flagVis)
    {
        z = "";
        switch (count)
        {
        case 0:
        {
            TimeMillis();
            flagTemp = false;
            flagPress = false;
            flagHum = false;
            flagTempOut = false;
            flagHumOut = false;
            break;
        }
        case 1:
        {
            if (flagTemp == false)
                bmeTemp();
            break;
        }
        case 2:
        {
            if (flagPress == false)
                bmePress();
            break;
        }
        case 3:
        {
            if (flagHum == false)
                bmeHum();
            break;
        }
            // case 4:
            // {
            //     if (flagTempOut == false)
            //         bmeTempOut();
            //     break;
            // }
            // case 5:
            // {
            //     if (flagHumOut == false)
            //         bmeHumOut();
            //     break;
            // }
        }
        flagVis = false;
        visibleWork(z, color, brightVis);
        Serial.println(z);
        // timeReadOld = millis();
    }

    //  Отображение секундного тире
    if (millis() - timeReadOld >= 500 && (!count))
    {
        flagSec = !flagSec;
        timeReadOld = millis();

        flagVis = true;
    }

    // Счетчик отображаемых данных
    if (millis() - timeOld >= (visData[count] * 1000))
    {
        (count >= countData) ? count = 0 : count++;
        flagVis = true;
        timeOld = millis();
    }

    client.loop();
    server.handleClient();
    delay(1);
}

//********************************************************************