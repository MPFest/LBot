#include <IRremote.hpp>  // IRremote 4.x

const unsigned int ir_vysilac = 3;  // IR vysilac na pinu D3 (na Arduino NANO ho budi casovac Timer2, jiny pin nejde)

const unsigned int tlacitko = A7;
bool stav_tlacitka = 0;

void setup() {
  IrSender.begin(ir_vysilac);  // Inicializace vysilace
}

void loop() {
  stav_tlacitka = !(analogRead(tlacitko) >= 512);
  if (stav_tlacitka) {
    uint8_t tData = 0x41; // Data pro odeslani - ASCII znak A
    IrSender.sendSony(0x00, tData, 0);  // Odesleme data s kodovanim SONY - adresa, cislo tlacitka (command), pocet opakovani
    // Sony je jen ukazkovy protokol ("jazyk" signalu), zadne Sony zarizeni neni potreba.
    //
    // BONUS: vypni robotem televizi, kterou mate doma!
    //  1. Nahraj priklad IR_prijimac (Lekce 6) a zmackni na ovladaci od televize tlacitko Power.
    //     Vypise se napr.: Protocol=Samsung Address=0x707 Command=0x2
    //  2. Radek se sendSony nahrad funkci pro ten protokol s adresou a Command, ktere se vypsaly,
    //     napr. pro Samsung: IrSender.sendSamsung(0x0707, 0x02, 0);
    //     (kody se u ruznych modelu lisi, proto je vzdy zjisti z vlastniho ovladace)
    //  3. Nahraj, namir robota na televizi a zmackni tlacitko na desce.
    // Pozor: nektere ovladace (hlavne "chytre" s mikrofonem) posilaji jen pres Bluetooth, ne IR.
    delay(200);
  }
}
