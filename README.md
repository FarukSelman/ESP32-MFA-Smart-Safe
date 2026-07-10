# 🔒 ESP32 Multi-Factor Authentication (MFA) Smart Safe

Bu proje; **ESP32** mikrodenetleyicisi mimarisi üzerinde, yüksek güvenlik gereksinimlerini karşılamak amacıyla **çift aşamalı doğrulama (Multi-Factor Authentication)** lojiği ile geliştirilmiş akıllı bir dijital kasa sistemidir. Sistem, donanım bileşenleri ile asenkron durum yönetimini (State Machine) başarıyla entegre eder.

---

## 🛠️ Öne Çıkan Özellikler & Çalışma Mantığı
Sistem, kasanın durumuna göre akıllı bir lojik akış yönetmektedir:
1. **1. Aşama (Şifre Doğrulama):** Sistem başlangıçta kilitli moddadır ve kullanıcıdan 4 haneli Matrix Keypad üzerinden şifre bekler. Girilen şifre maskedilerek (`*`) OLED ekrana basılır. Şifre doğrulandığında kısa bir onay sesiyle bir sonraki aşamaya geçilir.
2. **2. Aşama (Biyometrik/Kart Doğrulama):** Şifre aşaması geçildikten sonra tuş takımı kilitlenir ve MFRC522 RFID modülü aktif olur. Yalnızca sisteme tanımlanmış olan Master Kart UID'si okutulduğunda kilit mekanizmasını kontrol eden Servo Motor yavaş senkronizasyonla (`servoYavasCek`) 90 derece dönerek kasayı açar.
3. **Hata ve Güvenlik Protokolü:** Şifre veya kartın hatalı girilmesi durumunda, sistem kırmızı LED'leri flaşör moduna alır, buzzer ile düşük frekanslı hata tonu verir ve güvenlik aşamasını tamamen sıfırlayarak en başa (şifre ekranına) döner.
4. **Akıllı Kapanma & Mesafe Sensörü:** Kasa açıldığında 10 saniyelik bir geri sayım başlar. Eğer süre dolarsa kasa sesli uyarı verir. Ancak kapak kapatılıp HC-SR04 ultrasonik sensör mesafesi 5 cm'nin altına düştüğü an, kasa sürenin dolmasını beklemeden otomatik olarak kendini kilitler.

---

## 🏗️ Donanım Bileşenleri
- **Mikrodenetleyici:** ESP32 NodeMCU
- **Doğrulama Araçları:** 4x3 Matrix Keypad & MFRC522 RFID Okuyucu
- **Görselleştirme:** SSD1306 OLED Ekran (128x64, I2C)
- **Mekanik Kilit:** SG90 Servo Motor
- **Geri Bildirim & Sensörler:** HC-SR04 Ultrasonik Mesafe Sensörü, Buzzer, Yeşil/Kırmızı LED'ler

---

## 🎛️ Donanım Pin Eşleşmeleri (Pinout)

Sistemin kararlı çalışması için ESP32 üzerindeki pin dağılımı şu şekilde konfigüre edilmiştir:

| Bileşen | Bileşen Pini | ESP32 Pin |
| :--- | :--- | :--- |
| **OLED Ekran** | SDA / SCL | I2C Standart (GPIO 21 / 22) |
| **RFID Modülü** | SS (SDA) / SCK / MOSI / MISO | SPI Standart (GPIO 5 / 18 / 23 / 19) |
| **Servo Motor** | Signal Pin | GPIO 15 |
| **Keypad Satırlar** | R1, R2, R3, R4 | GPIO 13, 12, 14, 27 |
| **Keypad Sütunlar** | C1, C2, C3 | GPIO 26, 25, 33 |
| **Mesafe Sensörü** | TRIG / ECHO | GPIO 16 / 17 |
| **Geri Bildirim** | YEŞİL LED / KIRMIZI LED / BUZZER | GPIO 4 / 2 / 32 |

---

## 🚀 Gerekli Kütüphaneler ve Kurulum

Projeyi Arduino IDE veya VS Code (PlatformIO) üzerinde derlemek için aşağıdaki kütüphanelerin yüklü olması gerekir:
- `Adafruit_SSD1306` & `Adafruit_GFX`
- `ESP32Servo`
- `MFRC522`
- `Keypad`

### Çalıştırma Adımları:
1. Repoyu bilgisayarınıza klonlayın.
2. `src/smart_safe.ino` dosyasını Arduino IDE ile açın.
3. ESP32 kart seçiminizi yapın ve pin bağlantılarını şemaya uygun şekilde tamamlayın.
4. Kodu derleyip ESP32'nize yükleyin.

---
💡 *Bu proje, gömülü sistemler, nesnelerin interneti (IoT) ve donanım-yazılım entegrasyonu mimarilerini uygulamalı olarak deneyimlemek amacıyla geliştirilmiş bağımsız bir mühendislik çalışmasıdır.*
