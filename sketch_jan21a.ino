  //  Аверьянов Н. Н.
  //  21.01.2021
  
  //  1. +Читаем память на предмет SSID и пароля
  //  2. Если есть SSID и пароль переходим к шагу 4
  //  3. Если нет SSID и пароль переходим к шагу 10
  //  4. +Подключаемся к Wi-Fi
  //  5. Если подключились переходим к шагу 20
  //  6. Если не подключились переходим к шагу 10
  //  10. +Запускаем точку доступа со стандартным SSID и пароль (temp_404)
  //  11. Переходим к шагу 20
  //  15. +Пишем в энергонезависимую память
  //  16. +Опрашиваем датчик температуры
  //  20. +Запускаем сервер на прослушку и ждем запроса
  //      +Запрос на данные:
  //          /json
  //          опрашиваем датчик температуры и выдаем данные (json)
  //      +Запрос на страницу изменения данных:
  //          /settings
  //          заполнение формы SSID и пароль
  //      +Запрос на сохранение:
  //          /save-ssid
  //          Пишем в энергонезависимую память
  //      +Главная страница:
  //          /
  //          Опрашиваем датчик температуры
  //          Показываем данные с датчика
  //          Ссылка на страницу настроек

#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 aht;

// 1, 15
#define START_SSID_ADDR 256
#define START_PASSWORD_ADDR 320
#define SSID_LEN 32
#define PASSWORD_LEN 32

struct NetworkSSID {
  char ssid[SSID_LEN];
  char password[PASSWORD_LEN];
};

NetworkSSID network;

// 1
NetworkSSID read_SSID() {
  NetworkSSID rNetwork;
  
  for(int i = 0; i < SSID_LEN; i++) {
    rNetwork.ssid[i] = EEPROM.read(START_SSID_ADDR + i);
  }
  for(int i = 0; i < PASSWORD_LEN; i++) {
    rNetwork.password[i] = EEPROM.read(START_PASSWORD_ADDR + i);
  }
//  EEPROM.get(START_SSID_ADDR, rNetwork);
  return rNetwork;
}
// 15
void write_SSID(NetworkSSID _network) {
  for(int i = 0; i < SSID_LEN; i++) {
    EEPROM.write(START_SSID_ADDR + i, _network.ssid[i]);
  }
  for(int i = 0; i < PASSWORD_LEN; i++) {
    EEPROM.write(START_PASSWORD_ADDR + i, _network.password[i]);
  }
  EEPROM.commit();
}
//

// 16
sensors_event_t humidity, temp;
void getDataSensor() {
  aht.getEvent(&humidity, &temp);
}
//

// 4
int connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(network.ssid, network.password);
  //Serial.println("");

  int count_connect = 0;
  // Wait for connection
  while ((WiFi.status() != WL_CONNECTED) && count_connect < 128) {
    delay(500);
    //Serial.print(".");
    count_connect++;
  }
  if(WiFi.status() == WL_CONNECTED) {
    //Serial.println("");
    //Serial.print("Connected to ");
    //Serial.println(network.ssid);
    //Serial.print("IP address: ");
    //Serial.println(WiFi.localIP());
    return 0;
  }
  if(WiFi.status() != WL_CONNECTED) {
    //Serial.println("");
    //Serial.print("Do not connect");
    return 1;
  }
  return 2;
}
//

// 20
  ESP8266WebServer server(80);
  void server_router() {
    server.on("/", handleRoot);
    server.on("/json", handleJson);
    server.on("/settings", handleSettings);
    server.on("/save-ssid", handleSaveSSID);
    server.onNotFound(handleNotFound);
  }

  const String head = "<!DOCTYPE html>\
    <html lang=\"en\">\
    <head>\
        <meta charset=\"UTF-8\">\
        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
        <title>&#127777; Temperature & Humidity</title>\
        <style>\
            .text-center {text-align: center;}\
            .text-right {text-align: right;}\
            select, input { width: 100%; font: 1.2em sans-serif;}\
            button { font: 1.2em sans-serif; }\
            table {border: 0; width: 100%;}\
            .mt1 { margin-top: 1rem; }\
        </style>\
    </head>\
    <body>";
  const String foot = "</body>\
    </html>";

  String root() {
    getDataSensor();
    String resStr = "<h2 class=\"text-center\">Temperature & Humidity</h2>\
      <h3 class=\"text-center\"><a href=\"/settings\">&#128423; Network settings</a></h3>\
      <h3 class=\"text-center\">&#127777; Sensor information</h3>\
      <p>Temperature: " + (String)temp.temperature + "&deg;С</p>\
      <p>Humidity: " + (String)humidity.relative_humidity + "% rH</p>";
    return resStr;
  }

  const String saved = "<h2 class=\"text-center\">Temperature & Humidity</h2>\
    <h3 class=\"text-center\"><a href=\"/\">&#127777; Sensor information</a></h3>\
    <h3 class=\"text-center\">&#128427; Network data was saved</h3>\
    <p class=\"text-center\">Reboot the device for connection to new network.</p>";

  String settings() {
    int n = WiFi.scanNetworks();
    String wscan = "";
    if (n > 0) {
      for (int i = 0; i < n; ++i) {
        wscan += "<option value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</option>";
      }
    }
     String resStr = "<h2 class=\"text-center\">Temperature & Humidity</h2>\
      <h3 class=\"text-center\"><a href=\"/\">&#127777; Sensor information</a></h3>\
      <h3 class=\"text-center\">&#128423; Network settings</h3>\
      <p class=\"text-center\">Select SSID and enter password</p>\
      <form action=\"/save-ssid\" enctype=\"application/x-www-form-urlencoded\" method=\"post\">\
          <table>\
              <tr>\
                  <td>SSID:</td>\
                  <td>\
                      <select name=\"ssid\" id=\"ssid\">" + wscan + "\
                      </select>\
                  </td>\
              </tr>\
              <tr>\
                  <td>Password:</td>\
                  <td><input type=\"password\" name=\"password\" id=\"pass\"></td>\
              </tr>\
          </table>\
          <div class=\"text-center mt1\">\
              <button type=\"submit\">&#128427; Save network</button>\
          </div>\
      </form>";
      
    return resStr;
  }
  
  void handleRoot() {
    server.send(200, "text/html", head + root() + foot);
  }

  void handleJson() {
    getDataSensor();
    server.send(200, "application/json; charset=UTF-8", "{\"temperature\":\"" + (String)temp.temperature + "\", \"humidity\":\"" + (String)humidity.relative_humidity + "\"}");
  }

  void handleSettings() {
    server.send(200,  "text/html", head + settings() + foot);
  }

  void handleSaveSSID() {
    if (server.method() != HTTP_POST) {
      handleRoot();
    }
    if((server.arg("ssid") == "") || (server.arg("password") == "") || (server.arg("password").length() < 8)) {
      handleSettings();
    }
    NetworkSSID newNetwork;
    server.arg("ssid").toCharArray(newNetwork.ssid, SSID_LEN);
    server.arg("password").toCharArray(newNetwork.password, PASSWORD_LEN);
    write_SSID(newNetwork);
    delay(200);
    server.send(200, "text/html", head + saved + foot);
  }
  
  void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
  }
//

// 10
#define APSSID "ESP-12F-TEMP-AP"
#define APPSK  "12345678"

void create_AP() {
  //Serial.println();
  //Serial.print("Configuring access point...");
  IPAddress local_ip(192, 168, 120, 1);
  IPAddress gateway(192, 168, 120, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAP(APSSID, APPSK);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  //IPAddress myIP = WiFi.softAPIP();
  //Serial.print("AP IP address: ");
  //Serial.println(myIP);
}
//

void setup() {
  //Serial.begin(115200);
  
  EEPROM.begin(512);
  //Serial.println("Starting...");
  delay(1000);
  //Serial.println("Reading...");
  network = read_SSID();
  //Serial.println("Writing...");
  //write_SSID(network);
  delay(100);

  aht.begin();
//  if (!aht.begin()) {
//    //Serial.println("Could not find AHT? Check wiring");
//    
//    while (1) delay(10);
//  }
  //Serial.println("AHT10 OK...");
  //Serial.print("SSID:");
  //Serial.println(network.ssid);
  //Serial.print("Password:");
  //Serial.println(network.password);

  if(connect_wifi() == 0) {
    if (MDNS.begin("temp404")) {
      //Serial.println("MDNS responder started");
    }
  } else {
    //Serial.println("WiFi disconnecting");
    WiFi.disconnect();
    delay(100);
    //Serial.println("WiFi disconnect");
    create_AP();
  }
  
  server_router();
  server.begin();
}

void loop() {
  server.handleClient();
  //MDNS.update();
}
