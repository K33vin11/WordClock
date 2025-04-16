#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <time.h>

// WiFi credentials
const char* ssid = "WLAN";
const char* password = "Password";

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 200);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ch.pool.ntp.org", 3600, 60000);

// Neopixel setup
#define LED_PIN D2
#define LED_COUNT 121
#define ROW_LENGTH 11

// Definitionen für die Speicheradressen der RGB-Werte
#define RED_VALUE_TIME_ADDRESS 0
#define GREEN_VALUE_TIME_ADDRESS 1
#define BLUE_VALUE_TIME_ADDRESS 2
#define RED_VALUE_SPECIAL_ADDRESS 3
#define GREEN_VALUE_SPECIAL_ADDRESS 4
#define BLUE_VALUE_SPECIAL_ADDRESS 5
#define TimeOnHour_ADRESS 6
#define TimeOnMinute_ADRESS 7
#define TimeOffHour_ADRESS 8
#define TimeOffMinute_ADRESS 9
#define summerTime_ADRESS 10

int mode = 0;
int oldminutes = 100;
int timetostay = 10; // in Sekunden

int redValueTime;
int greenValueTime;
int blueValueTime;
int redValueSpecial;
int greenValueSpecial;
int blueValueSpecial;
int TimeOnHour = 6;
int TimeOnMinute = 0;
int TimeOffHour = 22;
int TimeOffMinute = 0;
int summerTime = 0;
int red, green, blue;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t textColor;
uint32_t specialColor;

int hoursValue = 0;

ESP8266WebServer server(80);

// Funktion zur Berechnung der Sommerzeit in der Schweiz
bool isSummerTime(int year, int month, int day, int hour) {
  struct tm timeinfo;
  timeinfo.tm_year = year - 1900; // Jahr seit 1900
  timeinfo.tm_mon = month - 1;     // Monat 0-11
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = 0;
  timeinfo.tm_sec = 0;
  
  time_t t = mktime(&timeinfo);
  struct tm *marchLastSunday = localtime(&t);
  marchLastSunday->tm_mon = 2; // März
  marchLastSunday->tm_mday = 31;
  mktime(marchLastSunday);
  while (marchLastSunday->tm_wday != 0) { // 0 = Sonntag
    marchLastSunday->tm_mday--;
    mktime(marchLastSunday);
  }
  time_t marchEnd = mktime(marchLastSunday) + 2 * 3600; // 02:00 Uhr
  
  struct tm *octLastSunday = localtime(&t);
  octLastSunday->tm_mon = 9; // Oktober
  octLastSunday->tm_mday = 31;
  mktime(octLastSunday);
  while (octLastSunday->tm_wday != 0) { // 0 = Sonntag
    octLastSunday->tm_mday--;
    mktime(octLastSunday);
  }
  time_t octEnd = mktime(octLastSunday) + 3 * 3600; // 03:00 Uhr
  
  time_t now = mktime(&timeinfo);
  
  return (now >= marchEnd && now < octEnd);
}

void setup() {
  WiFi.hostname("Wordclock");
  WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.begin(ssid, password);

  Serial.begin(115200);
  EEPROM.begin(512);
  loadColorsFromEEPROM();

  strip.begin();
  strip.show();
  updateColors(); // Farben initialisieren

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    rainbow(2);
    delay(500);
  }
  setupWebServer();

  Serial.print("Server IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  delay(800);

  timeClient.begin();
  timeClient.setTimeOffset(3600); // Initiales Offset, wird im loop aktualisiert

  oldminutes = 100;
  displayTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getEpochTime());
}

void loop() {
  server.handleClient();
  timeClient.update();
  
  time_t rawtime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawtime);
  int year = ti->tm_year + 1900;
  int month = ti->tm_mon + 1;
  int day = ti->tm_mday;
  int hour = ti->tm_hour;

  // Sommerzeit automatisch berechnen
  summerTime = isSummerTime(year, month, day, hour) ? 1 : 0;
  timeClient.setTimeOffset(summerTime ? 7200 : 3600); // UTC+2 für Sommerzeit, UTC+1 für Winterzeit

  displayTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getEpochTime());
  delay(100);
}

void loadColorsFromEEPROM() {
  redValueTime = EEPROM.read(RED_VALUE_TIME_ADDRESS);
  greenValueTime = EEPROM.read(GREEN_VALUE_TIME_ADDRESS);
  blueValueTime = EEPROM.read(BLUE_VALUE_TIME_ADDRESS);
  redValueSpecial = EEPROM.read(RED_VALUE_SPECIAL_ADDRESS);
  greenValueSpecial = EEPROM.read(GREEN_VALUE_SPECIAL_ADDRESS);
  blueValueSpecial = EEPROM.read(BLUE_VALUE_SPECIAL_ADDRESS);
  TimeOnHour = EEPROM.read(TimeOnHour_ADRESS);
  TimeOnMinute = EEPROM.read(TimeOnMinute_ADRESS);
  TimeOffHour = EEPROM.read(TimeOffHour_ADRESS);
  TimeOffMinute = EEPROM.read(TimeOffMinute_ADRESS);
  summerTime = EEPROM.read(summerTime_ADRESS);
  
  Serial.println("EEPROM gelesen");
}

void saveColorsToEEPROM() {
  EEPROM.write(RED_VALUE_TIME_ADDRESS, redValueTime);
  EEPROM.write(GREEN_VALUE_TIME_ADDRESS, greenValueTime);
  EEPROM.write(BLUE_VALUE_TIME_ADDRESS, blueValueTime);
  EEPROM.write(RED_VALUE_SPECIAL_ADDRESS, redValueSpecial);
  EEPROM.write(GREEN_VALUE_SPECIAL_ADDRESS, greenValueSpecial);
  EEPROM.write(BLUE_VALUE_SPECIAL_ADDRESS, blueValueSpecial);
  EEPROM.write(TimeOnHour_ADRESS, TimeOnHour);
  EEPROM.write(TimeOnMinute_ADRESS, TimeOnMinute);
  EEPROM.write(TimeOffHour_ADRESS, TimeOffHour);
  EEPROM.write(TimeOffMinute_ADRESS, TimeOffMinute);
  EEPROM.write(summerTime_ADRESS, summerTime);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert");
  delay(1000);
}

void updateColors() {
  // Farben direkt ohne Helligkeitsskalierung setzen
  textColor = strip.Color(redValueTime, greenValueTime, blueValueTime);
  specialColor = strip.Color(redValueSpecial, greenValueSpecial, blueValueSpecial);
}

void displayTime(int hours, int minutes, unsigned long epochTime) {
  time_t rawtime = epochTime;
  struct tm *ti = localtime(&rawtime);

  int year = ti->tm_year + 1900;
  int month = ti->tm_mon + 1;
  int day = ti->tm_mday;
  int hour = ti->tm_hour;
  int minute = ti->tm_min;

  Serial.print("Datum: ");
  Serial.print(day);
  Serial.print(".");
  Serial.print(month);
  Serial.print(".");
  Serial.print(year);
  Serial.print(" Zeit: ");
  Serial.print(hours);
  Serial.print(":");
  Serial.println(minutes);

  Serial.print("SpecialColor: ");
  Serial.print(redValueSpecial);
  Serial.print(".");
  Serial.print(greenValueSpecial);
  Serial.print(".");
  Serial.println(blueValueSpecial);

  Serial.print("TimeColor: ");
  Serial.print(redValueTime);
  Serial.print(".");
  Serial.print(greenValueTime);
  Serial.print(".");
  Serial.println(blueValueTime);

  Serial.print("OnTime: ");
  Serial.print(TimeOnHour);
  Serial.print(":");
  Serial.println(TimeOnMinute);

  Serial.print("OffTime: ");
  Serial.print(TimeOffHour);
  Serial.print(":");
  Serial.println(TimeOffMinute);

  Serial.print("SummerTime: ");
  Serial.println(summerTime);

  if (minutes != oldminutes) {
    if ((hours > TimeOnHour || (hours == TimeOnHour && minutes >= TimeOnMinute)) &&
        (hours < TimeOffHour || (hours == TimeOffHour && minutes <= TimeOffMinute))) {

      Serial.println("TimeToShine");

      clearWords();

      lightUpWord("ES", textColor);
      lightUpWord("ISCH", textColor);

      if ((minutes >= 0) & (minutes < 5)) {
        Serial.println("GENAUI STUND");
      }

      if ((minutes >= 5) & (minutes < 10)) {
        Serial.println("füf ab");
        lightUpWord("FÜF", textColor);
        lightUpWord("AB", textColor);
      }

      if ((minutes >= 10) & (minutes < 15)) {
        Serial.println("ZÄH AB");
        lightUpWord("ZÄH", textColor);
        lightUpWord("AB", textColor);
      }

      if ((minutes >= 15) & (minutes < 20)) {
        Serial.println("VIERTU AB");
        lightUpWord("VIERTU", textColor);
        lightUpWord("AB", textColor);
      }
      if ((minutes >= 20) & (minutes < 25)) {
        Serial.println("ZWÄNZG AB");
        lightUpWord("ZWÄNZG", textColor);
        lightUpWord("AB", textColor);
      }
      if ((minutes >= 25) & (minutes < 30)) {
        Serial.println("FÜF VOR HAUBI");
        lightUpWord("FÜF", textColor);
        lightUpWord("VOR", textColor);
        lightUpWord("HAUBI", textColor);
      }

      if ((minutes >= 30) & (minutes < 35)) {
        Serial.println("HAUBI");
        lightUpWord("HAUBI", textColor);
      }

      if ((minutes >= 35) & (minutes < 40)) {
        Serial.println("FÜF AB HAUBI");
        lightUpWord("FÜF", textColor);
        lightUpWord("AB", textColor);
        lightUpWord("HAUBI", textColor);
      }

      if ((minutes >= 40) & (minutes < 45)) {
        Serial.println("ZWÄNZG VOR");
        lightUpWord("ZWÄNZG", textColor);
        lightUpWord("VOR", textColor);
      }

      if ((minutes >= 45) & (minutes < 50)) {
        Serial.println("VIERTU VOR");
        lightUpWord("VIERTU", textColor);
        lightUpWord("VOR", textColor);
      }

      if ((minutes >= 50) & (minutes < 55)) {
        Serial.println("ZÄH");
        lightUpWord("ZÄH", textColor);
        lightUpWord("VOR", textColor);
      }

      if ((minutes >= 55) & (minutes < 60)) {
        Serial.println("FÜF VOR");
        lightUpWord("FÜF", textColor);
        lightUpWord("VOR", textColor);
      }

      if ((minutes >= 0) & (minutes < 25)) {
        hoursValue = hours;
        Serial.println(hoursValue);
      }
      if ((minutes >= 25) & (minutes < 59)) {
        hoursValue = hours + 1;
        Serial.println(hoursValue);
      }
      if (hoursValue > 12) {
        hoursValue = hoursValue - 12;
      }
      lightUpNumber(hoursValue, textColor);

      if ((day == 1) & (month == 1)) {
        Serial.println("Happy NewYear");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("NEW", specialColor);
        lightUpWord("YEAR", specialColor);
      }

      if ((day == 5) & (month == 7)) {
        Serial.println("Happy Birthday Livia");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
        lightUpWord("LIVIA", specialColor);
      }

      if ((day == 26) & (month == 11)) {
        Serial.println("Happy Birthday Kevin");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
        lightUpWord("KEVIN", specialColor);
      }

      if ((day == 26) & (month == 3)) {
        Serial.println("Happy Birthday Tiziano");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      if ((day == 14) & (month == 9)) {
        Serial.println("Happy Birthday Andrea");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      if ((day == 25) & (month == 3)) {
        Serial.println("Happy Birthday Alessia");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      if ((day == 29) & (month == 11)) {
        Serial.println("Happy Birthday Leo");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      if ((day == 9) & (month == 2)) {
        Serial.println("Happy Birthday Shady");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      if ((day == 3) & (month == 6)) {
        delay(timetostay * 1000);
        clearWords();
        Serial.println("Anniversary");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("YEARS", specialColor);
        if (year - 2018 == 5) {
          lightUpWord("FÜF", specialColor);
        }
        if (year - 2018 == 6) {
          lightUpWord("SÄCHS", specialColor);
        }
        if (year - 2018 == 7) {
          lightUpWord("SIBE", specialColor);
        }
        if (year - 2018 == 8) {
          lightUpWord("ACHT", specialColor);
        }
        if (year - 2018 == 9) {
          lightUpWord("NÜN", specialColor);
        }
        if (year - 2018 == 10) {
          lightUpWord("ZÄH", specialColor);
        }
        if (year - 2018 == 11) {
          lightUpWord("ÖUF", specialColor);
        }
        if (year - 2018 == 12) {
          lightUpWord("ZWÖUF", specialColor);
        }
        delay(timetostay * 1000);
      }

      oldminutes = minutes;
    } else {
      clearWords();
    }
  }
}

void lightUpWord(String word, uint32_t color) {
  if (word == "ES") lightUp(0, 2, color);
  else if (word == "ISCH") lightUp(3, 4, color);
  else if (word == "FÜF") lightUp(8, 3, color);
  else if (word == "VIERTU") lightUp(15, 6, color);
  else if (word == "ZÄH") lightUp(11, 3, color);
  else if (word == "ZWÄNZG") lightUp(23, 6, color);
  else if (word == "VOR") lightUp(30, 3, color);
  else if (word == "AB") lightUp(40, 2, color);
  else if (word == "HAUBI") lightUp(34, 5, color);
  else if (word == "NEW") lightUp(45, 3, color);
  else if (word == "YEAR") lightUp(49, 4, color);
  else if (word == "YEARS") lightUp(49, 5, color);
  else if (word == "LIVIA") lightUp(83, 5, color);
  else if (word == "KEVIN") lightUp(78, 5, color);
  else if (word == "HAPPY") {
    lightUp(21, 1, color);
    lightUp(22, 1, color);
    lightUp(43, 1, color);
    lightUp(44, 1, color);
    lightUp(65, 1, color);
  } else if (word == "BIRTHDAY") {
    lightUp(33, 1, color);
    lightUp(54, 1, color);
    lightUp(55, 1, color);
    lightUp(76, 1, color);
    lightUp(77, 1, color);
    lightUp(98, 1, color);
    lightUp(99, 1, color);
    lightUp(120, 1, color);
  } else if (word == "ZWÖI") lightUp(113, 4, color);
  else if (word == "DRÜ") lightUp(117, 3, color);
  else if (word == "VIER") lightUp(88, 5, color);
  else if (word == "FÜF") lightUp(57, 3, color);
  else if (word == "SÄCHS") lightUp(66, 5, color);
  else if (word == "SIBE") lightUp(70, 5, color);
  else if (word == "ACHT") lightUp(93, 4, color);
  else if (word == "NÜN") lightUp(61, 3, color);
  else if (word == "ZÄ") lightUp(106, 2, color);
  else if (word == "ÖUF") lightUp(100, 3, color);
  else if (word == "ZWÖUF") lightUp(100, 5, color);
}

void lightUpNumber(int number, uint32_t color) {
  if (number == 1) lightUp(110, 3, color);
  else if (number == 2) lightUp(113, 4, color);
  else if (number == 3) lightUp(117, 3, color);
  else if (number == 4) lightUp(88, 5, color);
  else if (number == 5) lightUp(57, 4, color);
  else if (number == 6) lightUp(66, 6, color);
  else if (number == 7) lightUp(70, 5, color);
  else if (number == 8) lightUp(93, 5, color);
  else if (number == 9) lightUp(61, 4, color);
  else if (number == 10) lightUp(106, 4, color);
  else if (number == 11) lightUp(100, 4, color);
  else if (number == 12) lightUp(100, 6, color);
}

void lightUp(int startIndex, int count, uint32_t color) {
  for (int i = startIndex; i < startIndex + count; i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void clearWords() {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
}

void handleRoot() {
  mode = 0;
  String summerTimeChecked = (summerTime == 1) ? "checked" : "";
  // Formatierung der Zeitwerte mit führenden Nullen
  String startTimeValue = (TimeOnHour < 10 ? "0" : "") + String(TimeOnHour) + ":" + (TimeOnMinute < 10 ? "0" : "") + String(TimeOnMinute);
  String endTimeValue = (TimeOffHour < 10 ? "0" : "") + String(TimeOffHour) + ":" + (TimeOffMinute < 10 ? "0" : "") + String(TimeOffMinute);
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WordClock</title>
  <style>
    body {
      font-family: 'Arial', sans-serif;
      background: linear-gradient(135deg, #667eea, #764ba2);
      margin: 0;
      padding: 20px;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      color: #fff;
    }
    .container {
      background: rgba(255, 255, 255, 0.95);
      padding: 30px;
      border-radius: 15px;
      box-shadow: 0 8px 16px rgba(0, 0, 0, 0.2);
      width: 100%;
      max-width: 450px;
      color: #333;
    }
    h2 {
      text-align: center;
      color: #764ba2;
      margin-bottom: 25px;
      font-size: 28px;
      text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.1);
    }
    .section {
      margin-bottom: 25px;
      padding: 15px;
      background: #f9f9f9;
      border-radius: 10px;
      box-shadow: inset 0 1px 3px rgba(0, 0, 0, 0.05);
    }
    label {
      display: block;
      margin: 10px 0 5px;
      color: #555;
      font-weight: 600;
      font-size: 14px;
    }
    input[type="color"] {
      width: 100%;
      height: 40px;
      padding: 0;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      margin-bottom: 10px;
    }
    input[type="time"] {
      width: 100%;
      padding: 8px;
      margin-bottom: 10px;
      border: 1px solid #ddd;
      border-radius: 5px;
      box-sizing: border-box;
      font-size: 14px;
    }
    input[type="checkbox"] {
      margin-left: 10px;
      vertical-align: middle;
    }
    button {
      width: 100%;
      padding: 12px;
      margin: 5px 0;
      background: linear-gradient(to right, #667eea, #764ba2);
      color: white;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 16px;
      font-weight: bold;
      transition: transform 0.2s, box-shadow 0.2s;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
    }
    .color-picker {
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    .info {
      font-size: 12px;
      color: #777;
      margin-top: 5px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>WordClock Steuerung</h2>
    <div class="section">
      <label>Zeitfarbe:</label>
      <div class="color-picker">
        <input type="color" id="timeColor" value="#ffffff">
        <button onclick="setTimeColor()">Set Time Color</button>
      </div>
    </div>
    <div class="section">
      <label>Sonderfarbe:</label>
      <div class="color-picker">
        <input type="color" id="specialColor" value="#ffffff">
        <button onclick="setSpecialColor()">Set Special Color</button>
      </div>
    </div>
    <div class="section">
      <label>Einschaltzeit:</label>
      <input type="time" id="startTime" value=")rawliteral" + startTimeValue + R"rawliteral(">
      <label>Ausschaltzeit:</label>
      <input type="time" id="endTime" value=")rawliteral" + endTimeValue + R"rawliteral(">
      <label>Sommerzeit: <input type="checkbox" id="summerTime" disabled )rawliteral" + summerTimeChecked + R"rawliteral(></label>
      <div class="info">Sommerzeit wird automatisch basierend auf dem Datum berechnet.</div>
      <button onclick="setTime()">Set Time</button>
    </div>
  </div>
  <script>
    function hexToRgb(hex) {
      const r = parseInt(hex.slice(1, 3), 16);
      const g = parseInt(hex.slice(3, 5), 16);
      const b = parseInt(hex.slice(5, 7), 16);
      return { r, g, b };
    }

    function setTimeColor() {
      const { r, g, b } = hexToRgb(document.getElementById('timeColor').value);
      fetch('/timecolor', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `red=${r}&green=${g}&blue=${b}`
      }).then(response => {
        alert('Done');
        window.location.href = '/';
      });
    }

    function setSpecialColor() {
      const { r, g, b } = hexToRgb(document.getElementById('specialColor').value);
      fetch('/specialcolor', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `red=${r}&green=${g}&blue=${b}`
      }).then(response => {
        alert('Done');
        window.location.href = '/';
      });
    }

    function setTime() {
      const start = document.getElementById('startTime').value;
      const end = document.getElementById('endTime').value;
      const timeRgb = hexToRgb(document.getElementById('timeColor').value);
      const specialRgb = hexToRgb(document.getElementById('specialColor').value);
      fetch('/apply', {
        method: 'POST',
        headers: {'Content-Type': 'application/x-www-form-urlencoded'},
        body: `startTime=${start}&endTime=${end}&redTime=${timeRgb.r}&greenTime=${timeRgb.g}&blueTime=${timeRgb.b}&redSpecial=${specialRgb.r}&greenSpecial=${specialRgb.g}&blueSpecial=${specialRgb.b}`
      }).then(response => {
        alert('Done');
        window.location.href = '/';
      });
    }
  </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleTimeColor() {
  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")) {
    redValueTime = server.arg("red").toInt();
    greenValueTime = server.arg("green").toInt();
    blueValueTime = server.arg("blue").toInt();

    updateColors();
    
    Serial.printf("TimeColor values - red: %d, green: %d, blue: %d\n", 
      redValueTime, greenValueTime, blueValueTime);
    
    saveColorsToEEPROM();
    oldminutes = 100; // Erzwinge ein Update der Anzeige
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Done");
  } else {
    redValueTime = EEPROM.read(RED_VALUE_TIME_ADDRESS);
    greenValueTime = EEPROM.read(GREEN_VALUE_TIME_ADDRESS);
    blueValueTime = EEPROM.read(BLUE_VALUE_TIME_ADDRESS);

    updateColors();

    Serial.printf("TimeColor values - red: %d, green: %d, blue: %d\n", 
      redValueTime, greenValueTime, blueValueTime);
    
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Done");
  }
}

void handleSpecialColor() {
  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")) {
    redValueSpecial = server.arg("red").toInt();
    greenValueSpecial = server.arg("green").toInt();
    blueValueSpecial = server.arg("blue").toInt();

    updateColors();
    
    Serial.printf("SpecialColor values - red: %d, green: %d, blue: %d\n", 
      redValueSpecial, greenValueSpecial, blueValueSpecial);
    
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Done");
  } else {
    redValueSpecial = EEPROM.read(RED_VALUE_SPECIAL_ADDRESS);
    greenValueSpecial = EEPROM.read(GREEN_VALUE_SPECIAL_ADDRESS);
    blueValueSpecial = EEPROM.read(BLUE_VALUE_SPECIAL_ADDRESS);

    updateColors();

    Serial.printf("SpecialColor values - red: %d, green: %d, blue: %d\n", 
      redValueSpecial, greenValueSpecial, blueValueSpecial);
    
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Done");
  }
}

void handleApply() {
  if (server.hasArg("startTime") && server.hasArg("endTime")) {
    Serial.println("apply");

    String startTimeStr = server.arg("startTime");
    String endTimeStr = server.arg("endTime");
    
    int startTimeHour = startTimeStr.substring(0, 2).toInt();
    int startTimeMinute = startTimeStr.substring(3).toInt();
    int endTimeHour = endTimeStr.substring(0, 2).toInt();
    int endTimeMinute = endTimeStr.substring(3).toInt();

    TimeOnHour = startTimeHour;
    TimeOnMinute = startTimeMinute;
    TimeOffHour = endTimeHour;
    TimeOffMinute = endTimeMinute;

    if (server.hasArg("redTime") && server.hasArg("greenTime") && server.hasArg("blueTime")) {
      redValueTime = server.arg("redTime").toInt();
      greenValueTime = server.arg("greenTime").toInt();
      blueValueTime = server.arg("blueTime").toInt();
    }

    if (server.hasArg("redSpecial") && server.hasArg("greenSpecial") && server.hasArg("blueSpecial")) {
      redValueSpecial = server.arg("redSpecial").toInt();
      greenValueSpecial = server.arg("greenSpecial").toInt();
      blueValueSpecial = server.arg("blueSpecial").toInt();
    }

    updateColors();

    Serial.printf("Time values - TimeOnHour: %d, TimeOnMinute: %d, TimeOffHour: %d, TimeOffMinute: %d\n", 
      TimeOnHour, TimeOnMinute, TimeOffHour, TimeOffMinute);
    
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Done");
  } else {
    server.send(400, "text/plain", "Missing values");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/apply", HTTP_POST, handleApply);
  server.on("/timecolor", HTTP_POST, handleTimeColor);
  server.on("/specialcolor", HTTP_POST, handleSpecialColor);

  server.begin();
  Serial.println("HTTP server started");
}

void rainbow(int wait) {
  for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
    strip.rainbow(firstPixelHue);
    strip.show();
    delay(wait);
  }
}