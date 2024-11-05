#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <TimeLib.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>



//WiFi credentials
const char* ssid = "WLAN";
const char* password = "Passwort";

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 200);

// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ch.pool.ntp.org", 3600);



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


int mode = 0;
int up = 0;
int down = 0;
int left = 0;
int right = 0;
int oldminutes = 100;
int timetostay = 10; // in sekunden

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
int red, green, blue;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t textColor = strip.Color(redValueTime, greenValueTime, blueValueTime);
uint32_t specialColor = strip.Color(redValueSpecial, greenValueSpecial, blueValueSpecial);

int hoursValue = 0;

ESP8266WebServer server(80);

void setup() {
  WiFi.hostname("Wordclock");
  WiFi.config(local_IP, gateway, subnet, dns);
  WiFi.begin(ssid, password);

  Serial.begin(115200);
  EEPROM.begin(512);
  loadColorsFromEEPROM();

  strip.begin();
  strip.show();
  textColor = strip.Color(redValueTime, greenValueTime, blueValueTime);
  specialColor = strip.Color(redValueSpecial, greenValueSpecial, blueValueSpecial);



  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    rainbow(2);
    delay(500);
  }
  setupWebServer();
  loadColorsFromEEPROM();

  Serial.print("Server IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  delay(800);

  timeClient.begin();
  timeClient.setTimeOffset(3600);

  oldminutes = 100;
  displayTime(timeClient.getHours(), timeClient.getMinutes(), timeClient.getEpochTime());
}

void loop() {
  server.handleClient();
  timeClient.update();
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

  EEPROM.commit();
  Serial.println("EEPROM gespeichert");
  delay(1000);
}

void displayTime(int hours, int minutes, unsigned long epochTime) {
  struct tm* ti;
  time_t rawtime = epochTime;
  ti = localtime(&rawtime);
  int year = ti->tm_year + 1900;
  int month = ti->tm_mon + 1;
  int day = ti->tm_mday;

  int aniYears = year - 2019;
  hours = hours;

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
        Serial.println("Happy Birtday Livia");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
        lightUpWord("LIVIA", specialColor);
      }

      if ((day == 26) & (month == 11)) {
        Serial.println("Happy Birtday Kevin");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
        lightUpWord("Kevin", specialColor);
      }

        if ((day == 26) & (month == 3)) {
        Serial.println("Happy Birtday Tiziano");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

         if ((day == 14) & (month == 9)) {
        Serial.println("Happy Birtday Andrea");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

               if ((day == 25) & (month == 3)) {
        Serial.println("Happy Birtday Alessia");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

      
        if ((day == 29) & (month == 11)) {
        Serial.println("Happy Birtday Leo");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }

        if ((day == 9) & (month == 2)) {
        Serial.println("Happy Birtday Shady");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("BIRTHDAY", specialColor);
      }



      if ((day == 3) & (month == 6)) {
        delay(timetostay * 1000);
        clearWords();
        Serial.println("Anniversary");
        lightUpWord("HAPPY", specialColor);
        lightUpWord("YEARS", specialColor);
        if (aniYears == 5) {
          lightUpWord("FÜF", specialColor);
        }
        if (aniYears == 6) {
          lightUpWord("SÄCHS", specialColor);
        }
        if (aniYears == 7) {
          lightUpWord("SIBE", specialColor);
        }
        if (aniYears == 8) {
          lightUpWord("ACHT", specialColor);
        }
        if (aniYears == 9) {
          lightUpWord("NÜN", specialColor);
        }
        if (aniYears == 10) {
          lightUpWord("ZÄH", specialColor);
        }
        if (aniYears == 11) {
          lightUpWord("ÖUF", specialColor);
        }
        if (aniYears == 12) {
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
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>WordClock</title>
  <style>
    .color-circle {
      position: relative;
      width: 300px;
      height: 300px;
      border-radius: 50%;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      background: radial-gradient(circle, transparent 40%, black 150%), radial-gradient(circle closest-side, white 1%, transparent 50%), conic-gradient(purple, magenta, red, orange, yellow, lime, aqua, blue, purple);
      cursor: crosshair;
    }
    .color-indicator {
      position: absolute;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      border: 2px solid white;
      background-color: white;
      pointer-events: none;
    }
    .color-display-container {
      display: flex;
      align-items: center;
      margin-top: 20px;
    }
    .color-display {
      width: 100px;
      height: 100px;
      border: 2px solid #000;
    }
    .slider {
      margin-left: 20px;
    }
    .button-container {
      margin-top: 20px;
    }
    .time-container {
      margin-top: 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }
    .time-input {
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    input[type='time'] {
      width: 150px;
    }
  </style>
</head>
<body>
  <h1>WordClock</h1>
  <div class='color-circle' id='colorCircle'>
    <div class='color-indicator' id='colorIndicator'></div>
  </div>
  <div class='color-display-container'>
    <div class='color-display' id='colorDisplay'></div>
    <input type='range' min='0' max='100' value='50' class='slider' id='colorSlider'>
  </div>
  <div class='button-container'>
    <button id='timeColor'>TimeColor</button>
    <button id='specialColor'>SpecialColor</button>
  </div>
  <div class='time-container'>
    <div class="time-input">
      <label for='startTime'>On Time:</label>
      <input type='time' id='startTime' name='startTime'>
    </div>
    <div class="time-input">
      <label for='endTime'>Off Time:</label>
      <input type='time' id='endTime' name='endTime'>
    </div>
  </div>
  <div class='button-container'>
    <button id='applyButton'>Apply</button>
  </div>
  <form id="colorForm" style="display:none;">
    <input type="text" id="red" name="red">
    <input type="text" id="green" name="green">
    <input type="text" id="blue" name="blue">
    <input type="text" id="startTimeValue" name="startTimeValue">
    <input type="text" id="endTimeValue" name="endTimeValue">
  </form>
  <script>
    const colorCircle = document.getElementById('colorCircle');
    const colorIndicator = document.getElementById('colorIndicator');
    const colorDisplay = document.getElementById('colorDisplay');
    const colorSlider = document.getElementById('colorSlider');
    const timeColorButton = document.getElementById('timeColor');
    const specialColorButton = document.getElementById('specialColor');
    const applyButton = document.getElementById('applyButton');

    let currentColor = 'hsl(0, 100%, 50%)';
    let hue = 0, saturation = 100, lightness = 50;
    let colorMode = '';

    colorCircle.addEventListener('mousemove', function(event) {
      const rect = colorCircle.getBoundingClientRect();
      const centerX = rect.left + rect.width / 2;
      const centerY = rect.top + rect.height / 2;
      const x = event.clientX - centerX;
      const y = event.clientY - centerY;
      const angle = Math.atan2(y, x);
      hue = angle >= 0 ? angle * 180 / Math.PI : (2 * Math.PI + angle) * 180 / Math.PI;
      const distance = Math.sqrt(x * x + y * y);
      const maxRadius = rect.width / 2;
      lightness = 100 - (distance / maxRadius) * 50;
      currentColor = `hsl(${Math.round(hue)}, ${saturation}%, ${Math.round(lightness)}%)`;
      colorIndicator.style.left = `${event.clientX - rect.left}px`;
      colorIndicator.style.top = `${event.clientY - rect.top}px`;
      colorIndicator.style.backgroundColor = currentColor;
    });

    colorCircle.addEventListener('click', function() {
      colorDisplay.style.backgroundColor = currentColor;
    });

    colorSlider.addEventListener('input', function() {
      lightness = colorSlider.value;
      currentColor = `hsl(${Math.round(hue)}, ${saturation}%, ${Math.round(lightness)}%)`;
      colorDisplay.style.backgroundColor = currentColor;
      colorIndicator.style.backgroundColor = currentColor;
    });

    timeColorButton.addEventListener('click', function() {
      colorMode = 'timecolor';
      submitColor('/timecolor');
    });

    specialColorButton.addEventListener('click', function() {
      colorMode = 'specialcolor';
      submitColor('/specialcolor');
    });

    applyButton.addEventListener('click', function() {
      if (colorMode) {
        submitColor(colorMode === 'timecolor' ? '/timecolor' : '/specialcolor');
      } else {
        submitColor('/apply');
      }
    });

    function submitColor(endpoint) {
      const [h, s, l] = currentColor.match(/\d+/g).map(Number);
      const [r, g, b] = hslToRgb(h, s, l);

      document.getElementById('red').value = r;
      document.getElementById('green').value = g;
      document.getElementById('blue').value = b;
      document.getElementById('startTimeValue').value = document.getElementById('startTime').value;
      document.getElementById('endTimeValue').value = document.getElementById('endTime').value;

      const formData = new FormData(document.getElementById('colorForm'));

      fetch(endpoint, {
        method: 'POST',
        body: new URLSearchParams(formData)
      })
      .then(response => {
        if (response.ok) {
          window.location.href = '/';
        } else {
          alert('Error submitting values');
        }
      })
      .catch(error => {
        alert('Error submitting values');
      });
    }

    function hslToRgb(h, s, l) {
      h /= 360;
      s /= 100;
      l /= 100;
      let r, g, b;

      if (s === 0) {
        r = g = b = l;
      } else {
        const hue2rgb = (p, q, t) => {
          if (t < 0) t += 1;
          if (t > 1) t -= 1;
          if (t < 1 / 6) return p + (q - p) * 6 * t;
          if (t < 1 / 2) return q;
          if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
          return p;
        };
        const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        const p = 2 * l - q;
        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
      }

      return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
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

    Serial.printf("TimeColor values - red: %d, green: %d, blue: %d\n", redValueTime, greenValueTime, blueValueTime);
    textColor = strip.Color(redValueTime, greenValueTime, blueValueTime);
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Redirecting to home");
  } else {
  redValueTime = EEPROM.read(RED_VALUE_TIME_ADDRESS);
  greenValueTime = EEPROM.read(GREEN_VALUE_TIME_ADDRESS);
  blueValueTime = EEPROM.read(BLUE_VALUE_TIME_ADDRESS);

    Serial.printf("TimeColor values - red: %d, green: %d, blue: %d\n", redValueTime, greenValueTime, blueValueTime);
    textColor = strip.Color(redValueTime, greenValueTime, blueValueTime);
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Redirecting to home");

  }
}

void handleSpecialColor() {
  if (server.hasArg("red") && server.hasArg("green") && server.hasArg("blue")) {
    redValueSpecial = server.arg("red").toInt();
    greenValueSpecial = server.arg("green").toInt();
    blueValueSpecial = server.arg("blue").toInt();

    Serial.printf("SpecialColor values - red: %d, green: %d, blue: %d\n", redValueSpecial, greenValueSpecial, blueValueSpecial);
    specialColor = strip.Color(redValueSpecial, greenValueSpecial, blueValueSpecial);
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Redirecting to home");
  } else {
  redValueSpecial = EEPROM.read(RED_VALUE_SPECIAL_ADDRESS);
  greenValueSpecial = EEPROM.read(GREEN_VALUE_SPECIAL_ADDRESS);
  blueValueSpecial = EEPROM.read(BLUE_VALUE_SPECIAL_ADDRESS);

    Serial.printf("SpecialColor values - red: %d, green: %d, blue: %d\n", redValueSpecial, greenValueSpecial, blueValueSpecial);
    specialColor = strip.Color(redValueSpecial, greenValueSpecial, blueValueSpecial);
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Redirecting to home");



  }
}

void handleApply() {
  if (server.hasArg("startTimeValue") && server.hasArg("endTimeValue")) {
    Serial.println("apply");

    String startTimeStr = server.arg("startTimeValue");
    String endTimeStr = server.arg("endTimeValue");


    int startTimeHour = startTimeStr.substring(0, 2).toInt();
    int startTimeMinute = startTimeStr.substring(3).toInt();
    int endTimeHour = endTimeStr.substring(0, 2).toInt();
    int endTimeMinute = endTimeStr.substring(3).toInt();

    TimeOnHour = startTimeHour;
    TimeOnMinute = startTimeMinute;
    TimeOffHour = endTimeHour;
    TimeOffMinute = endTimeMinute;


    Serial.printf("Time values - TimeOnHour: %d, TimeOnMinute: %d, TimeOffHour: %d, TimeOffMinute: %d\n", TimeOnHour, TimeOnMinute, TimeOffHour, TimeOffMinute);
    saveColorsToEEPROM();
    oldminutes = 100;
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "Redirecting to home");
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
