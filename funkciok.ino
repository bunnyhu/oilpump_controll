// *********************
// LCD gombok kiolvasása
// *********************
int read_LCD_buttons() {
  int adc_key_in = analogRead(LCD_A0);  // épp lenyomott gomb ID
  int resultId = btnNONE;               // visszaadott érték ID
  
  if (adc_key_in > 1000) { resultId = btnNONE; }
  else if (adc_key_in < 50)   { resultId =  btnRIGHT; } 
  else if (adc_key_in < 195)  { resultId =  btnUP; }
  else if (adc_key_in < 380)  { resultId =  btnDOWN; }
  else if (adc_key_in < 555)  { resultId =  btnLEFT; }
  else if (adc_key_in < 790)  { resultId =  btnSELECT; }
  else { resultId =  btnNONE; };


  if (resultId ==  btnNONE) {    // már nincs lenyomva gomb
    if (idKeyDown != btnNONE) {  // gombot épp felengedte
      resultId = idKeyDown;
    };
    idKeyDown = btnNONE;
    timeKeyDown = 0;
  } else if (resultId ==  btnSELECT) {    // SELECT külön kezelve, az nincs ismétléskezelve
    idKeyDown = btnNONE;
    timeKeyDown = 0;
    return btnSELECT;
  } else {    // van lenyomott gomb
    if (idKeyDown != btnNONE) { // már egy ideje nyomva tartja 
      if ((millis() - timeKeyDown) > KEY_REPEAT_TIME) {  // ismétlési időn túl nyomja
        resultId = idKeyDown;
        timeKeyDown = millis();      
      } else {
        resultId = btnNONE;
      };
    } else {  // most nyomta meg épp
      idKeyDown = resultId;
      timeKeyDown = millis();      
      resultId = btnNONE;
    };
  };
  return resultId;
};

/**
   LCD gombok kiolvasása sima lenyomvatartással
*/ 
int old_Read_LCD_buttons() {
  int adc_key_in = analogRead(LCD_A0);
  adc_key_in = analogRead(LCD_A0);
  if (adc_key_in > 1000) { return btnNONE; }
  else if (adc_key_in < 50)  { return btnRIGHT; } 
  else if (adc_key_in < 195)  { return btnUP; }
  else if (adc_key_in < 380)  { return btnDOWN; }
  else if (adc_key_in < 555)  { return btnLEFT; }
  else if (adc_key_in < 790)  { return btnSELECT; }
  else { return btnNONE; };
};


/**
 * space változóval feltölti hossz-nyira a szam-ot
*/
String fillSpace(unsigned long szam , int hossz, char space) {
  String numberString = String(szam);
  int zeroDarab = hossz - numberString.length();
  if ( zeroDarab > 0) {
    for (int f = 1; f <= zeroDarab; f++) {
      numberString = space + numberString;
    };
  };
  return numberString;
};

/**
 * Változók alapértékkel feltöltése
 */
void resetVariables() {
  Serial.println("Valtozok rezetelodtek.");
  need_relay = false;
  timeLastKeyPressed = 0;
  timeKeyDown = 0;
  secLeft = 0;
  timePeriodEnd = millis() + (secWaitPeriod * 1000);
};

/**
 * EEPROM nullázása
 */
bool hddClear() {
  Serial.println("Nincs EEPROM adat, nullazom az EEPROM-ot.");    
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  };  
};

/**
 * EEPROM betöltése
 */
bool hddLoad() {
  Serial.println("EEPROM adat betoltese.");
  EEPROM.get(EEPROM_START, HDD);
  secWaitPeriod = HDD.wait;
  secPumpWorking = HDD.pump;

  Serial.println(HDD.id);
  Serial.println(HDD.wait);
  Serial.println(HDD.pump);  
};

/**
 * Változók EEPROM mentése
 */
bool hddStore() {
  Serial.println("EEPROM iras.");
  HDD.id[0] = EEPROM_VERSION[0];
  HDD.id[1] = EEPROM_VERSION[1];
  HDD.id[2] = EEPROM_VERSION[2];
  HDD.wait = secWaitPeriod;
  HDD.pump = secPumpWorking;
  EEPROM.put(EEPROM_START, HDD);
  EEPROM.get(EEPROM_START, HDD);      
  if ((HDD.wait == secWaitPeriod) && (HDD.pump == secPumpWorking)) {
    Serial.println("Sikeres EEPROM iras.");
    return true;
  } else {
    Serial.println("HIBA! Nem sikerult az EEPROM iras.");
    return false;
  };
  return false;
}

