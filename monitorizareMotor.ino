
const int SCT_PIN = 34;
const int ADC_MAX = 4095;
const float ADC_VOLTAGE = 3.3;
const float CURRENT_THRESHOLD = 0.025;

const int SIM_RST_PIN = 18; 
const int GSM_RX_PIN = 16;
const int GSM_TX_PIN = 17;

HardwareSerial GSM(2);

const char PHONE_NUMBER[] = "+40744575514";

// Setat la 35 de minute
const unsigned long MOTOR_OFF_TIME = 35UL * 60UL * 1000UL; 
const unsigned long DAILY_SMS_INTERVAL = 24UL * 60UL * 60UL * 1000UL;
const unsigned long NETWORK_CHECK_INTERVAL = 30UL * 60UL * 1000UL; 
const unsigned long CALL_DURATION = 30000UL; // 30 secunde apel

unsigned long lastDailySMS = 0;
unsigned long lastNetworkCheck = 0;
unsigned long motorStoppedSince = 0;

bool motorOffTimerActive = false;
bool alarmSent = false;

// Prototipuri functii
bool sendATCommand(const char* command, const char* expectedResponse, unsigned long timeout);
bool sendSMS(const char* phoneNumber, const char* message);
bool makeCall(const char* phoneNumber);
bool checkNetwork();
void resetSIM800L();
float readSCTRMS();

void setup() {
  Serial.begin(115200);
  delay(1000);

  analogReadResolution(12);
  analogSetPinAttenuation(SCT_PIN, ADC_11db);

  pinMode(SIM_RST_PIN, OUTPUT);
  digitalWrite(SIM_RST_PIN, HIGH); 

  GSM.begin(9600, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
  delay(2000);

  if (sendATCommand("AT", "OK", 2000)) {
    Serial.println("SIM800L: OK");
  } else {
    Serial.println("EROARE: SIM800L nu raspunde!");
  }

  if (sendATCommand("AT+CMGF=1", "OK", 2000)) {
    Serial.println("Mod SMS text: OK");
  } else {
    Serial.println("EROARE la configurarea SMS.");
  }

  Serial.println("Verificare retea GSM...");
  GSM.println("AT+CREG?");
  delay(1000);
  while (GSM.available()) {
    Serial.write(GSM.read());
  }

  if (sendSMS(PHONE_NUMBER, "Sistem functional. Monitorizarea aeratorului este activa.")) {
    Serial.println("SMS initial trimis cu succes!");
  } else {
    Serial.println("EROARE: SMS-ul initial nu a fost trimis.");
  }

  lastDailySMS = millis();
  lastNetworkCheck = millis();
}

void loop() {
  // 1. Verificare SMS zilnic (24h)
  if (millis() - lastDailySMS >= DAILY_SMS_INTERVAL) {
    Serial.println("Au trecut 24h. Trimitere SMS zilnic...");
    sendSMS(PHONE_NUMBER, "Sistem functional. Monitorizarea aeratorului este activa.");
    lastDailySMS = millis();
  }

  // 2. Verificare periodica a retelei GSM (30 min)
  if (millis() - lastNetworkCheck >= NETWORK_CHECK_INTERVAL) {
    Serial.println("Verificare periodica a retelei GSM...");
    if (checkNetwork()) {
      Serial.println("Status retea: OK.");
    } else {
      Serial.println("EROARE: Conexiune GSM pierduta! Reinitiez SIM800L...");
      resetSIM800L();
    }
    lastNetworkCheck = millis();
  }

  // 3. Verificare curent motor
  float rmsVoltage = readSCTRMS();
  bool motorRunning = (rmsVoltage > CURRENT_THRESHOLD);

Serial.print(rmsVoltage, 4);
  if (motorRunning) {
    if (motorOffTimerActive) {
      Serial.println("Motorul a pornit din nou. Timer resetat.");
      motorOffTimerActive = false;
      motorStoppedSince = 0;
    }
    if (alarmSent) {
      Serial.println("Alarma resetata.");
      alarmSent = false;
    }
  } else {
    if (!motorOffTimerActive) {
      motorOffTimerActive = true;
      motorStoppedSince = millis();
      Serial.println("\n!!! MOTOR OPRIT !!!");
      Serial.println("Timer de 35 minute pornit.\n");
    }

    unsigned long elapsedTime = millis() - motorStoppedSince;

    // Declansare alarma dupa 35 minute
    if (elapsedTime >= MOTOR_OFF_TIME && !alarmSent) {
      bool callOK = makeCall(PHONE_NUMBER);
      if (callOK) {
        Serial.println("Apelul de alarma a fost efectuat!");
        alarmSent = true;
      } else {
        Serial.println("EROARE: Apelul nu a putut fi initiat.");
      }
    }
  }

  delay(1000);
}

float readSCTRMS() {
  const int samples = 1000;
  double sum = 0;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(SCT_PIN);
    delayMicroseconds(100);
  }
  double offset = sum / samples;

  double sumSquares = 0;
  for (int i = 0; i < samples; i++) {
    double acValue = analogRead(SCT_PIN) - offset;
    sumSquares += acValue * acValue;
    delayMicroseconds(100);
  }

  double rmsADC = sqrt(sumSquares / samples);
  return (rmsADC * ADC_VOLTAGE) / ADC_MAX;
}

bool sendATCommand(const char* command, const char* expectedResponse, unsigned long timeout) {
  while (GSM.available()) GSM.read();

  GSM.println(command);
  unsigned long startTime = millis();
  String response = "";

  while (millis() - startTime < timeout) {
    while (GSM.available()) {
      char c = GSM.read();
      response += c;
      Serial.write(c);
      if (response.indexOf(expectedResponse) != -1) {
        return true;
      }
    }
  }
  return false;
}

bool sendSMS(const char* phoneNumber, const char* message) {
  while (GSM.available()) GSM.read();

  GSM.print("AT+CMGS=\"");
  GSM.print(phoneNumber);
  GSM.println("\"");

  unsigned long startTime = millis();
  bool promptReceived = false;

  while (millis() - startTime < 5000) {
    if (GSM.available()) {
      char c = GSM.read();
      Serial.write(c);
      if (c == '>') {
        promptReceived = true;
        break;
      }
    }
  }

  if (!promptReceived) {
    Serial.println("\nSIM800L nu a dat promptul >");
    return false;
  }

  GSM.print(message);
  GSM.write(26); // CTRL+Z

  startTime = millis();
  String response = "";

  while (millis() - startTime < 15000) {
    while (GSM.available()) {
      char c = GSM.read();
      response += c;
      Serial.write(c);
      if (response.indexOf("OK") != -1) return true;
      if (response.indexOf("ERROR") != -1) return false;
    }
  }
  return false;
}

bool makeCall(const char* phoneNumber) {
  while (GSM.available()) GSM.read();

  Serial.println("Initiere apel...");
  GSM.print("ATD");
  GSM.print(phoneNumber);
  GSM.println(";");

  unsigned long startTime = millis();
  String response = "";

  while (millis() - startTime < 10000) {
    while (GSM.available()) {
      char c = GSM.read();
      response += c;
      Serial.write(c);

      if (response.indexOf("OK") != -1) {
        Serial.println("\nApel initiat. SIM800L va suna...");
        delay(CALL_DURATION);

        Serial.println("Inchid apelul...");
        GSM.println("ATH");
        delay(1000);
        while (GSM.available()) Serial.write(GSM.read());
        return true;
      }

      if (response.indexOf("NO CARRIER") != -1 || response.indexOf("BUSY") != -1 || 
          response.indexOf("NO DIALTONE") != -1 || response.indexOf("ERROR") != -1) {
        Serial.println("\nApelul a esuat.");
        return false;
      }
    }
  }
  Serial.println("\nTimeout la initierea apelului.");
  return false;
}

bool checkNetwork() {
  while (GSM.available()) GSM.read();

  GSM.println("AT+CREG?");
  unsigned long startTime = millis();
  String response = "";

  while (millis() - startTime < 3000) {
    while (GSM.available()) {
      char c = GSM.read();
      response += c;
    }
  }

  if (response.indexOf("0,1") != -1 || response.indexOf("0,5") != -1) {
    return true;
  }
  return false;
}

void resetSIM800L() {
  Serial.println("!!! INITIEZ RESETARE HARDWARE SIM800L !!!");
  digitalWrite(SIM_RST_PIN, LOW);
  delay(1000);
  digitalWrite(SIM_RST_PIN, HIGH);

  Serial.println("Asteptam 20 secunde pentru repornirea modulului...");
  delay(20000);

  sendATCommand("AT", "OK", 2000);
  if (sendATCommand("AT+CMGF=1", "OK", 2000)) {
    Serial.println("Modul text SMS re-activat dupa reset.");
  } else {
    Serial.println("Eroare la re-activare SMS text.");
  }
}