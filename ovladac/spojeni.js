// LBot ovladac - spojeni s robotem
//
// Kazdy druh spojeni ma stejne rozhrani, takze zbytek ovladace nemusi vedet, jak spojeni funguje:
//   Spojeni.jeDostupne()      - umi to tento prohlizec?
//   await spojeni.pripoj()    - vyber zarizeni a otevreni spojeni (musi byt po kliknuti uzivatele)
//   await spojeni.posli(text) - posle znaky robotovi
//   await spojeni.odpoj()
//   spojeni.priRadku = fn(radek)  - zavola se pro kazdy radek textu, ktery robot posle
//   spojeni.priOdpojeni = fn()    - zavola se, kdyz spojeni spadne nebo se odpoji
//
// SerialSpojeni = Web Serial (Chrome, Edge na Androidu, Windows, Macu, Linuxu):
//   Bluetooth modul HC-06 (nejdriv sparovat v nastaveni Bluetooth, PIN 1234) nebo USB kabel.
// Pozdeji sem muze pribyt BleSpojeni (Web Bluetooth) pro BLE moduly - napr. iPad v prohlizeci Bluefy.

class SerialSpojeni {
  static jeDostupne() {
    return 'serial' in navigator;
  }

  constructor() {
    this.port = null;
    this.pisar = null;
    this.ctenar = null;
    this.cteni = null;   // Bezici cteci smycka (Promise)
    this.priRadku = null;
    this.priOdpojeni = null;
    this.odpojuji = false;
  }

  get pripojeno() {
    return this.pisar !== null;
  }

  async pripoj() {
    // Chrome ukaze seznam sparovanych Bluetooth modulu a USB portu, uzivatel vybere robota
    this.port = await navigator.serial.requestPort();
    // Rychlost musi sedet se Serial.begin(9600) v robotovi (u Bluetooth se nepouziva, ale je povinna)
    await this.port.open({ baudRate: 9600 });
    this.odpojuji = false;
    this.pisar = this.port.writable.getWriter();
    this.cteni = this.ctiSmycka();  // Bezi na pozadi, dokud je spojeni otevrene
  }

  async posli(text) {
    if (!this.pisar) return;
    try {
      await this.pisar.write(new TextEncoder().encode(text));
    } catch (chyba) {
      // Zapis selhal = spojeni spadlo (na Androidu se to jinak nepozna)
      this.spojeniSpadlo();
    }
  }

  async odpoj() {
    if (!this.port) return;
    this.odpojuji = true;
    await this.zastavCteni();
    await this.zavriPort();
  }

  // Cte data od robota a sklada je do radku (robot posila text zakonceny koncem radku)
  async ctiSmycka() {
    const dekoder = new TextDecoder();
    let zbytek = '';
    // port.readable je null, kdyz spojeni spadlo uplne; po drobne chybe prenosu vznikne novy
    while (this.port && this.port.readable && !this.odpojuji) {
      this.ctenar = this.port.readable.getReader();
      try {
        while (true) {
          const { value, done } = await this.ctenar.read();
          if (done) break;
          zbytek += dekoder.decode(value, { stream: true });
          let konec;
          while ((konec = zbytek.indexOf('\n')) >= 0) {
            const radek = zbytek.slice(0, konec).replace(/\r$/, '');
            zbytek = zbytek.slice(konec + 1);
            if (this.priRadku) this.priRadku(radek);
          }
        }
      } catch (chyba) {
        // Chyba prenosu (napr. poskozeny znak) - smycka zkusi cist dal, pokud port jeste zije
      } finally {
        this.ctenar.releaseLock();
        this.ctenar = null;
      }
    }
    if (!this.odpojuji) this.spojeniSpadlo();
  }

  async spojeniSpadlo() {
    if (!this.port || this.odpojuji) return;  // Uz se resi
    this.odpojuji = true;
    await this.zastavCteni();
    await this.zavriPort();
    if (this.priOdpojeni) this.priOdpojeni();
  }

  // Ukonci cteci smycku a pocka, az pusti port (jinak by nesel zavrit)
  async zastavCteni() {
    try {
      if (this.ctenar) await this.ctenar.cancel();
      await this.cteni;
    } catch (chyba) {
      // Cteni uz skoncilo samo
    }
  }

  async zavriPort() {
    const port = this.port;
    try {
      if (this.pisar) this.pisar.releaseLock();
      if (port) await port.close();
    } catch (chyba) {
      // Port uz je zavreny nebo mrtvy - neni co zavirat
    }
    this.port = null;
    this.pisar = null;
    this.ctenar = null;
  }
}

// SimulaceSpojeni = "Vyzkousej bez robota": misto skutecneho robota odpovida jednoduchy
// napodobeny robot, ktery se chova jako demo program (prekazky, couvani, cara, data pro sonar).
class SimulaceSpojeni {
  static jeDostupne() {
    return true;
  }

  constructor() {
    this.priRadku = null;
    this.priOdpojeni = null;
    this.casovac = null;
  }

  get pripojeno() {
    return this.casovac !== null;
  }

  async pripoj() {
    this.vzdalenost = 60;      // Jak daleko je pred robotem prekazka (cm)
    this.rezim = 'R';          // R = rucni jizda, C = cara, Z = couvani
    this.povel = 'o';
    this.povelCas = 0;
    this.couvaniStart = 0;
    this.cekamOd = 0;          // Na care: od kdy stoji pred prekazkou (0 = nestoji)
    this.otoceniCil = null;    // Kam se pri otaceni posune prekazka
    this.telemetrieCas = -Infinity;
    this.telemetrieBezi = false;
    this.svetlaKdy = 'a';        // Svetla: a = potme, s = vzdy, n = nikdy
    this.efekt = 'p';            // p policie, b bila, k blinkry, g parkovaci, h duha
    this.tma = false;
    this.start = performance.now();
    this.casovac = setInterval(() => this.krok(), 60);  // Robot meri kazdych 60 ms
    setTimeout(() => this.radek('Pripraven...'), 300);
  }

  async posli(text) {
    for (const znak of text) this.prijmi(znak);
  }

  async odpoj() {
    clearInterval(this.casovac);
    this.casovac = null;
  }

  prijmi(znak) {
    const ted = performance.now();
    if ('udlr'.includes(znak)) {
      this.povel = znak;
      this.povelCas = ted;
    } else if (znak === 'o') {
      this.povel = 'o';
    } else if (znak === '2') {
      this.rezim = (this.rezim === 'C') ? 'R' : 'C';
      this.cekamOd = 0;
      this.radek(this.rezim === 'C' ? 'Sledovani cary zapnuto' : 'Sledovani cary vypnuto');
    } else if (znak === 'v') {
      if (!this.telemetrieBezi) {
        this.radek('K:10,13,100,500,50');
        this.radek(`S:${this.svetlaKdy},${this.efekt}`);
      }
      this.telemetrieBezi = true;
      this.telemetrieCas = ted;
    } else if ('asn'.includes(znak) || 'pbkgh'.includes(znak)) {
      if ('asn'.includes(znak)) this.svetlaKdy = znak;
      else this.efekt = znak;
      if (this.telemetrieBezi) this.radek(`S:${this.svetlaKdy},${this.efekt}`);
    }
  }

  krok() {
    const ted = performance.now();
    const dt = 0.06;
    const povel = (ted - this.povelCas <= 500) ? this.povel : 'o';  // Pojistka jako BT_TIMEOUT_MS

    if (this.rezim === 'R') {
      if (povel === 'u') {
        if (this.vzdalenost <= 10) {
          this.rezim = 'Z';
          this.couvaniStart = ted;
        } else {
          this.vzdalenost -= 30 * dt;
        }
      } else if (povel === 'd') {
        this.vzdalenost += 20 * dt;
      }
      // Pri otaceni se pred robotem objevi neco jineho
      if (povel === 'l' || povel === 'r') {
        if (this.otoceniCil === null) this.otoceniCil = 15 + Math.random() * 110;
        this.vzdalenost += (this.otoceniCil - this.vzdalenost) * 0.08;
      } else {
        this.otoceniCil = null;
      }
    } else if (this.rezim === 'Z') {
      this.vzdalenost += 12 * dt;
      if (this.vzdalenost >= 13 || ted - this.couvaniStart > 2000) this.rezim = 'R';
    } else if (this.rezim === 'C') {
      if (this.cekamOd) {
        // Po chvili nekdo prekazku odnese
        if (ted - this.cekamOd > 2500) {
          this.vzdalenost = 50 + Math.random() * 60;
          this.cekamOd = 0;
        }
      } else {
        this.vzdalenost -= 15 * dt;
        if (this.vzdalenost <= 10) this.cekamOd = ted;
      }
    }
    this.vzdalenost = Math.min(160, Math.max(3, this.vzdalenost));

    if (this.telemetrieBezi && ted - this.telemetrieCas > 1500) this.telemetrieBezi = false;
    if (this.telemetrieBezi) {
      const sum = (Math.random() - 0.5) * 0.8;  // Skutecne cidlo trochu kolisa
      const cm = this.vzdalenost > 100 ? -1 : Math.round(this.vzdalenost + sum);
      let cidla = '00';
      if (this.rezim === 'C') cidla = ['11', '10', '11', '01'][Math.floor(ted / 700) % 4];
      // Okolni svetlo pomalu kolisa mezi dnem a noci (asi 40 s), tma s hysterezi jako v robotovi
      const svetlo = Math.round(600 + 380 * Math.cos((ted - this.start) / 40000 * 2 * Math.PI));
      if (this.tma && svetlo > 550) this.tma = false;
      if (!this.tma && svetlo < 450) this.tma = true;
      this.radek(`T:${cm},${this.rezim},${cidla},${svetlo},${this.tma ? 1 : 0}`);
    }
  }

  radek(text) {
    if (this.priRadku) this.priRadku(text);
  }
}
