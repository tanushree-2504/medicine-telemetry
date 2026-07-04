#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* WIFI_SSID     = "Tanu_shree";
const char* WIFI_PASSWORD = "11111111";

WebServer server(80);

#define BUFFER_SIZE 100
float hrBuf[BUFFER_SIZE];
float spo2Buf[BUFFER_SIZE];
float tempBuf[BUFFER_SIZE];
int   bufIdx      = 0;
int   totalStored = 0;

unsigned long pktReceived = 0;
unsigned long pktRejected = 0;
unsigned long pktLoss     = 0;
unsigned long alertCount  = 0;
unsigned long lastPktNum  = 0;

int    latestHR     = 0;
int    latestSpO2   = 0;
float  latestTemp   = 0;
int    latestSys    = 0;
int    latestDia    = 0;
int    latestRR     = 0;
String latestStatus = "WAITING";

String alertLog[10];
int    alertLogIdx = 0;

float getAvg(float* b, int n) {
  if (n == 0) return 0;
  float s = 0;
  for (int i = 0; i < n; i++) s += b[i];
  return s / n;
}

float getMax(float* b, int n) {
  if (n == 0) return 0;
  float m = b[0];
  for (int i = 1; i < n; i++) if (b[i] > m) m = b[i];
  return m;
}

float getMin(float* b, int n) {
  if (n == 0) return 0;
  float m = b[0];
  for (int i = 1; i < n; i++) if (b[i] < m) m = b[i];
  return m;
}

void parsePacket(String raw) {
  if (!raw.startsWith("<") || !raw.endsWith(">")) {
    pktRejected++;
    return;
  }

  String content = raw.substring(1, raw.length() - 1);
  String fields[9];
  int count = 0, start = 0;

  for (int i = 0; i <= content.length(); i++) {
    if (i == content.length() || content[i] == ',') {
      if (count < 9) fields[count++] = content.substring(start, i);
      start = i + 1;
    }
  }

  if (count != 9) { pktRejected++; return; }
  if (fields[0] != "PKT") { pktRejected++; return; }

  unsigned long pktNum = fields[1].toInt();
  int   hr   = fields[2].toInt();
  int   spo2 = fields[3].toInt();
  float temp = fields[4].toFloat();
  int   sys  = fields[5].toInt();
  int   dia  = fields[6].toInt();
  int   rr   = fields[7].toInt();
  String st  = fields[8];

  if (hr < 0   || hr > 300)   { pktRejected++; return; }
  if (spo2 < 0 || spo2 > 100) { pktRejected++; return; }
  if (temp < 30 || temp > 45) { pktRejected++; return; }

  if (lastPktNum > 0 && pktNum != lastPktNum + 1)
    pktLoss += (pktNum - lastPktNum - 1);
  lastPktNum = pktNum;

  latestHR     = hr;
  latestSpO2   = spo2;
  latestTemp   = temp;
  latestSys    = sys;
  latestDia    = dia;
  latestRR     = rr;
  latestStatus = st;

  hrBuf[bufIdx]   = hr;
  spo2Buf[bufIdx] = spo2;
  tempBuf[bufIdx] = temp;
  bufIdx = (bufIdx + 1) % BUFFER_SIZE;
  if (totalStored < BUFFER_SIZE) totalStored++;

  pktReceived++;

  String alarm = "";
  if      (hr > 100)  alarm = "TACHYCARDIA HR="  + String(hr);
  else if (hr < 60)   alarm = "BRADYCARDIA HR="  + String(hr);
  if      (temp > 38) alarm = "FEVER Temp="      + String(temp, 1);
  if      (spo2 < 92) alarm = "HYPOXIA SpO2="    + String(spo2);

  if (alarm != "") {
    alertCount++;
    alertLog[alertLogIdx % 10] = alarm;
    alertLogIdx++;
    Serial.print("ALERT: ");
    Serial.println(alarm);
  }

  Serial.print("PKT#"); Serial.print(pktNum);
  Serial.print(" HR="); Serial.print(hr);
  Serial.print(" SpO2="); Serial.print(spo2);
  Serial.print(" Temp="); Serial.println(temp);
}

void handleData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  StaticJsonDocument<1024> doc;
  doc["hr"]               = latestHR;
  doc["spo2"]             = latestSpO2;
  doc["temp"]             = latestTemp;
  doc["systolic"]         = latestSys;
  doc["diastolic"]        = latestDia;
  doc["rr"]               = latestRR;
  doc["status"]           = latestStatus;
  doc["packets_received"] = pktReceived;
  doc["packets_rejected"] = pktRejected;
  doc["packet_loss"]      = pktLoss;
  doc["alert_count"]      = alertCount;
  doc["avg_hr"]           = getAvg(hrBuf, totalStored);
  doc["max_hr"]           = getMax(hrBuf, totalStored);
  doc["min_hr"]           = getMin(hrBuf, totalStored);
  doc["avg_spo2"]         = getAvg(spo2Buf, totalStored);
  doc["avg_temp"]         = getAvg(tempBuf, totalStored);

  JsonArray alerts = doc.createNestedArray("alerts");
  int n = min(alertLogIdx, 10);
  for (int i = 0; i < n; i++) alerts.add(alertLog[i]);

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "ESP32 Medical Gateway Running.");
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  for (int i = 0; i < BUFFER_SIZE; i++)
    hrBuf[i] = spo2Buf[i] = tempBuf[i] = 0;

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print(">>> IP ADDRESS: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED. Check name/password.");
  }

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Server started.");
}

void loop() {
  server.handleClient();
  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) parsePacket(line);
  }
}