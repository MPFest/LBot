//LBot - vylepseny ridici program v2.0 (MPFest edition)
//Autor uprav: Martin Petrik (MPFest) - https://github.com/MPFest/LBot
//Zalozeno na: LaskaKIT LBot factory default software v1.7 (https://github.com/LaskaKit/LBot)
//
//Vyzaduje knihovny: IRremote 4.x, Adafruit NeoPixel, HCSR04 (Martin Sosic)

// IR prijimac pouzije casovac Timer1, protoze Timer2 potrebuje bzucak (funkce tone).
// Musi byt nadefinovano PRED #include <IRremote.hpp>. Knihovnu uz neni potreba upravovat.
#define IR_USE_AVR_TIMER1
#define DECODE_NEC          // Dekodovat jen protokol NEC (ovladac Car MP3) - mensi program
#include <IRremote.hpp>     // IRremote 4.x
#include <Adafruit_NeoPixel.h>
#include <HCSR04.h>

// =====================================================================
// KONFIGURACE - vsechno, co se da nastavit nebo doladit pro konkretni robot
// =====================================================================

// --- Piny ---
const unsigned int cidlo_L = 9;      // Leve cidlo sledovani cary
const unsigned int cidlo_R = 10;     // Prave cidlo sledovani cary
const unsigned int triggerPin = 11;  // Trigger ultrazvukoveho cidla
const unsigned int echoPin = 12;     // Echo ultrazvukoveho cidla
const unsigned int bzucak = 8;       // Bzucak
const unsigned int tlacitko = A7;    // Tlacitko na zakladni desce
const int svetelny_senzor = A6;      // Cidlo okolniho osvetleni
const unsigned int ir_prijimac = 2;  // IR prijimac
const unsigned int rgb_pin = 13;     // Data pro RGB LED WS2812
const byte LED_LEVA = 1;             // Poradi RGB LED v retezu: prvni (0) je vpravo ve smeru jizdy,
const byte LED_PRAVA = 0;            // druha (1) vlevo
const unsigned int pwmLeft = 6;      // PWM levy motor
const unsigned int left = 7;         // Smer levy motor
const unsigned int pwmRight = 5;     // PWM pravy motor
const unsigned int right = 4;        // Smer pravy motor

// --- Prepinace: veci, ktere se mohou u ruznych kusu lisit ---
// Smer otaceni motoru. Pokud se kolo pri jizde dopredu toci dozadu, prepni na true.
const bool MOTOR_L_OBRACENY = false;
const bool MOTOR_R_OBRACENY = false;

// Logika cidel cary. Zjistis ji podle LED na cidle:
//   true  = LED sviti nad svetlou podlahou a nad carou zhasne
//   false = LED sviti nad carou a nad svetlou podlahou zhasne
const bool CIDLO_CARY_SVETLO_JE_HIGH = true;

// Zpusob sledovani cary:
//   true  = robot jede PO care, obe cidla nad carou (siroka cara, sirsi nez rozestup cidel)
//   false = robot jede rozkrocmo, cara je MEZI cidly (tenka cara, uzsi nez rozestup cidel)
const bool JIZDA_PO_CARE = true;

// --- Kalibrace motoru ---
// Kdyz robot pri jizde rovne zataci, pridej par procent pomalejsimu motoru.
// Tahne doprava = pridej pravemu (KOREKCE_R), tahne doleva = levemu (KOREKCE_L).
const unsigned int KOREKCE_L = 0;    // % navic pro levy motor
const unsigned int KOREKCE_R = 0;    // % navic pro pravy motor

// --- Rychlosti (0 - 255) ---
// Skutecna rychlost zavisi i na napeti baterie: nastaveno pro 6x AA NiMH (~7,2 V).
// Se slabsi baterii (napr. 4x AA) jede robot pomaleji - pak hodnoty zvys zhruba o polovinu.
const unsigned int RYCHLOST_JIZDA = 100;   // Jizda na ovladac
const unsigned int RYCHLOST_COUVANI = 45;  // Couvani od prekazky
const unsigned int ZATACENI_PROCENT = 50;  // Ostre zataceni: vnitrni kolo jede POZPATKU na x % rychlosti

// --- Sledovani cary ---
const unsigned int CARA_RYCHLOST_ROVNE = 55;    // Kdyz robot jede rovne po care
const unsigned int CARA_RYCHLOST_KOREKCE = 45;  // Kdyz se vraci na caru
// Kdyz cidlo sjede z cary, robot nejdriv zataci jemne (vnitrni kolo jede dopredu pomaleji)
// a teprve kdyz se cara do CARA_PRITVRDIT_MS nevrati, zatoci ostre (vnitrni kolo pozpatku).
const unsigned int CARA_JEMNE_PROCENT = 30;     // Jemne zataceni: vnitrni kolo dopredu na x % rychlosti
const unsigned long CARA_PRITVRDIT_MS = 300;
// Kdyz robot caru ztrati (obe cidla mimo), tuto dobu ji hleda na strane, kde byla naposledy.
// Kdyz ji nenajde, zastavi (konec cary = cil).
const unsigned long CARA_HLEDANI_MS = 1000;

// --- Prekazky ---
const int PREKAZKA_STOP_CM = 10;       // Bliz nez tohle = zastav
const int PREKAZKA_VOLNO_CM = 13;      // Prekazka je pryc, az kdyz je dal nez tohle
                                       // (na ovladac robot couvne sem, na care tady pocka)
const unsigned int ULTRAZVUK_MAX_CM = 100;     // Dal se nemeri (kratsi mereni = rychlejsi reakce)
const unsigned long MERENI_INTERVAL_MS = 60;   // Jak casto merit vzdalenost
const unsigned long MERENI_MAX_ODKLAD_MS = 200; // Nejdele o kolik se mereni odlozi kvuli prichazejicimu IR signalu
const unsigned long COUVANI_MAX_MS = 2000;     // Pojistka: dele nez tohle nikdy necouvej

// --- Svetla ---
const int SVETLO_PRAH = 500;           // Pod touto hodnotou cidla osvetleni je tma
const int SVETLO_HYSTEREZE = 50;       // Rozsviti se pod PRAH - HYSTEREZE, zhasne nad PRAH + HYSTEREZE
                                       // (aby svetla v seru neprebliknavala sem a tam)
const unsigned long SVETLO_INTERVAL_MS = 200;  // Jak casto kontrolovat okolni svetlo
// Kdy svitit a jaky efekt po zapnuti robota. Za jizdy jde oboji zmenit z weboveho ovladace
// nebo IR tlacitky (1 az 5 = efekt, 7 / 8 / 9 = kdy svitit).
//   SVETLA_POTME = rozsviti se samo potme, SVETLA_VZDY = sviti porad, SVETLA_NIKDY = nesviti
//   EFEKT_POLICIE   = leva LED 2x blikne cervene, pak prava 2x modre
//   EFEKT_BILA      = sviti bile
//   EFEKT_BLINKRY   = jako auto: rovne bila, pri zataceni blika oranzove strana zatacky,
//                     stoji cervena, pri couvani od prekazky blikaji obe oranzove
//   EFEKT_PARKOVACI = podle prekazky: volno zelena, bliz blika zlute a oranzove
//                     (cim bliz, tim rychleji), u STOP cervena
//   EFEKT_DUHA      = barvy se pomalu prelevaji
enum KdySvitit { SVETLA_POTME, SVETLA_VZDY, SVETLA_NIKDY };
enum Efekt { EFEKT_POLICIE, EFEKT_BILA, EFEKT_BLINKRY, EFEKT_PARKOVACI, EFEKT_DUHA };
const KdySvitit SVETLA_KDY = SVETLA_POTME;
const Efekt SVETLA_EFEKT = EFEKT_POLICIE;
const unsigned long POLICIE_KROK_MS = 80;    // Rychlost policejniho blikani (mensi = rychlejsi)
const unsigned long BLINKR_KROK_MS = 350;    // Rychlost blinkru
const unsigned long DUHA_KROK_MS = 80;       // Jak casto se posune barva duhy
const unsigned int PARKOVACI_DALEKO_CM = 50; // Dal nez tohle sviti parkovaci svetla zelene
const unsigned int PARKOVACI_BLIZKO_CM = 25; // Bliz nez tohle blikaji oranzove (jinak zlute)
const byte JAS_LED = 10;               // Jas RGB LED (0 - 255)
// Zapis do LED na chvilku vypne preruseni a rusi prijem IR. Proto se LED prepisuji jen pri zmene
// a nejvys jednou za tuto dobu.
const unsigned long LED_MIN_ODSTUP_MS = 80;

// --- Ostatni ---
const int TLACITKO_PRAH = 512;         // Hodnota analogRead, pod kterou je tlacitko stisknute
const unsigned int PIP_FREKVENCE = 1000;  // Hz
const unsigned int PIP_DELKA = 200;       // ms
// Varovani pred prekazkou: nekolik rychlych vysokych pipnuti (jinak nez bezne pipnuti)
const unsigned int VAROVANI_FREKVENCE = 2500;    // Hz
const unsigned int VAROVANI_DELKA = 60;          // ms, jedno pipnuti
const unsigned long VAROVANI_ROZESTUP_MS = 120;  // ms, od zacatku jednoho pipnuti k dalsimu
const byte VAROVANI_POCET = 3;                   // Kolikrat pipne
const unsigned long PREPNUTI_PAUZA_MS = 500;  // Ochrana proti dvojitemu prepnuti rezimu

// --- IR dalkovy ovladac (Car MP3, protokol NEC) ---
// Cisla tlacitek zjistis prikladem IR_prijimac (Lekce 6), vypisuje je jako "Command".
// Ovladaci kriz je kolem tlacitka "+".
const uint8_t IR_DOPREDU = 0x40;   // Tlacitko >>|
const uint8_t IR_DOZADU = 0x19;    // Tlacitko 100+
const uint8_t IR_DOLEVA = 0x07;    // Tlacitko -
const uint8_t IR_DOPRAVA = 0x09;   // Tlacitko EQ
const uint8_t IR_CARA = 0x16;      // Tlacitko 0 = zapnuti/vypnuti sledovani cary
// Svetla: tlacitka 1 az 5 = efekt, 7 / 8 / 9 = kdy svitit
const uint8_t IR_EFEKT_POLICIE = 0x0C;    // Tlacitko 1
const uint8_t IR_EFEKT_BILA = 0x18;       // Tlacitko 2
const uint8_t IR_EFEKT_BLINKRY = 0x5E;    // Tlacitko 3
const uint8_t IR_EFEKT_PARKOVACI = 0x08;  // Tlacitko 4
const uint8_t IR_EFEKT_DUHA = 0x1C;       // Tlacitko 5
const uint8_t IR_SVETLA_POTME = 0x42;     // Tlacitko 7
const uint8_t IR_SVETLA_VZDY = 0x52;      // Tlacitko 8
const uint8_t IR_SVETLA_NIKDY = 0x4A;     // Tlacitko 9
// Pri drzeni tlacitka posila ovladac opakovaci kod asi kazdych 110 ms.
// Kdyz neprijde nic dele nez tuto dobu, robot zastavi (pusteni tlacitka).
const unsigned long IR_TIMEOUT_MS = 250;

// --- Bluetooth povely ---
const char BT_DOPREDU = 'u';
const char BT_DOZADU = 'd';
const char BT_DOLEVA = 'l';
const char BT_DOPRAVA = 'r';
const char BT_STOP = 'o';
const char BT_CARA = '2';   // Zapnuti/vypnuti sledovani cary
const char BT_SVETLA_POTME = 'a';     // Svetla: automaticky potme
const char BT_SVETLA_VZDY = 's';      // Svetla: svitit vzdy
const char BT_SVETLA_NIKDY = 'n';     // Svetla: nesvitit
const char BT_EFEKT_POLICIE = 'p';
const char BT_EFEKT_BILA = 'b';
const char BT_EFEKT_BLINKRY = 'k';
const char BT_EFEKT_PARKOVACI = 'g';
const char BT_EFEKT_DUHA = 'h';
// Kdyz z aplikace neprijde povel dele nez tuto dobu, robot zastavi (pusteni tlacitka).
// Aplikace musi pri drzeni tlacitka posilat povel opakovane. Kdyz robot pri drzeni cuka, zvys.
const unsigned long BT_TIMEOUT_MS = 500;

// --- Data pro webovy ovladac (slozka ovladac/ v repu) ---
// Ovladac si o data pro sonar rika znakem BT_TELEMETRIE. Robot je pak posila po kazdem mereni
// vzdalenosti, dokud ovladac zada - bez ovladace nic navic neposila (Serial Monitor zustane cisty).
const char BT_TELEMETRIE = 'v';
const unsigned long TELEMETRIE_DOBA_MS = 1500;  // Kdyz 'v' neprijde dele nez tohle, robot prestane

// =====================================================================
// PROMENNE
// =====================================================================

// Co ma robot delat podle ovladace (IR nebo Bluetooth)
enum Povel { STOP, DOPREDU, DOZADU, DOLEVA, DOPRAVA };
// V jakem rezimu robot je
enum Rezim { RUCNI, CARA, COUVANI };

Rezim rezim = RUCNI;
Povel povel = STOP;              // Posledni povel z ovladace
unsigned long povelCas = 0;      // Kdy povel prisel (ms)
unsigned long povelPlatnost = 0; // Jak dlouho povel plati (ms)
unsigned long couvaniStart = 0;  // Kdy zacalo couvani (ms)
float vzdalenost = -1;           // Posledni zmerena vzdalenost v cm (-1 = nic v dosahu)
int rychlost = 100;              // Aktualni rychlost jizdy (0 - 255)
byte varovaniZbyva = 0;          // Kolik varovnych pipnuti jeste zahrat
unsigned long varovaniCas = 0;   // Kdy zaznelo posledni varovne pipnuti (ms)
bool telemetrieBezi = false;     // Posila robot data webovemu ovladaci?
unsigned long telemetrieCas = 0; // Kdy ovladac naposledy pozadal o data (ms)
KdySvitit svetlaKdy = SVETLA_KDY; // Kdy svitit (jde menit za jizdy)
Efekt svetlaEfekt = SVETLA_EFEKT; // Jaky svetelny efekt (jde menit za jizdy)
int osvetleni = 1023;            // Posledni hodnota cidla okolniho svetla (mensi = tmavsi)
bool tma = false;                // Je tma? (s hysterezi)
int motorL = 0;                  // Posledni rychlost leveho motoru (-255 az 255), pro blinkry
int motorP = 0;                  // Posledni rychlost praveho motoru

Adafruit_NeoPixel rgbWS = Adafruit_NeoPixel(2, rgb_pin, NEO_GRB + NEO_KHZ800);
UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin, ULTRAZVUK_MAX_CM);

// =====================================================================
// SETUP A HLAVNI SMYCKA
// =====================================================================

void setup() {
  Serial.begin(9600);  // Seriova linka = USB i Bluetooth modul
  pinMode(bzucak, OUTPUT);
  // Tlacitko (A7) a cidlo svetla (A6) jsou na Arduino NANO jen analogove vstupy - pinMode() se na ne
  // NESMI volat: jadro Arduina nekontroluje cislo pinu, cetlo by za koncem tabulky pinu a nahodne
  // prepsalo nastaveni jinych pinu (napr. spoust ultrazvuku D11). analogRead() zadny pinMode nepotrebuje.
  pinMode(pwmLeft, OUTPUT);
  pinMode(left, OUTPUT);
  pinMode(pwmRight, OUTPUT);
  pinMode(right, OUTPUT);

  // Blikani LED pri prijmu IR vypnuto - pin 13 je na teto desce datovy pin RGB LED
  IrReceiver.begin(ir_prijimac, DISABLE_LED_FEEDBACK);
  rgbWS.begin();
  rgbWS.setBrightness(JAS_LED);

  motoryStop();
  Serial.println("Pripraven...");

  nastavLed(0, 255, 0);  // Zelene bliknuti a pipnuti = robot je pripraven
  pip();
  delay(PIP_DELKA);
  nastavLed(0, 0, 0);
}

// Smycka nikde neceka (zadne delay), takze robot reaguje okamzite.
// Kazda funkce se jen podiva, jestli ma neco udelat, a hned se vrati.
void loop() {
  ctiTlacitko();
  ctiIR();
  ctiBluetooth();
  merVzdalenost();
  hlidejSvetlo();
  hlidejZvuk();

  switch (rezim) {
    case RUCNI:   rezimRucni();   break;
    case COUVANI: rezimCouvani(); break;
    case CARA:    rezimCara();    break;
  }
}

// =====================================================================
// REZIMY (chovani robota)
// =====================================================================

// Jizda podle ovladace. Povel plati jen chvili - kdyz neprijde dalsi, robot zastavi.
void rezimRucni() {
  bool povelPlati = (millis() - povelCas <= povelPlatnost);
  Povel p = povelPlati ? povel : STOP;

  if (p == DOPREDU && prekazkaBlizko()) {  // Pred robotem je prekazka
    motoryStop();
    varovani();
    rezim = COUVANI;
    couvaniStart = millis();
    return;
  }

  rychlost = RYCHLOST_JIZDA;
  switch (p) {
    case DOPREDU: jedDopredu();  break;
    case DOZADU:  jedDozadu();   break;
    case DOLEVA:  otocDoleva();  break;
    case DOPRAVA: otocDoprava(); break;
    case STOP:    motoryStop();  break;
  }
}

// Couvani od prekazky, dokud neni dost daleko (nebo neuplyne pojistny cas)
void rezimCouvani() {
  if (prekazkaPryc() || millis() - couvaniStart > COUVANI_MAX_MS) {
    motoryStop();
    rezim = RUCNI;
    return;
  }
  if (millis() - couvaniStart < 50) {  // Chvilku stat, at se motory neprepinaji z plne jizdy rovnou dozadu
    motoryStop();
    return;
  }
  rychlost = RYCHLOST_COUVANI;
  jedDozadu();
}

// Sledovani cary
void rezimCara() {
  // Prekazka na trati: zastav, varuj a pockej, az zmizi
  static bool cekamNaVolno = false;
  if (!cekamNaVolno && prekazkaBlizko()) {
    cekamNaVolno = true;
    varovani();
  }
  if (cekamNaVolno) {
    if (prekazkaPryc()) {
      cekamNaVolno = false;
    } else {
      motoryStop();
      return;
    }
  }

  bool naCare_L = cidloNaCare(cidlo_L);
  bool naCare_R = cidloNaCare(cidlo_R);

  // Jak dlouho uz cidla vidi porad to same (podle toho se zataci jemne, nebo ostre)
  static byte predtim = 255;           // Predchozi kombinace cidel (255 = jeste nevime)
  static unsigned long zmenaCas = 0;   // Kdy se kombinace naposledy zmenila
  byte ted = naCare_L * 2 + naCare_R;
  if (ted != predtim) {
    predtim = ted;
    zmenaCas = millis();
  }
  unsigned long trva = millis() - zmenaCas;

  static int strana = 0;  // Kde byla cara naposledy: -1 = vlevo, 1 = vpravo, 0 = nevime

  // Jedno cidlo na care, druhe mimo -> vratit se na caru
  if (naCare_L != naCare_R) {
    strana = naCare_R ? 1 : -1;
    rychlost = CARA_RYCHLOST_KOREKCE;
    if (trva < CARA_PRITVRDIT_MS) zatocJemne(strana);
    else zatocOstre(strana);
    return;
  }

  // Obe cidla stejne:
  //   po care:   obe na care = rovne, obe mimo = ztratil caru
  //   rozkrocmo: obe mimo = rovne, obe na care = prekrizena cara / cil
  bool rovne = JIZDA_PO_CARE ? naCare_L : !naCare_L;
  if (rovne) {
    rychlost = CARA_RYCHLOST_ROVNE;
    jedDopredu();
  } else if (JIZDA_PO_CARE && strana != 0 && trva < CARA_HLEDANI_MS) {
    rychlost = CARA_RYCHLOST_KOREKCE;  // Ztratil caru (treba v ostre zatacce) - hledat ji
    zatocOstre(strana);
  } else {
    motoryStop();  // Konec cary = cil
  }
}

// Zapne nebo vypne sledovani cary (tlacitko, IR "0" nebo Bluetooth "2")
void prepniCaru() {
  static unsigned long posledniPrepnuti = 0;
  if (millis() - posledniPrepnuti < PREPNUTI_PAUZA_MS) return;  // Uz se prepinalo pred chvili
  posledniPrepnuti = millis();

  if (rezim == CARA) {
    rezim = RUCNI;
    Serial.println("Sledovani cary vypnuto");
  } else {
    rezim = CARA;
    Serial.println("Sledovani cary zapnuto");
  }
  nastavPovel(STOP, 0);
  motoryStop();
  pip();
}

// =====================================================================
// VSTUPY (ovladace a cidla)
// =====================================================================

// Ulozi povel z ovladace a jak dlouho plati
void nastavPovel(Povel p, unsigned long platnost) {
  povel = p;
  povelCas = millis();
  povelPlatnost = platnost;
}

// Tlacitko na desce - reaguje jen na okamzik stisknuti, ne na drzeni
void ctiTlacitko() {
  static bool predtim = false;
  bool stisknuto = (analogRead(tlacitko) < TLACITKO_PRAH);
  if (stisknuto && !predtim) prepniCaru();
  predtim = stisknuto;
}

// IR dalkovy ovladac
void ctiIR() {
  if (!IrReceiver.decode()) return;  // Nic neprislo
  IRData kod = IrReceiver.decodedIRData;
  IrReceiver.resume();  // Pripravit na dalsi kod

  // Ruseni nebo spatne precteny kod (neni to NEC) - ignorovat, posledni povel dal plati
  if (kod.protocol != NEC) return;

  // Pri drzeni tlacitka chodi opakovaci kody se stejnym cislem tlacitka
  bool opakovani = (kod.flags & IRDATA_FLAGS_IS_REPEAT);

  switch (kod.command) {
    case IR_DOPREDU: nastavPovel(DOPREDU, IR_TIMEOUT_MS); break;
    case IR_DOZADU:  nastavPovel(DOZADU, IR_TIMEOUT_MS);  break;
    case IR_DOLEVA:  nastavPovel(DOLEVA, IR_TIMEOUT_MS);  break;
    case IR_DOPRAVA: nastavPovel(DOPRAVA, IR_TIMEOUT_MS); break;
    case IR_CARA:
      if (!opakovani) prepniCaru();  // Drzeni tlacitka neprepina porad dokola
      break;
    case IR_EFEKT_POLICIE:   if (!opakovani) nastavEfekt(EFEKT_POLICIE);   break;
    case IR_EFEKT_BILA:      if (!opakovani) nastavEfekt(EFEKT_BILA);      break;
    case IR_EFEKT_BLINKRY:   if (!opakovani) nastavEfekt(EFEKT_BLINKRY);   break;
    case IR_EFEKT_PARKOVACI: if (!opakovani) nastavEfekt(EFEKT_PARKOVACI); break;
    case IR_EFEKT_DUHA:      if (!opakovani) nastavEfekt(EFEKT_DUHA);      break;
    case IR_SVETLA_POTME:    if (!opakovani) nastavSvetla(SVETLA_POTME);   break;
    case IR_SVETLA_VZDY:     if (!opakovani) nastavSvetla(SVETLA_VZDY);    break;
    case IR_SVETLA_NIKDY:    if (!opakovani) nastavSvetla(SVETLA_NIKDY);   break;
    default:
      if (!opakovani) {
        Serial.print("Neznamy IR kod: 0x");
        Serial.println(kod.command, HEX);
      }
      break;
  }
}

// Bluetooth - precte vsechny znaky, ktere mezitim prisly
void ctiBluetooth() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    switch (c) {
      case BT_DOPREDU: nastavPovel(DOPREDU, BT_TIMEOUT_MS); break;
      case BT_DOZADU:  nastavPovel(DOZADU, BT_TIMEOUT_MS);  break;
      case BT_DOLEVA:  nastavPovel(DOLEVA, BT_TIMEOUT_MS);  break;
      case BT_DOPRAVA: nastavPovel(DOPRAVA, BT_TIMEOUT_MS); break;
      case BT_STOP:    nastavPovel(STOP, 0);                break;
      case BT_CARA:    prepniCaru();                        break;
      case BT_TELEMETRIE: zapniTelemetrii();                break;
      case BT_SVETLA_POTME:    nastavSvetla(SVETLA_POTME);   break;
      case BT_SVETLA_VZDY:     nastavSvetla(SVETLA_VZDY);    break;
      case BT_SVETLA_NIKDY:    nastavSvetla(SVETLA_NIKDY);   break;
      case BT_EFEKT_POLICIE:   nastavEfekt(EFEKT_POLICIE);   break;
      case BT_EFEKT_BILA:      nastavEfekt(EFEKT_BILA);      break;
      case BT_EFEKT_BLINKRY:   nastavEfekt(EFEKT_BLINKRY);   break;
      case BT_EFEKT_PARKOVACI: nastavEfekt(EFEKT_PARKOVACI); break;
      case BT_EFEKT_DUHA:      nastavEfekt(EFEKT_DUHA);      break;
      default: break;  // Ostatni znaky (napr. konec radku) ignorovat
    }
  }
}

// Ultrazvuk - meri jen kazdych MERENI_INTERVAL_MS (ve vsech rezimech)
void merVzdalenost() {
  static unsigned long posledniMereni = 0;
  if (millis() - posledniMereni < MERENI_INTERVAL_MS) return;
  // Kdyz zrovna prichazi IR signal, mereni chvilku odlozime (at ho neprerusime)
  if (!IrReceiver.isIdle() && millis() - posledniMereni < MERENI_MAX_ODKLAD_MS) return;
  posledniMereni = millis();

  // Prijem IR bezi v preruseni kazdych 50 us. Mereni ozveny (pulseIn) pocita cas
  // smyckou a preruseni by mu cast casu "ukradla" - proto IR na dobu mereni pozastavime.
  IrReceiver.stopTimer();
  vzdalenost = distanceSensor.measureDistanceCm();  // -1 = nic v dosahu
  IrReceiver.restartTimer();

  // Hned PO mereni: odeslani trva asi 12 ms a skonci davno pred dalsim merenim
  posliTelemetrii();
}

// Je pred robotem prekazka tak blizko, ze musi zastavit?
bool prekazkaBlizko() {
  return (vzdalenost >= 0 && vzdalenost <= PREKAZKA_STOP_CM);
}

// Je prekazka uz dost daleko (nebo pryc), aby se mohlo jet dal?
bool prekazkaPryc() {
  return (vzdalenost < 0 || vzdalenost >= PREKAZKA_VOLNO_CM);
}

// Vrati true, pokud je cidlo nad carou (podle prepinace CIDLO_CARY_SVETLO_JE_HIGH)
bool cidloNaCare(unsigned int pin) {
  bool svetlo = (digitalRead(pin) == HIGH) == CIDLO_CARY_SVETLO_JE_HIGH;
  return !svetlo;
}

// =====================================================================
// DATA PRO WEBOVY OVLADAC (sonar)
// =====================================================================

// Ovladac pozadal o data. Na zacatku mu robot posle i sve nastaveni vzdalenosti,
// aby sonar ukazoval zony STOP a volno presne podle KONFIGURACE: "K:stop,volno,max"
void zapniTelemetrii() {
  bool zacatek = !telemetrieBezi;
  telemetrieBezi = true;
  telemetrieCas = millis();
  if (!zacatek) return;

  // "K:stop,volno,max,prah,hystereze" - zony sonaru a kdy je tma
  Serial.print("K:");
  Serial.print(PREKAZKA_STOP_CM);
  Serial.print(',');
  Serial.print(PREKAZKA_VOLNO_CM);
  Serial.print(',');
  Serial.print(ULTRAZVUK_MAX_CM);
  Serial.print(',');
  Serial.print(SVETLO_PRAH);
  Serial.print(',');
  Serial.println(SVETLO_HYSTEREZE);
  posliStavSvetel();
}

// Posle ovladaci nastaveni svetel: "S:kdy,efekt" se stejnymi pismeny jako povely, napr. "S:a,p"
void posliStavSvetel() {
  if (!telemetrieBezi) return;
  char kdy = BT_SVETLA_POTME;
  if (svetlaKdy == SVETLA_VZDY) kdy = BT_SVETLA_VZDY;
  if (svetlaKdy == SVETLA_NIKDY) kdy = BT_SVETLA_NIKDY;
  char efekt = BT_EFEKT_POLICIE;
  if (svetlaEfekt == EFEKT_BILA) efekt = BT_EFEKT_BILA;
  if (svetlaEfekt == EFEKT_BLINKRY) efekt = BT_EFEKT_BLINKRY;
  if (svetlaEfekt == EFEKT_PARKOVACI) efekt = BT_EFEKT_PARKOVACI;
  if (svetlaEfekt == EFEKT_DUHA) efekt = BT_EFEKT_DUHA;
  Serial.print("S:");
  Serial.print(kdy);
  Serial.print(',');
  Serial.println(efekt);
}

// Posle data pro sonar: "T:vzdalenost,rezim,cidla,svetlo,tma", napr. "T:23,R,01,512,0"
//   vzdalenost v cm (-1 = nic v dosahu)
//   rezim R = rucni jizda, C = sledovani cary, Z = couvani od prekazky
//   cidla cary levy a pravy: 1 = vidi caru, 0 = nevidi
//   svetlo = hodnota cidla okolniho svetla (0 - 1023), tma: 1 = je tma
void posliTelemetrii() {
  if (!telemetrieBezi) return;
  if (millis() - telemetrieCas > TELEMETRIE_DOBA_MS) {  // Ovladac se odmlcel (odpojil se)
    telemetrieBezi = false;
    return;
  }
  int cm = (vzdalenost < 0) ? -1 : (int)(vzdalenost + 0.5);  // Zaokrouhlit na cele cm
  char rezimZnak = 'R';
  if (rezim == CARA) rezimZnak = 'C';
  if (rezim == COUVANI) rezimZnak = 'Z';

  Serial.print("T:");
  Serial.print(cm);
  Serial.print(',');
  Serial.print(rezimZnak);
  Serial.print(',');
  Serial.print(cidloNaCare(cidlo_L) ? '1' : '0');
  Serial.print(cidloNaCare(cidlo_R) ? '1' : '0');
  Serial.print(',');
  Serial.print(osvetleni);
  Serial.print(',');
  Serial.println(tma ? '1' : '0');
}

// =====================================================================
// SVETLA A ZVUK
// =====================================================================

// Barvy LED (0xRRGGBB)
const uint32_t BARVA_CERVENA = 0xFF0000;
const uint32_t BARVA_MODRA = 0x0000FF;
const uint32_t BARVA_BILA = 0xFFFFFF;
const uint32_t BARVA_ORANZOVA = 0xFF5000;
const uint32_t BARVA_ZLUTA = 0xFFB400;
const uint32_t BARVA_ZELENA = 0x00FF00;

// Svetla podle nastaveni: kdy svitit (potme / vzdy / nikdy) a jaky efekt
void hlidejSvetlo() {
  // 1) Je tma? Kontrola jen obcas a s hysterezi, aby svetla v seru neprebliknavala
  static unsigned long posledniKontrola = 0;
  if (millis() - posledniKontrola >= SVETLO_INTERVAL_MS) {
    posledniKontrola = millis();
    osvetleni = analogRead(svetelny_senzor);
    if (tma && osvetleni > SVETLO_PRAH + SVETLO_HYSTEREZE) tma = false;
    if (!tma && osvetleni < SVETLO_PRAH - SVETLO_HYSTEREZE) tma = true;
  }

  // 2) Barvy obou LED podle efektu (0 = zhasnuto)
  uint32_t leva = 0;
  uint32_t prava = 0;
  bool sviti = (svetlaKdy == SVETLA_VZDY) || (svetlaKdy == SVETLA_POTME && tma);
  if (sviti) {
    switch (svetlaEfekt) {
      case EFEKT_POLICIE:   efektPolicie(leva, prava);       break;
      case EFEKT_BILA:      leva = prava = BARVA_BILA;       break;
      case EFEKT_BLINKRY:   efektBlinkry(leva, prava);       break;
      case EFEKT_PARKOVACI: efektParkovaci(leva, prava);     break;
      case EFEKT_DUHA:      efektDuha(leva, prava);          break;
    }
  }

  // 3) LED prepsat jen pri zmene a ne moc casto (zapis do LED na chvilku rusi prijem IR)
  static uint32_t zapsanoLeva = 0xFFFFFFFF;  // 0xFFFFFFFF = jeste nic nezapsano
  static uint32_t zapsanoPrava = 0xFFFFFFFF;
  static unsigned long posledniZapis = 0;
  if (leva == zapsanoLeva && prava == zapsanoPrava) return;
  if (millis() - posledniZapis < LED_MIN_ODSTUP_MS) return;
  posledniZapis = millis();
  zapsanoLeva = leva;
  zapsanoPrava = prava;
  rgbWS.setPixelColor(LED_LEVA, leva);
  rgbWS.setPixelColor(LED_PRAVA, prava);
  rgbWS.show();
}

// Policie: leva LED 2x blikne cervene, pak prava 2x modre (8 kroku, liche = zhasnuto)
void efektPolicie(uint32_t &leva, uint32_t &prava) {
  byte krok = (millis() / POLICIE_KROK_MS) % 8;
  if (krok % 2 == 1) return;
  if (krok < 4) leva = BARVA_CERVENA;
  else prava = BARVA_MODRA;
}

// Blinkry jako auto - podle toho, co zrovna delaji motory
void efektBlinkry(uint32_t &leva, uint32_t &prava) {
  // Kam robot jede: 0 = stoji, 1 = rovne, 2 = doleva, 3 = doprava, 4 = couva od prekazky
  byte smer = 1;
  if (rezim == COUVANI) smer = 4;
  else if (motorL == 0 && motorP == 0) smer = 0;
  else if (motorL < motorP) smer = 2;
  else if (motorL > motorP) smer = 3;

  // Blikani zacina rozsvicenim, kdykoli se smer zmeni (jako u auta)
  static byte predtim = 255;
  static unsigned long zacatek = 0;
  if (smer != predtim) {
    predtim = smer;
    zacatek = millis();
  }
  bool blik = ((millis() - zacatek) / BLINKR_KROK_MS) % 2 == 0;
  uint32_t oranzova = blik ? BARVA_ORANZOVA : 0;

  switch (smer) {
    case 0: leva = prava = BARVA_CERVENA;           break;  // Stoji: jako zadni svetla
    case 2: leva = oranzova; prava = BARVA_BILA;    break;  // Zataci doleva
    case 3: leva = BARVA_BILA; prava = oranzova;    break;  // Zataci doprava
    case 4: leva = prava = oranzova;                break;  // Vystrazna svetla
    default: leva = prava = BARVA_BILA;             break;  // Rovne (dopredu i dozadu)
  }
}

// Parkovaci svetla: barva podle prekazky, cim bliz, tim rychleji blikaji
void efektParkovaci(uint32_t &leva, uint32_t &prava) {
  uint32_t barva;
  if (vzdalenost < 0 || vzdalenost > PARKOVACI_DALEKO_CM) {
    barva = BARVA_ZELENA;
  } else if (vzdalenost <= PREKAZKA_STOP_CM) {
    barva = BARVA_CERVENA;
  } else {
    // Doba jednoho bliknuti: u zony STOP 200 ms, na hranici PARKOVACI_DALEKO_CM 800 ms
    unsigned long perioda = map((long)vzdalenost, PREKAZKA_STOP_CM, PARKOVACI_DALEKO_CM, 200, 800);
    bool sviti = (millis() % perioda) < perioda / 2;
    uint32_t barvaBliku = (vzdalenost < PARKOVACI_BLIZKO_CM) ? BARVA_ORANZOVA : BARVA_ZLUTA;
    barva = sviti ? barvaBliku : 0;
  }
  leva = prava = barva;
}

// Duha: barvy se pomalu prelevaji, prava LED ma vzdy opacnou barvu nez leva
void efektDuha(uint32_t &leva, uint32_t &prava) {
  uint16_t odstin = (millis() / DUHA_KROK_MS) * 512;  // 128 kroku na celou duhu (asi 10 s)
  leva = rgbWS.gamma32(rgbWS.ColorHSV(odstin));
  prava = rgbWS.gamma32(rgbWS.ColorHSV(odstin + 32768));
}

// Zmena, kdy svitit (z ovladace nebo IR tlacitkem): pipne a da vedet ovladaci
void nastavSvetla(KdySvitit kdy) {
  svetlaKdy = kdy;
  pip();
  posliStavSvetel();
}

// Zmena svetelneho efektu (z ovladace nebo IR tlacitkem): pipne a da vedet ovladaci
void nastavEfekt(Efekt efekt) {
  svetlaEfekt = efekt;
  pip();
  posliStavSvetel();
}

// Nastavi obe RGB LED na stejnou barvu
void nastavLed(byte r, byte g, byte b) {
  rgbWS.setPixelColor(0, rgbWS.Color(r, g, b));
  rgbWS.setPixelColor(1, rgbWS.Color(r, g, b));
  rgbWS.show();
}

// Pipnuti - bzucak hraje sam, program mezitim bezi dal
void pip() {
  tone(bzucak, PIP_FREKVENCE, PIP_DELKA);
}

// Spusti varovani pred prekazkou (pipnuti pak hraje hlidejZvuk)
void varovani() {
  varovaniZbyva = VAROVANI_POCET;
  varovaniCas = millis() - VAROVANI_ROZESTUP_MS;  // Prvni pipnuti hned
}

// Zahraje dalsi varovne pipnuti, kdyz na nej prisel cas
void hlidejZvuk() {
  if (varovaniZbyva == 0) return;
  if (millis() - varovaniCas < VAROVANI_ROZESTUP_MS) return;
  varovaniCas = millis();
  tone(bzucak, VAROVANI_FREKVENCE, VAROVANI_DELKA);
  varovaniZbyva--;
}

// =====================================================================
// MOTORY
// =====================================================================

// Nastavi oba motory najednou.
// Rychlost -255 az 255: kladna = dopredu, zaporna = dozadu, 0 = stop.
// Kalibrace (KOREKCE_x) a obraceny smer (MOTOR_x_OBRACENY) se resi jen tady,
// zbytek programu uz na ne nemusi myslet.
void motory(int levy, int pravy) {
  motorL = levy;   // Zapamatovat pro blinkry
  motorP = pravy;
  nastavMotor(pwmLeft, left, levy, KOREKCE_L, MOTOR_L_OBRACENY);
  nastavMotor(pwmRight, right, pravy, KOREKCE_R, MOTOR_R_OBRACENY);
}

// Nastavi jeden motor (pouziva ji funkce motory)
void nastavMotor(unsigned int pwmPin, unsigned int smerPin, int rychlost, unsigned int korekce, bool obraceny) {
  bool dopredu = (rychlost >= 0);
  long pwm = abs(rychlost) * (100L + korekce) / 100;  // pricteni kalibrace v procentech
  if (pwm > 255) pwm = 255;
  digitalWrite(smerPin, (dopredu != obraceny) ? HIGH : LOW);
  analogWrite(pwmPin, pwm);
}

// Vrati dany pocet procent z rychlosti
int procent(int rychlost, unsigned int procenta) {
  return (long)rychlost * procenta / 100;
}

void motoryStop()  { motory(0, 0); }
void jedDopredu()  { motory(rychlost, rychlost); }    // ultrazvukem napred
void jedDozadu()   { motory(-rychlost, -rychlost); }
void otocDoleva()  { motory(-procent(rychlost, ZATACENI_PROCENT), rychlost); }   // leve kolo pomaleji pozpatku
void otocDoprava() { motory(rychlost, -procent(rychlost, ZATACENI_PROCENT)); }  // prave kolo pomaleji pozpatku

// Jemne zataceni do oblouku: obe kola dopredu, vnitrni pomaleji. strana: -1 = doleva, 1 = doprava
void zatocJemne(int strana) {
  int vnitrni = procent(rychlost, CARA_JEMNE_PROCENT);
  if (strana < 0) motory(vnitrni, rychlost);
  else motory(rychlost, vnitrni);
}

// Ostre zataceni: vnitrni kolo pozpatku. strana: -1 = doleva, 1 = doprava
void zatocOstre(int strana) {
  if (strana < 0) otocDoleva();
  else otocDoprava();
}
