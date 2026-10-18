const unsigned int tlacitko = A7;  // Tlacitko pripojeno na pin A7
const unsigned int bzucak = 8;  // Bzucak pripojen na pin D8
bool stav_tlacitka = 0;  // Promenna, kam se bude ukladat aktualni stav tlacitka

void setup() {
  Serial.begin(115200);
  pinMode(bzucak, OUTPUT);  // Nastaveni pinu, na ktery je pripojen bzucak, jako vystupni
  // Tlacitko je na pinu A7, ktery je na Arduino NANO jen analogovy vstup - pinMode() se na nej nesmi volat
  // (jadro Arduina nekontroluje cislo pinu a nahodne by prepsalo nastaveni jinych pinu). analogRead() ho nepotrebuje.
}

void loop() {
  stav_tlacitka = !(analogRead(tlacitko) >= 512);  // Nacteni analogove hodnoty na pinu a pokud je hodnota vetsi, nebo rovna 512 vrati hodnotu 1. Jinak vraci hodnotu 0. Vysledek je negovan, takze hodnota zapsana do promenne je: 1 = tlacitko sepnuto, 0 = tlacitko rozepnuto
  Serial.println(stav_tlacitka);
  if (stav_tlacitka) {  // Pokud je tlacitko stisknuto
    tone(bzucak, 1000);  // Spusti bzucak s frekvenci 1kHz
  } else {
    noTone(bzucak);  // Vypne bzucak
  }
  delay(250);
}