
// Demonstrate the use of WiFi.shutdown() and WiFi.resumeFromShutdown()
// Released to public domain

// Current on WEMOS D1 mini (including: LDO, usbserial chip):
// ~85mA during normal operations
// ~30mA during wifi shutdown
//  ~5mA during deepsleep

#ifndef STASSID
#define STASSID "Home"
#define STAPSK ""
#endif

#ifndef RTC_USER_DATA_SLOT_WIFI_STATE
#define RTC_USER_DATA_SLOT_WIFI_STATE 33u
#endif

#define ONE_WIRE_BUS 0 //D3 пин на плате (Линия данных для DS18B20)
#define DS18B20_POWER 5 //D1 пин на плате (Питание для DS18B20 для ручного управляения питанием датчика)

#include <ESP8266WiFi.h>
#include <include/WiFiState.h> // WiFiState structure details
#include <DallasTemperature.h> //Подключение библиотеки для работы с датчиком DS18B20

// создаем экземпляр класса oneWire; с его помощью 
// можно коммуницировать с любыми девайсами, работающими 
// через интерфейс 1-Wire, а не только с температурными датчиками
// от компании Maxim/Dallas:
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature DS18B20(&oneWire); 

WiFiState state;
WiFiClient client;

const char* ssid = STASSID;
const char* password = STAPSK;
unsigned long duration; //Время подключения

float getTemperature() {
  float tempC;
  do {
    DS18B20.requestTemperatures(); //Запрос измерения температуры
    tempC = DS18B20.getTempCByIndex(0);
    Serial.printf("Temp: %f", tempC);
    Serial.println();    
  } while (tempC == 85.0 || tempC == (-127.0)); //В случае некорректных показаний
  return tempC;
}

void data(float tempC) {
      DeviceAddress tempDeviceAddress; // Переменная для получения адреса датчика
      String buf; // Буфер для отправки на NarodMon
      buf="#ESP" + WiFi.macAddress()+ "\r\n";buf.replace(":", ""); //Формируем заголовок
      DS18B20.getAddress(tempDeviceAddress, 0); // Получаем уникальный адрес датчика
      buf = buf + "#";
      for (uint8_t i = 0; i < 8; i++) {if (tempDeviceAddress[i] < 16) buf = buf + "0"; buf = buf + String(tempDeviceAddress[i], HEX);} //Адрес датчика в буфер
      buf = buf + "#" + String(tempC)+ "\r\n"; //и температура
      buf = buf + "#Duration" + "#" + String(duration * 0.001)+ "\r\n"; //и задержка
      buf = buf + "##\r\n"; // закрываем пакет
      client.connect("185.245.187.136", 8283); //Подключаемся
      client.print(buf); // И отправляем данные
      client.stop();
      client.flush();
}

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);  // If you need debug output
  Serial.println();
  Serial.println("Trying to resume WiFi connection...");

  // May be necessary after deepSleep. Otherwise you may get "error: pll_cal exceeds 2ms!!!" when trying to connect
  delay(1);

  // ---
  // Here you can do whatever you need to do that doesn't need a WiFi connection.
  // ---
  pinMode(DS18B20_POWER, OUTPUT); //Задаём пин для питания DS18B20
  digitalWrite(DS18B20_POWER, HIGH); //Включаем его питание
  DS18B20.begin(); // по умолчанию разрешение датчика – 9-битное;
                   // если у вас какие-то проблемы, его имеет смысл
                   // поднять до 12 бит; если увеличить задержку, 
                   // это даст датчику больше времени на обработку
                   // температурных данных
  float tempC = getTemperature();
  
  ESP.rtcUserMemoryRead(RTC_USER_DATA_SLOT_WIFI_STATE, reinterpret_cast<uint32_t *>(&state), sizeof(state));
  unsigned long start = millis();

  if (!WiFi.resumeFromShutdown(state)
      || (WiFi.waitForConnectResult(10000) != WL_CONNECTED)) {
    Serial.println("Cannot resume WiFi connection, connecting via begin...");
    WiFi.persistent(false);

    if (!WiFi.mode(WIFI_STA)
        || !WiFi.begin(ssid, password)
        || (WiFi.waitForConnectResult(10000) != WL_CONNECTED)) {
      WiFi.mode(WIFI_OFF);
      Serial.println("Cannot connect!");
      Serial.flush();
      ESP.deepSleep(10e6, RF_DISABLED);
      return;
    }
  }

  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  duration = millis() - start;
  Serial.printf("Duration: %f", duration * 0.001);
  Serial.println();

  // ---
  // Here you can do whatever you need to do that needs a WiFi connection.
  // ---
  data(tempC);

  WiFi.shutdown(state);
  ESP.rtcUserMemoryWrite(RTC_USER_DATA_SLOT_WIFI_STATE, reinterpret_cast<uint32_t *>(&state), sizeof(state));

  // ---
  // Here you can do whatever you need to do that doesn't need a WiFi connection anymore.
  // ---
  digitalWrite(DS18B20_POWER, LOW); //Отключаем питание датчика перед сном
  
  Serial.println("Done.");
  Serial.flush();
  ESP.deepSleep(315e6, RF_DISABLED);
}

void loop() {
  // Nothing to do here.
}
