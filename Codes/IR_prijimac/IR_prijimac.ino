#include <IRremote.hpp>  // IRremote 4.x

const unsigned int ir_prijimac = 2;  // IR prijimac na pinu D2

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(ir_prijimac, ENABLE_LED_FEEDBACK);  // Start prijimace, pri prijmu kodu blika integrovana LED na desce Arduino NANO
}

void loop() {
  if (IrReceiver.decode()) {  // Pokud je prijaty kod
    // Vypise na seriovou konzoli protokol, adresu a cislo tlacitka (Command), napr.:
    // Protocol=NEC Address=0x0 Command=0x40 ...
    // Pri delsim stisku tlacitka se vypisuji opakovani se stejnym Command a slovem Repeat.
    IrReceiver.printIRResultShort(&Serial);
    IrReceiver.resume();  // Pripravi prijimac na dalsi kod
  }
}
