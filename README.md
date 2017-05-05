# Arduino controlling a pump to feed a furnace with oil
Hungarian comments

Olajszivattyú vezérlés Cseh Laci barátomnak

__Specifikáció:__
Megadott időnként megadott időre kapcsolja be az olajszivattyút. Mindkét idő állítható legyen, kikapcsolás után is maradjon meg a beállított érték.
 
__Megvalósítás:__
UNO + LCD Keypad Shield (16x2) + 230V-os relé
 
 A kijelző felső sora a beállított értékeket mutatja, kis ikonok jelzik mi micsoda.
 
 A második sor az aktuális fázist és a hátralévő időt.
 
 Az LCD háttérvilágítás egy idő után lekapcsol (LCD lekapcsolás bugos, nem jön vissza),  az adatokat változtatáskor memóriába tartja, majd egy idő után EEPROM-ba tárolja. 
 
 Az adatok mentett állapotát az első ikon mutatja (CHAR_SAVED és CHAR_NEED_SAVE).
 
 ![](https://raw.githubusercontent.com/bunnyhu/oilpump_controll/master/B2S54488.jpg)
 
