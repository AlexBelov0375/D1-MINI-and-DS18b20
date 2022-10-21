#include <ESP8266WiFi.h> //Подключение библиотеки для работы с сетью
#include <DallasTemperature.h> //Подключение библиотеки для работы с датчиком DS18B20

char ssid[] = "Home"; // Идентификатор вашей сети wifi
char password[] = "Password"; //пароль вашей сети wifi

#define ONE_WIRE_BUS 0 //D3 пин на плате (Линия данных для DS18B20)
#define DS18B20_POWER 5 //D1 пин на плате (Питание для DS18B20 для ручного управляения питанием датчика)

// создаем экземпляр класса oneWire; с его помощью 
// можно коммуницировать с любыми девайсами, работающими 
// через интерфейс 1-Wire, а не только с температурными датчиками
// от компании Maxim/Dallas:
OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature DS18B20(&oneWire);  
WiFiClient client;
IPAddress ip(192, 168, 1, 8); //Настройка статического IP адреса в локальной сети
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void toWifi() {
  WiFi.begin(ssid, password); //Подключение к роутеру
  WiFi.config(ip, gateway, subnet); //Задание настроек статического подключения
}

float getTemperature() {
  float tempC;
  do {
    DS18B20.requestTemperatures(); //Запрос измерения температуры
    delay(100); 
    //Задерка для преобразования
    //9-бит 93,75мс
    //10-бит 185,5мс
    //11-бит 375мс
    //12-бит 750мс
    tempC = DS18B20.getTempCByIndex(0);
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
      buf = buf + "##\r\n"; // закрываем пакет
      while (WiFi.status() != WL_CONNECTED) { //До этого датчик пытался подключится и только тут делаем проверку для оптимизации времени
         delay(30);
      }
      client.connect("185.245.187.136", 8283); //Подключаемся
      client.print(buf); // И отправляем данные
      client.stop();
      client.flush();
}
void setup() {
  pinMode(DS18B20_POWER, OUTPUT); //Задаём пин для питания DS18B20
  digitalWrite(DS18B20_POWER, HIGH); //Включаем его питание
  toWifi();
  DS18B20.begin(); // по умолчанию разрешение датчика – 9-битное;
                   // если у вас какие-то проблемы, его имеет смысл
                   // поднять до 12 бит; если увеличить задержку, 
                   // это даст датчику больше времени на обработку
                   // температурных данных
  //sensor.setResolution(12);
  float tempC = getTemperature();
  data(tempC);
  digitalWrite(DS18B20_POWER, LOW); //Отключаем питание датчика перед сном
  ESP.deepSleep(315e6);//Сон на 5 минут (в вашем случае может быть другая погрешность таймера)
}

void loop() {
}
