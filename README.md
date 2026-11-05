# 🔒 ESP32 Multi-Factor Authentication (MFA) Smart Safe

Bu proje; **ESP32** mikrodenetleyicisi mimarisi üzerinde, yüksek güvenlik gereksinimlerini karşılamak amacıyla **çift aşamalı doğrulama (Multi-Factor Authentication)** lojiği ile geliştirilmiş akıllı bir dijital kasa sistemidir. Sistem, donanım bileşenlerini dört durumlu bir durum makinesi (State Machine) ile yönetir: Şifre bekle → Kart bekle → Açık, ayrıca Ceza kilidi.

---

## 🛠️ Öne Çıkan Özellikler & Çalışma Mantığı
Sistem, kasanın durumuna göre akıllı bir lojik akış yönetmektedir:
1. **1. Aşama (Şifre Doğrulama):** Sistem başlangıçta kilitli moddadır ve kullanıcıdan 4 haneli Matrix Keypad üzerinden şifre bekler. Girilen şifre maskelenerek (`*`) OLED ekrana basılır. Şifre doğrulandığında kısa bir onay sesiyle bir sonraki aşamaya geçilir.
2. **2. Aşama (RFID Kart Doğrulama):** Şifre aşaması geçildikten sonra MFRC522 RFID modülü aktif olur ve 20 saniye içinde kart okutulması beklenir (süre dolarsa veya `*` tuşuna basılırsa şifre ekranına dönülür). Yalnızca sisteme tanımlanmış olan Master Kart UID'si okutulduğunda kilit mekanizmasını kontrol eden Servo Motor yavaş senkronizasyonla (`servoYavasCek`) 90 derece dönerek kasayı açar.
3. **Hata ve Güvenlik Protokolü:** Şifre veya kartın hatalı girilmesi durumunda, sistem kırmızı LED'leri flaşör moduna alır, buzzer ile düşük frekanslı hata tonu verir ve güvenlik aşamasını tamamen sıfırlayarak en başa (şifre ekranına) döner. 3 hatalı denemeden sonra sistem 30 saniyelik **ceza kilidine** girer; her yeni ceza kilidinde süre iki katına çıkar (en fazla 8 dk).
4. **Akıllı Kapanma & Mesafe Sensörü:** Kasa açıldığında 10 saniyelik bir geri sayım başlar. Sistem önce kapağın gerçekten açılmasını bekler (mesafe > 8 cm); kapak açılıp tekrar kapatıldığında (mesafe < 5 cm) kasa sürenin dolmasını beklemeden otomatik olarak kendini kilitler. Kapak açık kalırken süre dolarsa sesli alarm verilir; kilit açıldığı hâlde kapak hiç açılmazsa kasa süre sonunda kendini yeniden kilitler. Ölçümler 3 okumanın medyanı alınarak gürültüye karşı korunur.

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
| **RFID Modülü** | SS (SDA) / SCK / MOSI / MISO / RST | SPI Standart (GPIO 5 / 18 / 23 / 19) / RST → 3.3V |
| **Servo Motor** | Signal Pin | GPIO 15 |
| **Keypad Satırlar** | R1, R2, R3, R4 | GPIO 13, 12, 14, 27 |
| **Keypad Sütunlar** | C1, C2, C3 | GPIO 26, 25, 33 |
| **Mesafe Sensörü** | TRIG / ECHO | GPIO 16 / 17 (ECHO gerilim bölücü ile) |
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
5. Seri Monitör'ü (115200 baud) açıp master kartınızı okutun; ekranda yazan `Okunan UID` değerini koddaki `ANA_KART_UID` dizisine girip kodu tekrar yükleyin.

> ⚠️ **Donanım notları:** HC-SR04'ün ECHO çıkışı 5V'tur; ESP32'ye gerilim bölücü (örn. 1kΩ seri + 2kΩ toprağa) üzerinden bağlayın. GPIO 12 bir strapping pinidir; açılışta sorun yaşarsanız keypad R2 satırını başka bir pine taşıyın.
>
> 🔐 **Güvenlik notu:** Bu bir eğitim projesidir. Şifre ve kart UID'si kodun içindedir ve yalnızca UID'ye dayalı RFID doğrulaması kopyalanabilir kartlarla aşılabilir.

---
💡 *Bu proje, gömülü sistemler, nesnelerin interneti (IoT) ve donanım-yazılım entegrasyonu mimarilerini uygulamalı olarak deneyimlemek amacıyla geliştirilmiş bağımsız bir mühendislik çalışmasıdır.*
