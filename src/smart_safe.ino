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

// NOT: HC-SR04 ECHO cikisi 5V'tur. ESP32 pinleri 3.3V toleranslidir;
// ECHO ile GPIO17 arasina gerilim bolucu koyun (ornegin 1k seri + 2k toprağa).
#define TRIG_PIN 16
#define ECHO_PIN 17

#define SS_PIN 5
// RC522'nin RST ucu 3.3V'a baglidir; kutuphane yazilimsal reset kullanir.
// RST'yi bos bir GPIO'ya baglarsaniz buraya o pinin numarasini yazin.
// (Eski deger olan 99, ESP32'de gecerli bir GPIO degildi.)
const byte RST_PIN = MFRC522::UNUSED_PIN;

Servo kilitServo;
MFRC522 rfid(SS_PIN, RST_PIN);

// --- KEYPAD AYARLARI ---
// NOT: GPIO 12 bir strapping pinidir (flash voltajini belirler). Acilista
// sorun yasarsaniz R2 satirini baska bir pine tasiyin.
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

// --- GUVENLIK AYARLARI ---
const char SIFRE[] = "1234";

// Master kartin UID'si, bayt bayt.
// ONEMLI: Eski "43C1214" degeri baştaki sifirlari kaybettigi icin belirsizdi.
// Kartinizi okutun, Seri Monitor'de (115200) yazan "Okunan UID" degerini
// buraya bayt bayt girin. Asagidaki deger yalnizca bir tahmindir.
const byte ANA_KART_UID[] = {0x43, 0xC1, 0x21, 0x04};

const unsigned long KART_ZAMAN_ASIMI_MS = 20000; // 2. asamada kart bekleme suresi
const int  ACIK_KALMA_SN   = 10;     // Kasa acildiktan sonraki geri sayim
const int  MAX_HATA        = 3;      // Kac hatali denemeden sonra kilitlenir
const unsigned long TEMEL_CEZA_MS = 30000; // Ilk kilitlenme suresi (her seferde 2 katina cikar)
const int  MAX_CEZA_SEVIYESI = 4;    // En fazla 30 sn x 2^4 = 8 dk

const float KAPALI_ESIK_CM = 5.0;    // Bunun altinda kapak kapali sayilir
const float ACIK_ESIK_CM   = 8.0;    // Bunun ustunde kapak acik sayilir (histerezis)

// --- DURUM MAKINESI ---
enum Durum {
  SIFRE_BEKLE,   // 1. asama: sifre bekleniyor
  KART_BEKLE,    // 2. asama: kart bekleniyor
  CEZA_KILIDI,   // Cok fazla hatali deneme: gecici kilit
  ACIK           // Kasa acik
};
Durum durum = SIFRE_BEKLE;

String girilenSifre = "";
int hataSayisi = 0;
int cezaSeviyesi = 0;
unsigned long cezaBaslangic = 0;
unsigned long cezaSuresi = 0;
unsigned long kartAsamaBaslangic = 0;
unsigned long kasaAcilisZamani = 0;
bool kapakAcildi = false;
int sonGosterilenSn = -1; // Ekrani yalnizca deger degisince guncellemek icin

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

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("HATA: OLED ekran bulunamadi!"));
  }
  ekranYaz("AKILLI KASA", "2 Adimli Guvenlik", "");
  delay(2000);

  sifreEkraninaDon();
}

void loop() {
  switch(durum) {
    case SIFRE_BEKLE: sifreModu(); break;
    case KART_BEKLE:  kartModu();  break;
    case CEZA_KILIDI: cezaModu();  break;
    case ACIK:        acikMod();   break;
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

// --- MESAFE OLCME ---
float tekOlcum() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long sure = pulseIn(ECHO_PIN, HIGH, 30000);
  if(sure == 0) return 999.0; // Yanki yok / menzil disi
  return sure * 0.034 / 2;
}

// Gurultuyu azaltmak icin 3 olcumun medyani
float mesafeOlcer() {
  float a = tekOlcum(); delay(10);
  float b = tekOlcum(); delay(10);
  float c = tekOlcum();
  float t;
  if(a > b) { t = a; a = b; b = t; }
  if(b > c) { t = b; b = c; c = t; }
  if(a > b) { t = a; a = b; b = t; }
  return b;
}

// --- 1. ASAMA: SIFRE ---
void sifreModu() {
  char key = keypad.getKey();
  if(!key) return;

  if(key == '*') {
    girilenSifre = "";
    ekranYaz("KILITLI", "Sifre Giriniz:", "");
  } else if(key == '#') {
    if(girilenSifre.length() > 0) sifreKontrol();
  } else if(girilenSifre.length() < 4) {
    girilenSifre += key;
    String yildizlar = "";
    for(unsigned int i = 0; i < girilenSifre.length(); i++) yildizlar += "*";
    ekranYaz("KILITLI", "Sifre:", yildizlar);
  }
}

void sifreKontrol() {
  bool dogru = (girilenSifre == SIFRE);
  girilenSifre = "";

  if(dogru) {
    durum = KART_BEKLE;
    kartAsamaBaslangic = millis();
    sonGosterilenSn = -1;
    tone(BUZZER, 1500); delay(100);
    noTone(BUZZER);
  } else {
    hataliGiris("Yanlis Sifre!");
  }
}

// --- 2. ASAMA: KART ---
void kartModu() {
  unsigned long gecen = millis() - kartAsamaBaslangic;

  // Zaman asimi: sifre dogru girilip birakildiysa basa don
  if(gecen >= KART_ZAMAN_ASIMI_MS) {
    tone(BUZZER, 600); delay(200); noTone(BUZZER);
    ekranYaz("SURE BITTI", "Sifreyi tekrar", "giriniz");
    delay(1500);
    sifreEkraninaDon();
    return;
  }

  // '*' ile iptal edip sifre ekranina donulebilir
  if(keypad.getKey() == '*') {
    sifreEkraninaDon();
    return;
  }

  int kalan = (KART_ZAMAN_ASIMI_MS - gecen + 999) / 1000;
  if(kalan != sonGosterilenSn) {
    sonGosterilenSn = kalan;
    ekranYaz("ASAMA 1 OK", "Lutfen kart okutun", "Kalan: " + String(kalan) + " sn");
  }

  if(!rfid.PICC_IsNewCardPresent()) return;
  if(!rfid.PICC_ReadCardSerial()) return;

  uidYazdir();
  bool kartDogru = (rfid.uid.size == sizeof(ANA_KART_UID)) &&
                   (memcmp(rfid.uid.uidByte, ANA_KART_UID, sizeof(ANA_KART_UID)) == 0);
  rfid.PICC_HaltA();

  if(kartDogru) {
    kasaAc();
  } else {
    hataliGiris("Kart Hatali!");
  }
}

// Okunan UID'yi her bayti 2 haneli olacak sekilde Seri Monitor'e yazar
void uidYazdir() {
  Serial.print(F("Okunan UID: {"));
  for(byte i = 0; i < rfid.uid.size; i++) {
    char buf[6];
    sprintf(buf, "0x%02X", rfid.uid.uidByte[i]);
    Serial.print(buf);
    if(i < rfid.uid.size - 1) Serial.print(F(", "));
  }
  Serial.println(F("}"));
}

// --- HATALI DENEME VE CEZA KILIDI ---
void hataliGiris(String hataTuru) {
  hataSayisi++;

  tone(BUZZER, 300); delay(500);
  noTone(BUZZER);

  ekranYaz("HATA!", hataTuru, "Deneme: " + String(hataSayisi) + "/" + String(MAX_HATA));

  for(int i = 0; i < 3; i++) {
    digitalWrite(KIRMIZI_LED, LOW); delay(200);
    digitalWrite(KIRMIZI_LED, HIGH); delay(200);
  }
  delay(1500);

  if(hataSayisi >= MAX_HATA) {
    cezaBaslat();
  } else {
    sifreEkraninaDon();
  }
}

void cezaBaslat() {
  cezaSuresi = TEMEL_CEZA_MS << cezaSeviyesi; // 30 sn, 60 sn, 120 sn ...
  if(cezaSeviyesi < MAX_CEZA_SEVIYESI) cezaSeviyesi++;
  cezaBaslangic = millis();
  hataSayisi = 0;
  sonGosterilenSn = -1;
  durum = CEZA_KILIDI;
}

void cezaModu() {
  keypad.getKey(); // Bu surede basilan tuslar yok sayilir

  unsigned long gecen = millis() - cezaBaslangic;
  if(gecen >= cezaSuresi) {
    sifreEkraninaDon();
    return;
  }

  int kalan = (cezaSuresi - gecen + 999) / 1000;
  if(kalan != sonGosterilenSn) {
    sonGosterilenSn = kalan;
    ekranYaz("KILITLENDI", "Cok fazla hata", "Bekleyin: " + String(kalan) + " sn");
  }

  // Kirmizi LED yavas yanip soner
  digitalWrite(KIRMIZI_LED, (millis() / 500) % 2 == 0 ? HIGH : LOW);
}

// --- ACIK DURUM ---
void acikMod() {
  float mesafe = mesafeOlcer();
  int gecenSn = (millis() - kasaAcilisZamani) / 1000;
  int kalanSure = ACIK_KALMA_SN - gecenSn;

  if(!kapakAcildi) {
    // Kilit acildi ama kapak henuz acilmadi: otomatik kilitlemeyi baslatma
    if(mesafe > ACIK_ESIK_CM) {
      kapakAcildi = true;
      sonGosterilenSn = -1;
    } else if(kalanSure <= 0) {
      // Sure doldu ve kapak hic acilmadi: guvenli tarafta kal, tekrar kilitle
      kasaKapat();
      return;
    }
  } else if(mesafe < KAPALI_ESIK_CM) {
    // Kapak acilip tekrar kapatildi: hemen kilitle
    kasaKapat();
    return;
  }

  if(kalanSure > 0) {
    if(kalanSure != sonGosterilenSn) {
      sonGosterilenSn = kalanSure;
      String yazi = kapakAcildi ? "Kapanma: " : "Kapagi acin: ";
      ekranYaz("KASA ACIK", yazi + String(kalanSure) + " sn", "");
    }
  } else {
    if(sonGosterilenSn != 0) {
      sonGosterilenSn = 0;
      ekranYaz("UYARI!", "SURE DOLDU", "Kapagi kapatin!");
    }
    if((millis() / 300) % 2 == 0) {
      tone(BUZZER, 1000);
    } else {
      noTone(BUZZER);
    }
  }
}

// --- KASA ACMA / KAPAMA ---
void kasaAc() {
  hataSayisi = 0;
  cezaSeviyesi = 0;
  kapakAcildi = false;

  digitalWrite(KIRMIZI_LED, LOW);
  digitalWrite(YESIL_LED, HIGH);

  tone(BUZZER, 1000); delay(100);
  tone(BUZZER, 1500); delay(100);
  noTone(BUZZER);

  ekranYaz("ACILDI!", "Guvenlik Dogrulandi", "");
  servoYavasCek(90);

  durum = ACIK;
  sonGosterilenSn = -1;
  kasaAcilisZamani = millis(); // Geri sayim servo hareketi bittikten sonra baslar
}

void kasaKapat() {
  noTone(BUZZER);
  digitalWrite(KIRMIZI_LED, HIGH);
  digitalWrite(YESIL_LED, LOW);

  servoYavasCek(0);
  sifreEkraninaDon();
}

void sifreEkraninaDon() {
  durum = SIFRE_BEKLE;
  girilenSifre = "";
  sonGosterilenSn = -1;
  digitalWrite(KIRMIZI_LED, HIGH);
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
