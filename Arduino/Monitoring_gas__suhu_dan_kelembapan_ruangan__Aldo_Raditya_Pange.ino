//** Sistem Monitoring Suhu, Kelembapan, Gas, dan Kendali Kipas **//
// Menggunakan DHT22, MQ-135, LCD I2C, ESP8266, 2 Kipas, dan 1 LED Indikator Suhu Tinggi
#define BLYNK_TEMPLATE_ID "TMPL6SKlq0tBH"
#define BLYNK_TEMPLATE_NAME "Alat Suhu Kelembapan dan Gas"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiClientSecure.h>
#include <vector>

// -- Konfigurasi WiFi dan Blynk --
char auth[] = "BwQR_sLUL5Dy--FymWfVrELCjUVW4DpP";
char ssid[] = "Aku denganya";
char pass[] = "n7kfs8gs6j6dsda";

// -- Konfigurasi Telegram Bot --
#define BOT_TOKEN "6799315217:AAGnfNrp2sDy_i8ZUMKpoUpfCK-fdvzA5Ko"
std::vector<String> chatIds = {"7183247801", "1629663028"}; // Ganti dengan ID chat sebenarnya
WiFiClientSecure client;
String telegramUrl;

// -- Pin Sensor dan Aktuator --
#define DHTPIN D4       // Pin DHT22
#define MQ135PIN A0     // Pin MQ-135
#define FAN1PIN D5      // Pin Kipas 1
#define FAN2PIN D6      // Pin Kipas 2
#define LED_SUHU_TINGGI_PIN D7 // Pin LED Indikator Suhu Tinggi (sebelumnya Kipas 3)

// -- Objek Sensor dan LCD --
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variabel untuk menyimpan kondisi terakhir yang telah diberitahukan
bool notifiedHot = false;
bool notifiedLowHumidity = false;
bool notifiedHighHumidity = false;
bool notifiedBadGas = false;

// Variabel untuk menyimpan status kontrol manual kipas
bool manualFan1 = false;
bool manualFan2 = false;

// Variabel untuk menyimpan status mode suhu tinggi
bool highTempModeActive = false; // Mengindikasikan apakah mode suhu tinggi aktif (LED menyala)

// Blynk timer
BlynkTimer timer;

void setup() {
  Serial.begin(115200);

  // Koneksi WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Inisialisasi Blynk
  Blynk.begin(auth, ssid, pass);

  // Inisialisasi Telegram
  client.setInsecure();
  telegramUrl = "https://api.telegram.org/bot" + String(BOT_TOKEN) + "/sendMessage";

  // Inisialisasi Sensor dan LCD
  dht.begin();
  lcd.init();           // Inisialisasi LCD
  lcd.backlight();      // Mengaktifkan lampu latar
  lcd.setCursor(0, 0);
  lcd.print("Monitoring Start");

  // Inisialisasi Pin sebagai OUTPUT
  pinMode(FAN1PIN, OUTPUT);
  pinMode(FAN2PIN, OUTPUT);
  pinMode(LED_SUHU_TINGGI_PIN, OUTPUT); // LED_SUHU_TINGGI_PIN sebagai OUTPUT
  digitalWrite(FAN1PIN, HIGH);      // Kipas awal mati (HIGH karena active low)
  digitalWrite(FAN2PIN, HIGH);      // Kipas awal mati (HIGH karena active low)
  digitalWrite(LED_SUHU_TINGGI_PIN, LOW); // LED awal mati (LOW karena active HIGH)

  // Inisialisasi status kipas di Blynk
  Blynk.virtualWrite(V6, "Otomatis"); // Mode Kipas 1
  Blynk.virtualWrite(V7, "Otomatis"); // Mode Kipas 2
  Blynk.virtualWrite(V12, "Otomatis"); // Mode Kipas 2
  Blynk.virtualWrite(V8, 0);          // Tombol Kipas 1 Manual (0 = OFF, 1 = ON)
  Blynk.virtualWrite(V9, 0);          // Tombol Kipas 2 Manual (0 = OFF, 1 = ON)
  Blynk.virtualWrite(V10, 0);         // Tombol Kipas 1 Otomatis (0 = OFF, 1 = ON)
  Blynk.virtualWrite(V11, 0);         // Tombol Kipas 2 Otomatis (0 = OFF, 1 = ON)

  // Set timer untuk membaca sensor dan mengirim data ke Blynk secara berkala
  timer.setInterval(2000L, readAndSendSensorData);
}

// Fungsi untuk membaca data sensor dan mengirimkannya ke Blynk
void readAndSendSensorData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(MQ135PIN);

  // Evaluasi kondisi sensor
  String tempStatus = evaluateTemperature(temperature);
  String humStatus = evaluateHumidity(humidity);
  String gasStatus = evaluateGas(gasValue);

  // Menampilkan data di Serial Monitor
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print("C - "); Serial.println(tempStatus);
  Serial.print("Hum: "); Serial.print(humidity);
  Serial.print("% - "); Serial.println(humStatus);
  Serial.print("Gas: "); Serial.print(gasValue);
  Serial.print(" - "); Serial.println(gasStatus);

  // Menampilkan data di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperature); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("H:"); lcd.print(humidity); lcd.print("%");
  lcd.print("G:"); lcd.print(gasValue); lcd.print("PPM");

  Blynk.virtualWrite(V1, temperature);
  Blynk.virtualWrite(V2, humidity);
  Blynk.virtualWrite(V3, gasValue);

  // --- Kendali Kipas/LED Otomatis ---
  // Logika overriding untuk suhu tinggi
  if (temperature > 30) { // Ambang batas suhu tinggi
    if (!highTempModeActive) {
      Serial.println("HIGH TEMPERATURE DETECTED! Activating LED, turning off fans.");
      highTempModeActive = true;
      // Pastikan kontrol manual direset ke mode otomatis agar tidak ada konflik
      manualFan1 = false;
      manualFan2 = false;
      Blynk.virtualWrite(V10, 1); // Set Kipas 1 Otomatis ON (visual di Blynk)
      Blynk.virtualWrite(V11, 1); // Set Kipas 2 Otomatis ON (visual di Blynk)
      Blynk.virtualWrite(V8, 0);  // Set Kipas 1 Manual OFF
      Blynk.virtualWrite(V9, 0);  // Set Kipas 2 Manual OFF
    }
    digitalWrite(LED_SUHU_TINGGI_PIN, HIGH); // Nyalakan LED
    digitalWrite(FAN1PIN, HIGH);             // Matikan Kipas 1 (HIGH karena active low)
    digitalWrite(FAN2PIN, HIGH);             // Matikan Kipas 2 (HIGH karena active low)
    Blynk.virtualWrite(V12, "Otomatis - Menyala");
    Blynk.virtualWrite(V6, "Otomatis - Mati");
    Blynk.virtualWrite(V7, "Otomatis - Mati");
  } else { // Operasi normal ketika suhu tidak kritis
    if (highTempModeActive) { // Jika baru saja keluar dari mode suhu tinggi
      Serial.println("Temperature returned to normal. Resuming individual fan control.");
      highTempModeActive = false;
      // Panggil kembali logika otomatis untuk Kipas 1 dan Kipas 2 segera
      // Ini akan membuat kipas merespons kondisi kelembapan/gas saat ini
      if (!manualFan1) { // Hanya jika tidak dalam mode manual
        if (humidity <= 40 || humidity > 55) {
          digitalWrite(FAN1PIN, LOW); // Hidupkan kipas 1 jika kondisi tidak aman
          Blynk.virtualWrite(V6, "Otomatis - Menyala");
        } else {
          digitalWrite(FAN1PIN, HIGH); // Matikan kipas 1 jika kondisi aman
          Blynk.virtualWrite(V6, "Otomatis - Mati");
        }
      }

      if (!manualFan2) { // Hanya jika tidak dalam mode manual
        if (gasValue > 600) {
          digitalWrite(FAN2PIN, LOW); // Hidupkan kipas 2 jika gas berbahaya
          Blynk.virtualWrite(V7, "Otomatis - Menyala");
        } else {
          digitalWrite(FAN2PIN, HIGH); // Matikan kipas 2 jika gas aman
          Blynk.virtualWrite(V7, "Otomatis - Mati");
        }
      }
    }
    digitalWrite(LED_SUHU_TINGGI_PIN, LOW); // Pastikan LED mati saat suhu normal
    Blynk.virtualWrite(V12, "Otomatis - Mati");

    // Logika kontrol Kipas 1 (berdasarkan kelembapan) jika tidak dalam mode suhu tinggi dan tidak manual
    if (!highTempModeActive && !manualFan1) {
      if (humidity <= 40 || humidity > 55) {
        digitalWrite(FAN1PIN, LOW); // Hidupkan kipas 1 jika kondisi kelembapan tidak aman
        Blynk.virtualWrite(V6, "Otomatis - Menyala");
      } else {
        digitalWrite(FAN1PIN, HIGH); // Matikan kipas 1 jika kondisi kelembapan aman
        Blynk.virtualWrite(V6, "Otomatis - Mati");
      }
    }

    // Logika kontrol Kipas 2 (berdasarkan gas) jika tidak dalam mode suhu tinggi dan tidak manual
    if (!highTempModeActive && !manualFan2) {
      if (gasValue > 600) {
        digitalWrite(FAN2PIN, LOW); // Hidupkan kipas 2 jika gas berbahaya
        Blynk.virtualWrite(V7, "Otomatis - Menyala");
      } else {
        digitalWrite(FAN2PIN, HIGH); // Matikan kipas 2 jika gas aman
        Blynk.virtualWrite(V7, "Otomatis - Mati");
      }
    }
  }

  // Kirim notifikasi hanya untuk kondisi yang sesuai (selain notifikasi suhu tinggi yang sudah ada di atas)
  if (temperature > 30 && !notifiedHot) {
    sendTelegramAlert("Panas: Suhu ruangan terlalu panas (" + String(temperature) + "C)", temperature, humidity, gasValue);
    notifiedHot = true;
  } else if (temperature <= 30 && notifiedHot) {
    sendTelegramAlert("Suhu kembali normal.", temperature, humidity, gasValue);
    notifiedHot = false;
  }

  if (humStatus.startsWith("Berbahaya: Kelembapan rendah") && !notifiedLowHumidity) {
    sendTelegramAlert(humStatus, temperature, humidity, gasValue);
    notifiedLowHumidity = true;
  } else if (humStatus == "Aman" && notifiedLowHumidity) {
    sendTelegramAlert("Kelembapan kembali normal.", temperature, humidity, gasValue);
    notifiedLowHumidity = false;
  }

  if (humStatus.startsWith("Berbahaya: Kelembapan tinggi") && !notifiedHighHumidity) {
    sendTelegramAlert(humStatus, temperature, humidity, gasValue);
    notifiedHighHumidity = true;
  } else if (humStatus == "Aman" && notifiedHighHumidity) {
    sendTelegramAlert("Kelembapan kembali normal.", temperature, humidity, gasValue);
    notifiedHighHumidity = false;
  }

  if (gasStatus.startsWith("Bad:") && !notifiedBadGas) {
    sendTelegramAlert(gasStatus, temperature, humidity, gasValue);
    notifiedBadGas = true;
  } else if (!gasStatus.startsWith("Bad:") && notifiedBadGas) {
    sendTelegramAlert("Konsentrasi gas kembali normal.", temperature, humidity, gasValue);
    notifiedBadGas = false;
  }
}

// Blynk function untuk mengontrol Kipas 1 secara manual
BLYNK_WRITE(V8) {
  if (highTempModeActive) { // Cegah kontrol manual jika mode suhu tinggi aktif
    Blynk.virtualWrite(V8, 0); // Reset status tombol di Blynk
    return;
  }
  if (param.asInt() == 1) { // Tombol ON Manual
    digitalWrite(FAN1PIN, LOW); // Hidupkan Kipas 1
    Blynk.virtualWrite(V6, "Manual - Menyala");
    manualFan1 = true;
    Blynk.virtualWrite(V10, 0); // Pastikan tombol otomatis mati
  } else { // Tombol OFF Manual
    digitalWrite(FAN1PIN, HIGH); // Matikan Kipas 1
    Blynk.virtualWrite(V6, "Manual - Mati");
    manualFan1 = true;
    Blynk.virtualWrite(V10, 0); // Pastikan tombol otomatis mati
  }
}

// Blynk function untuk mengontrol Kipas 2 secara manual
BLYNK_WRITE(V9) {
  if (highTempModeActive) { // Cegah kontrol manual jika mode suhu tinggi aktif
    Blynk.virtualWrite(V9, 0); // Reset status tombol di Blynk
    return;
  }
  if (param.asInt() == 1) { // Tombol ON Manual
    digitalWrite(FAN2PIN, LOW); // Hidupkan Kipas 2
    Blynk.virtualWrite(V7, "Manual - Menyala");
    manualFan2 = true;
    Blynk.virtualWrite(V11, 0); // Pastikan tombol otomatis mati
  } else { // Tombol OFF Manual
    digitalWrite(FAN2PIN, HIGH); // Matikan Kipas 2
    Blynk.virtualWrite(V7, "Manual - Mati");
    manualFan2 = true;
    Blynk.virtualWrite(V11, 0); // Pastikan tombol otomatis mati
  }
}

// Blynk function untuk mengembalikan Kipas 1 ke mode otomatis
BLYNK_WRITE(V10) {
  if (highTempModeActive) { // Cegah pengalihan ke otomatis jika mode suhu tinggi aktif
    Blynk.virtualWrite(V10, 0); // Reset status tombol di Blynk
    return;
  }
  if (param.asInt() == 1) { // Tombol ON Otomatis
    manualFan1 = false;
    Blynk.virtualWrite(V6, "Otomatis");
    Blynk.virtualWrite(V8, 0); // Pastikan tombol manual mati
  }
}

// Blynk function untuk mengembalikan Kipas 2 ke mode otomatis
BLYNK_WRITE(V11) {
  if (highTempModeActive) { // Cegah pengalihan ke otomatis jika mode suhu tinggi aktif
    Blynk.virtualWrite(V11, 0); // Reset status tombol di Blynk
    return;
  }
  if (param.asInt() == 1) { // Tombol ON Otomatis
    manualFan2 = false;
    Blynk.virtualWrite(V7, "Otomatis");
    Blynk.virtualWrite(V9, 0); // Pastikan tombol manual mati
  }
}

void loop() {
  Blynk.run();
  timer.run(); // Jalankan timer Blynk
}

// Fungsi Evaluasi Suhu
String evaluateTemperature(float temp) {
  if (temp < 25) return "Dingin: Suhu ruangan terlalu dingin (" + String(temp) + "C)";
  else if (temp <= 30) return "Aman";
  else return "Panas: Suhu ruangan terlalu panas (" + String(temp) + "C)";
}

// Fungsi Evaluasi Kelembapan
String evaluateHumidity(float hum) {
  if (hum <= 40) return "Berbahaya: Kelembapan rendah, risiko tinggi (" + String(hum) + "%)";
  else if (hum > 40 && hum <= 55) return "Aman";
  else return "Berbahaya: Kelembapan tinggi, risiko tinggi (" + String(hum) + "%)";
}

// Fungsi Evaluasi Gas (menggunakan rentang 0-1000)
String evaluateGas(int gas) {
  if (gas < 100) return "Fresh: Konsentrasi gas dalam batas bersih (" + String(gas) + ")";
  else if (gas < 600) return "Cukup: Konsentrasi gas dalam batas stabil (" + String(gas) + ")";
  else return "Bad: Konsentrasi gas dalam batas bahaya (" + String(gas) + ")";
}

// Fungsi Kirim Notifikasi Telegram
void sendTelegramAlert(String alertMessage, float temp, float hum, int gas) {
  String message = "\u26A0 ALERT! " + alertMessage + "\n";
  message += "Suhu: " + String(temp) + "C\n";
  message += "Kelembapan: " + String(hum) + "%\n";
  message += "Gas: " + String(gas);

  for (const String& chatId : chatIds) {
    String url = telegramUrl + "?chat_id=" + chatId + "&text=" + message;
    if (client.connect("api.telegram.org", 443)) {
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: api.telegram.org\r\n" +
                   "Connection: close\r\n\r\n");
    } else {
      Serial.print("Gagal terhubung ke api.telegram.org untuk mengirim ke chat ID: ");
      Serial.println(chatId);
    }
    client.stop(); // Penting untuk menutup koneksi setelah setiap pengiriman
    delay(500);    // Beri jeda antar pengiriman untuk menghindari rate limiting
  }
}
