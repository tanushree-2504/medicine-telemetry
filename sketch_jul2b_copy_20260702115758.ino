// -------------------------------------------------------
// ARDUINO UNO — Medical Device Simulator
// This code makes Arduino pretend to be a patient monitor.
// It generates fake heartrate, oxygen, temperature etc.
// and sends them out every 2 seconds as a text packet.
// -------------------------------------------------------

unsigned long packetNumber = 0;  // counts how many packets sent

void setup() {
  Serial.begin(9600);            // start talking over USB at speed 9600
  randomSeed(analogRead(A0));    // make random numbers truly random
  delay(1000);                   // wait 1 second before starting
  Serial.println("SIMULATOR_READY");  // send one startup message
}

void loop() {

  // --- Generate normal healthy values ---
  int hr        = random(60, 101);              // heart rate 60 to 100
  int spo2      = random(95, 101);              // oxygen 95 to 100
  float temp    = 36.5 + random(0, 11) / 10.0; // temperature 36.5 to 37.5
  int systolic  = random(110, 131);             // upper blood pressure
  int diastolic = random(70, 91);               // lower blood pressure
  int rr        = random(12, 21);               // breathing rate
  String status = "NORM";                       // start as normal

  // --- Randomly make one value abnormal ---
  // Roll a number from 0 to 99
  // Different ranges trigger different medical conditions
  int roll = random(0, 100);

  if (roll < 5) {
    // 5% chance: low oxygen (hypoxia)
    spo2   = random(85, 93);
    status = "ALRT";
  }
  else if (roll < 15) {
    // 10% chance: fast heart rate (tachycardia)
    hr     = random(101, 141);
    status = "ALRT";
  }
  else if (roll < 20) {
    // 5% chance: slow heart rate (bradycardia)
    hr     = random(30, 59);
    status = "ALRT";
  }
  else if (roll < 40) {
    // 20% chance: fever
    temp   = 38.0 + random(0, 20) / 10.0;
    status = "ALRT";
  }

  // --- Increase packet number by 1 ---
  packetNumber++;

  // --- Send everything as one line of text ---
  // Format: <PKT, number, HR, SpO2, Temp, SYS, DIA, RR, STATUS>
  Serial.print("<PKT,");
  Serial.print(packetNumber);
  Serial.print(",");
  Serial.print(hr);
  Serial.print(",");
  Serial.print(spo2);
  Serial.print(",");
  Serial.print(temp, 1);
  Serial.print(",");
  Serial.print(systolic);
  Serial.print(",");
  Serial.print(diastolic);
  Serial.print(",");
  Serial.print(rr);
  Serial.print(",");
  Serial.print(status);
  Serial.println(">");   // this ends the line

  // --- Wait 2 seconds before sending next packet ---
  delay(2000);
}