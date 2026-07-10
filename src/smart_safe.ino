#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

// --- PIN TANIMLAMALARI ---
#define EKRAN_GENISLIGI 128
#define EKRAN_YUKSEKLIGI 64
#define OLED_RESET -1
Adafruit_SSD1306 display(EKRAN_GENISLIGI, EKRAN_YUKSEKLIGI, &Wire, OLED_RESET);

#define YESIL_LED 4
#define KIRMIZI_LED 2
#define BUZZER 32
#define SERVO_PIN 15

#define TRIG_PIN 16
#define ECHO_PIN 17

#define SS_PIN 5
#define RST_PIN 99 

Servo kilitServo;
MFRC522 rfid(SS_PIN, RST_PIN);

// --- KEYPAD AYARLARI ---
const byte ROWS = 4; 
const byte COLS = 3; 
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 12, 14, 27}; 
byte colPins[COLS] = {26, 25, 33};     
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// --- SİSTEM DEĞİŞKENLERİ ---
String sifre = "1234";       
String girilenSifre = "";     
String anaKartID = "43C1214"; 

bool kilitDurumu = true;      
int guvenlikAsamasi = 0;      // 0 = Şifre bekleniyor, 1 = Kart bekleniyor
unsigned long kasaAcilisZamani = 0;
int beklemeSuresiSn = 10; 

void setup() {
  Serial.begin(115200);
  
  pinMode(YESIL_LED, OUTPUT);
  pinMode(KIRMIZI_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  digitalWrite(KIRMIZI_LED, HIGH);
  digitalWrite(YESIL_LED, LOW);
  
  kilitServo.setPeriodHertz(50);
  kilitServo.attach(SERVO_PIN, 500, 2400);
  kilitServo.write(0); 

  SPI.begin();
  rfid.PCD_Init();
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  ekranYaz("AKILLI KASA", "2 Adimli Guvenlik", "");
  delay(2000);
  
  // Başlangıçta şifre ekranını göster
  guvenlikAsamasi = 0;
  ekranYaz("KILITLI", "Sifre Giriniz:", "");
}

void loop() {
  if(kilitDurumu) {
    kilitliMod();
  } else {
    acikMod(); 
  }
}

// --- YAVAS SERVO FONKSIYONU ---
void servoYavasCek(int hedefDerece) {
  int simdikiDerece = kilitServo.read();
  
  if (simdikiDerece < hedefDerece) {
    for (int i = simdikiDerece; i <= hedefDerece; i++) {
      kilitServo.write(i);
      delay(20); 
    }
  } else {
    for (int i = simdikiDerece; i >= hedefDerece; i--) {
      kilitServo.write(i);
      delay(20);
    }
  }
}

// --- MESAFE ÖLÇME ---
float mesafeOlcer() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long sure = pulseIn(ECHO_PIN, HIGH, 30000); 
  if(sure == 0) return 999.0; 
  float mesafe = sure * 0.034 / 2;
  return mesafe;
}

// --- KİLİTLİ DURUM (ÇİFT AŞAMALI) ---
void kilitliMod() {
  
  // --- 1. AŞAMA: ŞİFRE İŞLEMLERİ ---
  if(guvenlikAsamasi == 0) {
    char key = keypad.getKey();
    
    if(key) {
      if(key == '*') { 
        girilenSifre = "";
        ekranYaz("KILITLI", "Sifre Giriniz:", "");
      } else if(key == '#') { 
        sifreKontrol(); // Şifreyi kontrol et, doğruysa 2. aşamaya geç
      } else {
        if(girilenSifre.length() < 4) {
          girilenSifre += key;
          String yildizlar = "";
          for(int i=0; i<girilenSifre.length(); i++) yildizlar += "*";
          ekranYaz("KILITLI", "Sifre:", yildizlar);
        }
      }
    }
  }
  
  // --- 2. AŞAMA: KART İŞLEMLERİ ---
  else if(guvenlikAsamasi == 1) {
    // Tuş takımını burada devre dışı bıraktık, sadece kart okumaya bakıyoruz
    
    if(!rfid.PICC_IsNewCardPresent()) return;
    if(!rfid.PICC_ReadCardSerial()) return;

    String okunanID = "";
    for(byte i=0; i<rfid.uid.size; i++) {
      okunanID += String(rfid.uid.uidByte[i], HEX);
    }
    okunanID.toUpperCase();
    
    if(okunanID == anaKartID) {
      kasaAc(); // Şifre doğru, kart doğru -> KASA AÇILIR
    } else {
      hataliGiris("Kart Hatali!"); // Şifre doğruydu ama kart yanlış
    }
    rfid.PICC_HaltA();
  }
}

void sifreKontrol() {
  if(girilenSifre == sifre) {
    // Şifre doğru! 2. aşamaya geç
    guvenlikAsamasi = 1;
    tone(BUZZER, 1500); delay(100); // Kısa onay sesi
    noTone(BUZZER);
    ekranYaz("1. ASAMA BASARILI", "Lutfen Kart", "Okutun...");
  } else {
    hataliGiris("Yanlis Sifre!"); // Şifre yanlış, 2. aşamaya geçme
  }
  girilenSifre = "";
}

// --- AÇIK DURUM ---
void acikMod() {
  float mesafe = mesafeOlcer();
  
  if(mesafe < 5.0) { 
    noTone(BUZZER); 
    kasaKapat();
    return;
  }
  
  unsigned long gecenSure = (millis() - kasaAcilisZamani) / 1000;
  int kalanSure = beklemeSuresiSn - gecenSure;
  
  if(kalanSure > 0) {
    String yazi = "Kapanma: " + String(kalanSure) + " sn";
    ekranYaz("KASA ACIK", yazi, "");
  } else {
    ekranYaz("UYARI!", "SURE DOLDU", "Kapagi kapatin!");
    
    if((millis() / 300) % 2 == 0) {
      tone(BUZZER, 1000); 
    } else {
      noTone(BUZZER);
    }
  }
}

// --- KASA AÇMA / KAPAMA ---
void kasaAc() {
  kilitDurumu = false;
  kasaAcilisZamani = millis(); 
  guvenlikAsamasi = 0; // Sıfırla
  
  digitalWrite(KIRMIZI_LED, LOW);
  digitalWrite(YESIL_LED, HIGH);
  
  tone(BUZZER, 1000); delay(100);
  tone(BUZZER, 1500); delay(100);
  noTone(BUZZER);
  
  servoYavasCek(90); 
  ekranYaz("KASA ACILDI!", "Guvenlik Dogrulandi", "10 sn icinde kapanir");
}

void kasaKapat() {
  kilitDurumu = true;
  guvenlikAsamasi = 0; // Sıfırla, bir sonraki açılışta tekrar şifre istesin
  
  digitalWrite(KIRMIZI_LED, HIGH);
  digitalWrite(YESIL_LED, LOW);
  
  servoYavasCek(0); 
  ekranYaz("KILITLI", "Sifre Giriniz:", "");
}

void hataliGiris(String hataTuru) {
  tone(BUZZER, 300); delay(500);
  noTone(BUZZER);
  
  ekranYaz("HATA!", hataTuru, "Tekrar deneyin...");
  
  for(int i=0; i<3; i++) {
    digitalWrite(KIRMIZI_LED, LOW); delay(200);
    digitalWrite(KIRMIZI_LED, HIGH); delay(200);
  }
  
  guvenlikAsamasi = 0; // Hata yapınca başa dön, tekrar şifre istesin
  delay(1500);
  ekranYaz("KILITLI", "Sifre Giriniz:", "");
}

// --- EKRAN YAZMA ---
void ekranYaz(String satir1, String satir2, String satir3) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(satir1);
  
  display.setTextSize(1);
  display.setCursor(0, 25);
  display.println(satir2);
  display.setCursor(0, 40);
  display.println(satir3);
  display.display();
}
