const unsigned int cidlo_L = 9;  // Leve cidlo na pinu 9
const unsigned int cidlo_R = 10;  // Prave cidlo na pinu 10

// Logika cidel cary - muze se u ruznych kusu lisit. Zjistis ji podle LED na cidle:
//   true  = LED sviti nad svetlou podlahou a nad carou zhasne
//   false = LED sviti nad carou a nad svetlou podlahou zhasne
const bool CIDLO_CARY_SVETLO_JE_HIGH = true;

// Zpusob sledovani cary:
//   true  = robot jede PO care, obe cidla nad carou (siroka cara, sirsi nez rozestup cidel)
//   false = robot jede rozkrocmo, cara je MEZI cidly (tenka cara, uzsi nez rozestup cidel)
const bool JIZDA_PO_CARE = true;

// Motor A, Leva strana
const unsigned int pwmLeft = 6; // PWM pin pro motor A
const unsigned int left = 7; // Dir pin, ktery urcuje smer pro motor A
unsigned int korekce_L = 0;  // Promenna pro nastaveni korekce rychlosti leveho motoru
 
// Motor B, Prava strana
const unsigned int pwmRight = 5; // PWM pin pro motor B
const unsigned int right = 4; // Dir pin, ktery urcuje smer pro motor B
unsigned int korekce_R = 0;  // Promenna pro nastaveni korekce rychlosti praveho motoru

unsigned int rychlost = 70;  // Rychlost jizdy v rozsahu 0 - 255, 0 = stop, 255 = rychlost 100%

const int tlacitko = A7;
bool stav_tlacitka = 0;
bool on = 0;
const char* posledniStav = "";  // Co se naposledy vypsalo na seriovou konzoli

void setup () {
  Serial.begin(115200); 
  pinMode(pwmLeft, OUTPUT);  // PWM pin A jako vystup
  pinMode(left, OUTPUT);  // Dir pin A jako vystup
  pinMode(pwmRight, OUTPUT);  // PWM pin B jako vystup
  pinMode(right, OUTPUT);  // Dir pin B jako vystup

  motoryStop();
}

void loop () {
  stav_tlacitka = !(analogRead(tlacitko) >= 512);
  if (stav_tlacitka) {
    if (on){  // Pokud byl spusten rezim sledovani cary
      on = 0;  // Vypni rezim sledovani cary
     Serial.println("OFF");
    }else{  // Pokud nebyl spusten rezim sledovani cary
      on = 1;  // Zapni rezim sledovani cary
     Serial.println("ON");
    }
    delay(300);
  }

  if (on){  // Pokud byl zapnut rezim sledovani cary
    bool s1 = naSvetlePodlaze(cidlo_L);  // true = leve cidlo vidi svetlou podlahu, false = caru
    bool s2 = naSvetlePodlaze(cidlo_R);  // true = prave cidlo vidi svetlou podlahu, false = caru

    // Kdy jet rovne: po care = obe cidla na care, rozkrocmo = obe cidla na svetle podlaze
    bool rovne = JIZDA_PO_CARE ? (!s1 && !s2) : (s1 && s2);
    const char* stav;
    if (rovne) {
      jedDopredu();
      stav = "Dopredu";
    } else if (s1 && !s2) {  // Cara je pod pravym cidlem
      otocDoprava();
      stav = "Doprava";
    } else if (!s1 && s2) {  // Cara je pod levym cidlem
      otocDoleva();
      stav = "Doleva";
    } else {  // Po care: ztratil caru / rozkrocmo: prekrizena cara (cil)
      motoryStop();
      stav = "Stop";
    }

    // Vypisovat jen zmeny - jinak se seriova konzole zahlti a vypis se zpozduje o nekolik sekund
    if (stav != posledniStav) {
      Serial.println(stav);
      posledniStav = stav;
    }
  }else{  // Pokud byl vypnut rezim sledovani cary
    motoryStop();
  }
}

// Vrati true, kdyz cidlo vidi svetlou podlahu (podle prepinace CIDLO_CARY_SVETLO_JE_HIGH)
bool naSvetlePodlaze(unsigned int pin) {
  return (digitalRead(pin) == HIGH) == CIDLO_CARY_SVETLO_JE_HIGH;
}

// Pomocna procedura pro vypocet korigovane rychlosti otaceni leveho motoru 
unsigned int speed_L(unsigned int rychlost){
  unsigned int rychlost_L = rychlost + map(korekce_L, 0, 100, 0, rychlost);
  if (rychlost_L > 255) rychlost_L = 255;
  if (rychlost_L < 0) rychlost_L = 0;
  return rychlost_L;
}

// Pomocna procedura pro vypocet korigovane rychlosti otaceni praveho motoru
unsigned int speed_R(unsigned int rychlost){
  unsigned int rychlost_R = rychlost + map(korekce_R, 0, 100, 0, rychlost);
  if (rychlost_R > 255) rychlost_R = 255;
  if (rychlost_R < 0) rychlost_R = 0;
  return rychlost_R;
}

// Zastavi motory
void motoryStop() {
  digitalWrite(left, LOW); // Dir pin motoru A na log. 0
  digitalWrite(right, LOW);  // Dir pin motoru B na log. 0
  analogWrite(pwmLeft, 0);  // PWM motoru A na 0%
  analogWrite(pwmRight, 0);  // PWM motoru B na 0%
}

// Jizda dopredu
void jedDopredu() {
  korekce_L = 7;  // Korekce rychlosti leveho motoru 7% nastavene rychlosti
  korekce_R = 0;
  unsigned int rychlost_L = speed_L(rychlost);  // Vypocita rychlost leveho motoru
  unsigned int rychlost_R = speed_R(rychlost);  // Vypocita rychlost praveho motoru
  digitalWrite(left, HIGH);  // Dir pin motoru A na log. 1
  digitalWrite(right, HIGH);  // Dir pin motoru B na log. 1
  analogWrite(pwmLeft, rychlost_L);  // PWM motoru A na nastavenou rychlost
  analogWrite(pwmRight, rychlost_R);  // PWM motoru B na nastavenou rychlost
}

// Jizda dozadu
void jedDozadu() {
  korekce_L = 2;  // Korekce rychlosti leveho motoru 2% nastavene rychlosti
  korekce_R = 0;
  unsigned int rychlost_L = speed_L(rychlost);  // Vypocita rychlost leveho motoru
  unsigned int rychlost_R = speed_R(rychlost);  // Vypocita rychlost praveho motoru
  digitalWrite(left, LOW);  // Dir pin motoru A na log. 0
  digitalWrite(right, LOW);  // Dir pin motoru B na log. 0
  analogWrite(pwmLeft, rychlost_L);  // PWM motoru A na nastavenou rychlost
  analogWrite(pwmRight, rychlost_R);  // PWM motoru B na nastavenou rychlost
}

// Otoci doleva
void otocDoleva() {
  unsigned int rychlost_L = map(50, 0, 100, 0, speed_L(rychlost));  // 50% rychlosti praveho kola
  unsigned int rychlost_R = speed_R(rychlost);
  digitalWrite(left, LOW);  // Dir pin motoru A na log. 0
  digitalWrite(right, HIGH);  // Dir pin motoru B na log. 1
  analogWrite(pwmLeft, rychlost_L);  // PWM motoru A na nastavenou rychlost
  analogWrite(pwmRight, rychlost_R);  // PWM motoru B na nastavenou rychlost
}

// Otoci doprava
void otocDoprava() {
  unsigned int rychlost_L = speed_L(rychlost);
  unsigned int rychlost_R = map(50, 0, 100, 0, speed_R(rychlost));  // 50% rychlosti leveho kola
  digitalWrite(left, HIGH);  // Dir pin motoru A na log. 1
  digitalWrite(right, LOW);  // Dir pin motoru B na log. 0
  analogWrite(pwmLeft, rychlost_L);  // PWM motoru A na nastavenou rychlost
  analogWrite(pwmRight, rychlost_R);  // PWM motoru B na nastavenou rychlost
}