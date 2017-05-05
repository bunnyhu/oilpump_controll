/**
 * Olajszivattyú vezérlés Cseh Laci barátomnak
 * 
 * Specifikáció:
 * Megadott időnként megadott időre kapcsolja be az olajszivattyút. Mindkét idő állítható legyen, 
 * kikapcsolás után is maradjon meg a beállított érték.
 * 
 * Megvalósítás:
 * UNO + LCD Keypad Shield (16x2) + 230V-os relé
 * 
 * A kijelző felső sora a beállított értékeket mutatja, kis ikonok jelzik mi micsoda.
 * A második sor az aktuális fázist és a hátralévő időt.
 * Az LCD háttérvilágítás egy idő után (LCD lekapcsolás bugos, nem jön vissza),
 * az adatokat változtatáskor memóriába tartja, majd egy idő után EEPROM-ba tárolja. 
 * Az adatok mentett állapotát az első ikon mutatja (CHAR_SAVED és CHAR_NEED_SAVE).
 * 
 * A későbbiekben bővíteni lehetne hőfok érzékeléssel pl.
 */
 
#include <EEPROM.h>
#include <LiquidCrystal.h>

// LCD Karakterdefiníciók
#define CHAR_NEED_SAVE 0
byte iconLakatNyitott[8] {
  B01110,
  B10000,
  B10000,
  B11111,
  B11111,
  B11011,
  B11111,
};
#define CHAR_SAVED 1
byte iconLakatZart[8] {
  B01110,
  B10001,
  B10001,
  B11111,
  B11111,
  B11011,
  B11111,
};
#define CHAR_RIGHT 2
byte iconArrowRight[8] = {
  B01000,
  B00100,
  B00010,
  B11101,
  B00010,
  B00100,
  B01000,
};
#define CHAR_LEFT 3
byte iconArrowLeft[8] = {
  B00010,
  B00100,
  B01000,
  B10111,
  B01000,
  B00100,
  B00010,
};
#define CHAR_UP_DOWN 4
byte iconArrowUpDown[8] = {
  B00100,
  B01010,
  B10101,
  B00100,
  B10101,
  B01010,
  B00100,
};
#define CHAR_WAIT 5
byte iconClock[8] = {
  B11111,
  B01010,
  B01110,
  B00100,
  B01110,
  B01010,
  B11111,
};
#define CHAR_PUMP 6
byte iconOil[8] = {
  B01000,
  B00100,
  B00110,
  B01110,
  B11111,
  B11111,
  B01110,
};
#define CHAR_LINE 7
byte iconLine[8] = {
  B00000,
  B11111,
  B11111,
  B00000,
  B11111,
  B11111,
  B00000,
};

#define LCD4 4
#define LCD5 5
#define LCD6 6
#define LCD7 7
#define LCD8 8
#define LCD9 9
#define LCD10 10
#define LCD_A0 0

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
#define KEY_REPEAT_TIME 333     // Nyomvatartott gomb mennyi ms-onként adjon lenyomást
#define LCD_AUTO_DARK 30000     // LCD ennyi ms után sötétedik el

#define RELAY_PIN  3            // Relé vezérlő pin
#define EEPROM_SAVE_TIME 300000  // utolsó gomb felengedés után ms EEPROM-ba mentés ha kell

#define EEPROM_VERSION "bny"    // EEPROM objektum azonosító char tömb
#define EEPROM_START 30          // EEPROM változók kezdőbyte

// EEPROM-ban tárolt adatokhoz objektum keret
struct hddObject {
  char id[4];
  unsigned long wait;
  unsigned long pump;
};

int lcd_key = 0;                // loop gomb azonosító
boolean relay_status = false;   // Relé jelenleg nyitott-e
boolean need_relay = false;     // Relé nyitás kell-e
boolean lcd_bright = true;      // LCD háttérvilágítás állapota

unsigned long timeLastKeyPressed = 0;  // Utolsó gombnyomás timestamp
unsigned long timePeriodEnd = 0;   // A periódus vége timestamp
unsigned long timeActual = 0;      // millis() értéke
unsigned long lastTimeActual = 0;  // Előző loop millis() értéke (túlcsordulás ellenőrzéshez kell)
unsigned long timeKeyDown = 0;     // mióta van a gomb lenyomva
int idKeyDown = btnNONE;           // a nyomvatartott gomb ID

unsigned long secWaitPeriod = 30;         // Hány mp-enként nyisson a pumpa
unsigned long secPumpWorking = 3;         // Hány mp-ig dolgozzon a pumpa
unsigned long secLeft = 0;                // Aktuális állapotból ennyi mp van hátra

hddObject HDD { "---", 0, 0 };

LiquidCrystal lcd(LCD8, LCD9, LCD4, LCD5, LCD6, LCD7);

/* 
 ********************************************************************************
                 Inicializálás (SETUP)
 ******************************************************************************** 
 */
void setup() {
  Serial.begin(9600);
  Serial.println("Start");

  lcd.createChar(CHAR_WAIT, iconClock);
  lcd.createChar(CHAR_PUMP, iconOil);
  lcd.createChar(CHAR_LINE, iconLine);
  lcd.createChar(CHAR_LEFT, iconArrowLeft);
  lcd.createChar(CHAR_RIGHT, iconArrowRight);
  lcd.createChar(CHAR_UP_DOWN, iconArrowUpDown);
  lcd.createChar(CHAR_SAVED, iconLakatZart);
  lcd.createChar(CHAR_NEED_SAVE, iconLakatNyitott);

  pinMode(LCD10, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  lcd.begin(16, 2);
  lcd.home();
  lcd.print("                ");
  lcd.print("                ");
  
  if ((EEPROM.read(EEPROM_START+0)==EEPROM_VERSION[0]) &&
      (EEPROM.read(EEPROM_START+1)==EEPROM_VERSION[1]) &&
      (EEPROM.read(EEPROM_START+2)==EEPROM_VERSION[2])) {
    Serial.println("Az EEPROM-ban megtaláltam az azonosító karaktereket.");
    hddLoad();
    if ((secWaitPeriod>999) || (secWaitPeriod<0) || (secPumpWorking>999) || (secPumpWorking<0)) {
      // Hibás adat beolvasás az eepromból
      secWaitPeriod = 30;
      secPumpWorking = 3;
      hddClear();
      hddStore();
    }    
  } else {
    hddClear();
    hddStore();
  };
  
  resetVariables();
}

/**
 ********************************************************************************
                   loop kód
 ********************************************************************************                     
*/
void loop() {
  timeActual = millis();
  if ( lastTimeActual > timeActual ) {  resetVariables(); };  // túlcsordult az idő változó.
  lastTimeActual = timeActual;

  lcd_key = read_LCD_buttons();

  // Megnézi lejárt-e az épp futó periódus.
  if ( timePeriodEnd < timeActual ) {
    if (relay_status) {
      // A relé nyitva volt, úgyhogy most várakozási periódus jön, relé záródjon
      timePeriodEnd = timeActual + (secWaitPeriod * 1000);
      need_relay = false;
    } else {
      // Relé zárva volt, úgyhogy most pumpálunk és nyisson relét      
      timePeriodEnd = timeActual + (secPumpWorking * 1000);
      need_relay = true;
    };
    lcd.setCursor(0,1);
    lcd.print("                ");
  };
  secLeft = int((timePeriodEnd - timeActual) / 1000)+1; // Hány mp van még hátra a periódusból

  // LCD gombjainak kezelése
  if (lcd_key != btnNONE) {
    timeLastKeyPressed = timeActual;
    if (!lcd_bright) {
        // Ha az LCD ki volt kapcsolva, bekapcsoljuk és a gombot nem értelmezzük tovább
        pinMode(LCD10, INPUT);
        lcd_bright = true;
    } else {
      // AZ LCD világított, gombot értelmezünk
      if (lcd_key == btnSELECT) { 
        need_relay = true;
        timePeriodEnd = timeActual;
      } else {
        switch (lcd_key) {
          case btnRIGHT: {
              if (secWaitPeriod<999) { secWaitPeriod ++; };
              break;
            }
          case btnLEFT: {            
              if (secWaitPeriod > 0) { secWaitPeriod --; };
              break;
            }
          case btnUP: {
              if (secPumpWorking < 999) { secPumpWorking ++; };
              break;
            }
          case btnDOWN: {           
              if (secPumpWorking > 0) { secPumpWorking --; };
              break;
            }
        };
      };
    };      

  };  

  // Kijelző adatok megjelenítése 
  lcd.home();

  if ((HDD.wait != secWaitPeriod) || (HDD.pump != secPumpWorking)) {
    lcd.write(byte(CHAR_NEED_SAVE));
  } else {
    lcd.write(byte(CHAR_SAVED));    
  }

  lcd.write(byte(CHAR_WAIT));
  lcd.print(fillSpace(secWaitPeriod,3,' '));
  lcd.write(byte(CHAR_LEFT));
  lcd.write(byte(CHAR_RIGHT));

  lcd.setCursor(11,0);
  lcd.write(byte(CHAR_PUMP));
  lcd.print(fillSpace(secPumpWorking,3,' '));
  lcd.write(byte(CHAR_UP_DOWN));

  if (need_relay) {
    if (!relay_status) {
      digitalWrite(RELAY_PIN, HIGH);
    };
    relay_status = true;
  } else {
    if (relay_status) {
      digitalWrite(RELAY_PIN, LOW);
    };
    relay_status = false;
  };

  // Második sor 
  lcd.setCursor(0,1);
  if (relay_status) {
    lcd.print("Olaj be:");
  } else {
    lcd.print("Szunet:");
  };
  lcd.print(fillSpace(secLeft,3,' '));
  lcd.print( "mp");

  // Ha túl régóta világít, az LCD-t lekapcsolja
  if (lcd_bright && ((timeActual - timeLastKeyPressed) > LCD_AUTO_DARK )) {
        pinMode(LCD10, OUTPUT);
        digitalWrite(LCD10, LOW);
        lcd_bright = false;
  };
  // Ha az EEPROM változó különbözik a RAM változótól, tároljuk ha egy ideje már nem nyomott gombot
  if ((HDD.wait != secWaitPeriod) || (HDD.pump != secPumpWorking)) {
    if ((timeActual - timeLastKeyPressed) > EEPROM_SAVE_TIME ) {
      hddStore();
    };    
  };  
};

