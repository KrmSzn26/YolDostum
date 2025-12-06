#include <SoftwareSerial.h>

SoftwareSerial sim808(D1, D2); // RX, TX

#define BUFFER_SIZE 200
char lineBuffer[BUFFER_SIZE];
int lineIndex = 0;

double latitude_inf = 0.0;
double longitude_inf = 0.0;

unsigned long lastGpsCheck = 0;
unsigned long lastHttpSend = 0;

const char* serverURL = "http://kerm.pythonanywhere.com/konum";

String sim808Oku(long timeout) {
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (sim808.available()) {
      char c = sim808.read();
      response += c;
      Serial.write(c);
    }
  }
  return response;
}

void setup() {
  Serial.begin(115200);
  sim808.begin(115200);

  delay(2000);

  sim808.println("AT");
  sim808Oku(1000);

  sim808.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");
  sim808Oku(1000);
  sim808.println("AT+SAPBR=3,1,\"APN\",\"internet\""); // Kendi APN
  sim808Oku(1000);

  sim808.println("AT+SAPBR=1,1");
  String gprsResp = sim808Oku(7000);
  if (gprsResp.indexOf("OK") != -1 || gprsResp.indexOf("ALREADY") != -1) {
    Serial.println("GPRS bağlantısı başarılı.");
  } else {
    Serial.println("GPRS bağlantısı başarısız. APN ve SIM kart kontrolü yapın.");
  }

  sim808.println("AT+SAPBR=2,1");
  sim808Oku(2000);

  sim808.println("AT+CGNSPWR=1");
  sim808Oku(2000);
  sim808.println("AT+CGPSANT=1");
  sim808Oku(2000);

  Serial.println("GPS başlatıldı, sinyal bekleniyor...");

  memset(lineBuffer, 0, BUFFER_SIZE);
}

void loop() {
  while (sim808.available()) {
    char c = sim808.read();

    if (lineIndex < BUFFER_SIZE - 1) {
      lineBuffer[lineIndex++] = c;
      lineBuffer[lineIndex] = '\0';
    }

    if (c == '\n') {
      if (lineIndex > 1) {
        satirIsle(lineBuffer);
      }
      lineIndex = 0;
      memset(lineBuffer, 0, BUFFER_SIZE);
    }
  }

  unsigned long simdi = millis();

  if (simdi - lastGpsCheck > 5000) {
    sim808.println("AT+CGNSINF");
    lastGpsCheck = simdi;
  }

  if (simdi - lastHttpSend > 30000) {
    if (latitude_inf != 0.0 || longitude_inf != 0.0) {
      httpPostGonder(latitude_inf, longitude_inf);
    } else {
      Serial.println("GPS verisi yok, gönderim atlandı.");
    }
    lastHttpSend = simdi;
  }
}

void satirIsle(char *line) {
  if (strncmp(line, "+CGNSINF:", 9) == 0) {
    char *token;
    char *tempLine = strdup(line);
    char *rest = tempLine;
    int alanSayac = 0;

    while ((token = strtok_r(rest, ",", &rest))) {
      alanSayac++;
      if (alanSayac == 4) {
        latitude_inf = atof(token);
      } else if (alanSayac == 5) {
        longitude_inf = atof(token);
        Serial.print("[GPS] Enlem: ");
        Serial.println(latitude_inf, 6);
        Serial.print("[GPS] Boylam: ");
        Serial.println(longitude_inf, 6);
        Serial.println("--------------------");
        break;
      }
    }
    free(tempLine);
  }
}

void httpPostGonder(double lat, double lng) {
  String jsonData = "{\"lat\":";
  jsonData += String(lat, 6);
  jsonData += ",\"lng\":";
  jsonData += String(lng, 6);
  jsonData += "}";

  Serial.print("HTTP POST gönderiliyor: ");
  Serial.println(jsonData);

  sim808.println("AT+HTTPTERM");
  Serial.println(sim808Oku(2000));

  sim808.println("AT+HTTPINIT");
  String resp = sim808Oku(5000);
  Serial.println(resp);
  if (resp.indexOf("OK") == -1) return;

  sim808.println("AT+HTTPPARA=\"CID\",1");
  Serial.println(sim808Oku(2000));

  sim808.print("AT+HTTPPARA=\"URL\",\"");
  sim808.print(serverURL);
  sim808.println("\"");
  Serial.println(sim808Oku(2000));

  sim808.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  Serial.println(sim808Oku(2000));

  sim808.print("AT+HTTPDATA=");
  sim808.print(jsonData.length());
  sim808.println(",10000");
  String dataResp = sim808Oku(5000);
  Serial.println(dataResp);

  if (dataResp.indexOf("DOWNLOAD") != -1) {
    sim808.print(jsonData);
    Serial.println(sim808Oku(5000));
  } else {
    Serial.println("HTTPDATA komutu DOWNLOAD yanıtı vermedi.");
    return;
  }

  sim808.println("AT+HTTPACTION=1");
  delay(10000);
  Serial.println(sim808Oku(5000));

  sim808.println("AT+HTTPREAD");
  delay(5000);
  Serial.println(sim808Oku(5000));

  sim808.println("AT+HTTPTERM");
  delay(2000);
  Serial.println(sim808Oku(2000));
}
