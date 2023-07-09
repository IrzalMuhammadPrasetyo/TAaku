#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "Rahasia"
#define WIFI_PASSWORD "lupaguaeuy"
#define API_KEY "AIzaSyBArcrjCg9xNdnl2BMmBROygknJmBts0RA"
#define USER_EMAIL "bismillah@gmail.com"
#define USER_PASSWORD "suksesta"
#define DATABASE_URL "https://getdata-de5f8-default-rtdb.asia-southeast1.firebasedatabase.app/"

//oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//sensor moist
const int AirValue = 2788;    
const int WaterValue = 927;  
const int SensorPin = 33;
int soilMoistureValue = 0;
int soilmoisturepercent = 0;

//sensor pH
#define analogInPin 32
int sensorValue = 0;
float outputValue = 0.0;
int anpH1, anpH2;

//sensor npk
float n, p, k, coba, vaql1, vaql2, vaql3, Nx, Px, Kx;

//gps
static const int RXPin = 17, TXPin = 16;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
String maps;
SoftwareSerial gpsneo(RXPin, TXPin);

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate, tanggal, waktu, key, timestamp;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;

String countPath = "/Count";
String linkPath = "/Link";
String nPath = "/N";
String pPath = "/P";
String kPath = "/K";
String phPath = "/pH";
String moiPath = "/Moisture";
String datePath = "/Date";
String timePath = "/Time";
String mapsPath = "/Link";
String timestampPath = "/Timestamp";

String parentPath;
FirebaseJson json;

String counter = "0";

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 300000;

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);

  display.setCursor(0, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Connecting");
  display.setCursor(80, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("WiFi");
  display.display();
  display.clearDisplay();
  }
  Serial.println();
  Serial.print("Terkoneksi IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  display.setCursor(0, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("WiFi");
  display.setCursor(20, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Connected");
  display.display();
  delay(3000);
  display.clearDisplay();
}

void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  display.setCursor(5, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Monitoring");
  display.setCursor(40, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Cabai");
  display.display();
  display.clearDisplay();

  initWiFi();

  timeClient.begin();
  timeClient.setTimeOffset(25200);

  gpsneo.begin(GPSBaud);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;

  Firebase.begin(&config, &auth);
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
    
  display.setCursor(0, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Getting");
  display.setCursor(30, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("User UID");
  display.display();
  display.clearDisplay();
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  display.setCursor(0, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("User UID");
  display.setCursor(20, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Connected");
  display.display();
  display.clearDisplay();

  databasePath = uid ;
}

void npk() {
  n = analogRead(34);
  vaql1 = map(n, 0, 4095, 0, 1023);
  Nx = map(vaql1, 0, 1000, 0, 200);
  if (Nx < 0) {
    Nx = 0;
  } else if (Nx > 200)
    Nx = 200;
  else {
    Nx = Nx;
  }
  Serial.print("N=" + String(Nx));

  p = analogRead(34);
  vaql2 = map(p, 0, 4095, 0, 1023);
  Px = map(vaql2, 0, 590, 0, 200);

  if (Px < 0) {
    Px = 0;
  } else if (Px > 200)
    Px = 200;
  else {
    Px = Px;
  }
  Serial.print("; P=" + String(Px));

  k = analogRead(34);
  vaql3 = map(k, 0, 4095, 0, 1023);
  Kx = map(vaql3, 0, 900, 100, 300);

  if (Kx < 100) {
    Kx = 100;
  } else if (Kx > 300)
    Kx = 300;
  else {
    Kx = Kx;
  }
  Serial.println("; K=" + String(Kx));
}

void pH() {
  sensorValue = analogRead(analogInPin);
  anpH1 = map(sensorValue, 0, 4095, 0, 1023);
  anpH2 = map(anpH1, 0, 1023, 4, 45);
  outputValue = (-0.0693 * anpH2) + 7.3855;
  Serial.println("pH=" + String(outputValue));
}

void moist() {
  soilMoistureValue = analogRead(SensorPin);
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if (soilmoisturepercent > 100) {
    soilmoisturepercent = 100;
  } else if (soilmoisturepercent < 0) {
    soilmoisturepercent = 0;
  } else {
    soilmoisturepercent = soilmoisturepercent;
  }

  Serial.println("Moist=" + String(soilmoisturepercent));
}

void loc(){
  Serial.print("Maps= ");
  if (gps.location.isValid()){
    maps="https://www.google.com/maps/place/" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    Serial.println(maps);
  }
  else{
    Serial.println("INVALID");
  }
}

void ntp(){

  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  formattedDate = timeClient.getFormattedDate();

  int splitT = formattedDate.indexOf("T");
  tanggal = formattedDate.substring(0, splitT);
  Serial.print("Date: ");
  Serial.print(tanggal);
 
  waktu = formattedDate.substring(splitT+1, formattedDate.length()-1);
  Serial.print("; Time: ");
  Serial.print(waktu);

  timestamp = String(tanggal)+"T"+String(waktu);
  Serial.print("; Timestamp: ");
  Serial.println(timestamp);
}

void loop() {

  boolean newData = false;
  for (unsigned long start = millis(); millis() - start < 1000;){
    while (gpsneo.available()){
      if (gps.encode(gpsneo.read()) ){ 
        newData = true;
      }
    }
  }

  if(newData == true){
    newData = false;
    loc();
    npk();
    pH();
    moist();
    ntp();
    Serial.println("======================================================================");
    delay(1000);

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

  counter = String(counter.toInt() + 1);
  
  Serial.print("Counter: ");
  Serial.println(counter);

    parentPath = databasePath + "/" + String(tanggal) + "_" +String(waktu);

    String pH = String(outputValue, 1);
    String moi = String(soilmoisturepercent);
    String N = String(Nx, 0);
    String P = String(Px, 0);
    String K = String(Kx, 0);

    json.set(linkPath.c_str(), String(maps));
    json.set(nPath.c_str(), String(N));
    json.set(pPath.c_str(), String(P));
    json.set(kPath.c_str(), String(K));
    json.set(phPath.c_str(), String(pH));
    json.set(moiPath.c_str(), String(moi));
    json.set(datePath.c_str(), String(tanggal));
    json.set(timePath.c_str(), String(waktu));
    json.set(countPath.c_str(), String(counter));
    json.set(timestampPath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "OK" : fbdo.errorReason().c_str());
    Serial.println("======================================================================");
  }

  display.clearDisplay();
  display.setCursor(17, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("N");
  display.setCursor(5, 18);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(Nx, 0);
  display.display();

  display.setCursor(100, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("P");
  display.setCursor(88, 18);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(Px, 0);
  display.display();

  display.setCursor(63, 29);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("K");
  display.setCursor(50, 48);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(Kx, 0);
  display.display();
  delay(3000);
  display.clearDisplay();

  display.setCursor(0, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("Moist");
  display.setCursor(11, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(soilmoisturepercent);
  display.display();

  display.setCursor(95, 15);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println("pH");
  display.setCursor(88, 35);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(outputValue, 1);
  display.display();
  delay(3000);
  display.clearDisplay();
  }
  else{
    Serial.println("Menghubungkan satelit");
  }
}