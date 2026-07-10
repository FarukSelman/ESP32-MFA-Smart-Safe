# Donanım Bağlantı Rehberi (Pinout)

Sistemin kararlı çalışması için ESP32 üzerindeki pin dağılımı aşağıdaki gibi yapılmalıdır:

| Bileşen | Bileşen Pini | ESP32 Pin |
| :--- | :--- | :--- |
| **OLED Ekran** | SDA / SCL | GPIO 21 / 22 |
| **RFID Okuyucu** | SS (SDA) / SCK / MOSI / MISO / RST | GPIO 5 / 18 / 23 / 19 / 99 |
| **Servo Motor** | Sinyal Pini | GPIO 15 |
| **Keypad Satırlar** | R1, R2, R3, R4 | GPIO 13, 12, 14, 27 |
| **Keypad Sütunlar** | C1, C2, C3 | GPIO 26, 25, 33 |
| **Mesafe Sensörü** | TRIG / ECHO | GPIO 16 / 17 |
| **Buzzer & LED** | YEŞİL LED / KIRMIZI LED / BUZZER | GPIO 4 / 2 / 32 |
