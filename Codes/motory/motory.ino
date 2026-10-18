#include <IRremote.hpp>  // IRremote 4.x

const unsigned int ir_prijimac = 2;  // IR prijimac na pinu D2
unsigned long casPovelu = 0;  // Kdy naposledy prisel povel z ovladace (ms)

// Motor A, Leva strana
const unsigned int pwmLeft = 6; // PWM pin pro motor A
const unsigned int left = 7; // Dir pin, ktery urcuje smer pro motor A
unsigned int korekce_L = 0;  // Promenna pro nastaveni korekce rychlosti leveho motoru
 
// Motor B, Prava strana
const unsigned int pwmRight = 5; // PWM pin pro motor B
const unsigned int right = 4; // Dir pin, ktery urcuje smer pro motor B
unsigned int korekce_R = 0;  // Promenna pro nastaveni korekce rychlosti praveho motoru

unsigned int rychlost = 200;  // Rychlost jizdy v rozsahu 0 - 255, 0 = stop, 255 = rychlost 100%

void setup() {   
  pinMode(pwmLeft, OUTPUT);  // PWM pin A jako vystup
  pinMode(left, OUTPUT);  // Dir pin A jako vystup
  pinMode(pwmRight, OUTPUT);  // PWM pin B jako vystup
  pinMode(right, OUTPUT);  // Dir pin B jako vystup 

  IrReceiver.begin(ir_prijimac);  // Start prijimace
  motoryStop();  // Motory stop
}

void loop() {
  if (IrReceiver.decode()) {  // Pokud je prijaty kod
    // Jen nas ovladac Car MP3 (protokol NEC). Pri drzeni tlacitka chodi opakovani se stejnym cislem tlacitka.
    if (IrReceiver.decodedIRData.protocol == NEC) {
      switch (IrReceiver.decodedIRData.command) {  // Podle cisla tlacitka ovladame motory
        case 0x40:  // Tlacitko >>|
          jedDopredu();
          casPovelu = millis();
          break;
        case 0x19:  // Tlacitko 100+
          jedDozadu();
          casPovelu = millis();
          break;
        case 0x07:  // Tlacitko -
          otocDoleva();
          casPovelu = millis();
          break;
        case 0x09:  // Tlacitko EQ
          otocDoprava();
          casPovelu = millis();
          break;
        default:
          //
          break;
      }
    }
    IrReceiver.resume();  // Pripravi prijimac na dalsi kod
  }

  if (millis() - casPovelu > 250) {  // Kdyz uz neprichazi zadny kod tlacitek dalkoveho ovladace, zastav motory
    motoryStop();
  }
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