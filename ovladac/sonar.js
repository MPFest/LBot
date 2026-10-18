// LBot ovladac - sonar shora (kresleni do canvasu)
//
// Robot je dole uprostred, pred nim kuzel ultrazvuku. Meritko je odmocninove:
// blizke okoli (kde se robot zastavuje) zabira vic mista nez daleke.
//   sonar.nastavZony(stop, volno, max) - z radku "K:" od robota
//   sonar.data(cm, rezim, cidloL, cidloP) - z radku "T:" (cm = -1: nic v dosahu)
//   sonar.bezDat() - data neprichazeji, sonar se ztlumi

const SONAR_UHEL = 20 * Math.PI / 180;  // Polovina sirky kuzele (kresleni, ne presna fyzika)
const SONAR_RYCHLOST_VLNY = 0.9;        // Rychlost vlny v podilech dosahu za sekundu
const SONAR_ZPOZDENI_VLN = [130, 900];  // Interval vln u nejblizsi a nejvzdalenejsi prekazky (ms)

const BARVY = {
  hlubina: '#0e2a47',
  hlubinaSvetla: '#174a74',
  azur: '#2ec4d6',
  zluta: '#f5c518',
  oranzova: '#ff7a1a',
  cervena: '#e8253b',
  hlinik: '#d5d9dd',
  hlinikTmavy: '#9aa2aa',
  motor: '#f2b705',
  deska: '#1f5fbf',
};

class Sonar {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.zony = { stop: 10, volno: 13, max: 100 };
    this.cil = -1;           // Posledni namerena vzdalenost (cm)
    this.zobrazeno = -1;     // Plynule dopocitavana vzdalenost pro kresleni
    this.viditelnost = 0;    // 0 = stena neni videt, 1 = plne videt
    this.rezim = 'R';
    this.cidla = [false, false];
    this.maData = false;
    this.vlny = [];
    this.posledniVlna = 0;
    this.klid = window.matchMedia('(prefers-reduced-motion: reduce)');

    new ResizeObserver(() => this.zmenaVelikosti()).observe(canvas);
    this.zmenaVelikosti();
    this.minule = performance.now();
    requestAnimationFrame((t) => this.snimek(t));
  }

  nastavZony(stop, volno, max) {
    this.zony = { stop, volno, max };
  }

  data(cm, rezim, cidloL, cidloP) {
    this.cil = cm;
    if (this.zobrazeno < 0 && cm >= 0) this.zobrazeno = cm;  // Stena se objevi rovnou na miste
    this.rezim = rezim;
    this.cidla = [cidloL, cidloP];
    this.maData = true;
  }

  bezDat() {
    this.maData = false;
    this.cil = -1;
  }

  // Barva podle vzdalenosti: daleko azurova -> zluta -> oranzova -> u STOP cervena
  barva(cm) {
    const { stop, volno } = this.zony;
    const body = [
      [stop, BARVY.cervena],
      [volno + 5, BARVY.oranzova],
      [Math.max(35, volno + 10), BARVY.zluta],
      [Math.max(70, volno + 30), BARVY.azur],
    ];
    if (cm < 0) return BARVY.azur;
    if (cm <= body[0][0]) return body[0][1];
    for (let i = 1; i < body.length; i++) {
      if (cm <= body[i][0]) {
        const t = (cm - body[i - 1][0]) / (body[i][0] - body[i - 1][0]);
        return smichej(body[i - 1][1], body[i][1], t);
      }
    }
    return BARVY.azur;
  }

  zmenaVelikosti() {
    const dpr = window.devicePixelRatio || 1;
    const sirka = this.canvas.clientWidth;
    const vyska = this.canvas.clientHeight;
    this.canvas.width = Math.round(sirka * dpr);
    this.canvas.height = Math.round(vyska * dpr);
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.W = sirka;
    this.H = vyska;

    // Rozmery robota a stred sonaru (cidlo na predku robota)
    this.robotSirka = Math.max(44, Math.min(sirka, vyska) * 0.16);
    this.robotDelka = this.robotSirka * 1.15;
    this.stred = { x: sirka / 2, y: vyska - 16 - this.robotDelka };
    this.dosah = Math.max(40, this.stred.y - 22);
  }

  // Vzdalenost v cm -> polomer v pixelech (odmocninove meritko)
  polomer(cm) {
    const podil = Math.min(1, Math.max(0, cm / this.zony.max));
    return this.dosah * Math.sqrt(podil);
  }

  snimek(cas) {
    const dt = Math.min(100, cas - this.minule);
    this.minule = cas;

    // Plynule priblizeni k namerene hodnote (vzorky chodi kazdych ~60 ms)
    if (this.cil >= 0) {
      this.zobrazeno += (this.cil - this.zobrazeno) * (1 - Math.exp(-dt / 70));
      this.viditelnost = Math.min(1, this.viditelnost + dt / 150);
    } else {
      this.viditelnost = Math.max(0, this.viditelnost - dt / 300);
    }

    this.posunVlny(cas, dt);
    this.kresli(cas);
    requestAnimationFrame((t) => this.snimek(t));
  }

  posunVlny(cas, dt) {
    if (this.klid.matches || !this.maData) {
      this.vlny = [];
      return;
    }
    // Cim bliz prekazka, tim husti vlny (jako parkovaci senzor)
    const vzdalenost = this.cil >= 0 ? this.cil : this.zony.max;
    const podil = Math.min(1, vzdalenost / this.zony.max);
    const interval = SONAR_ZPOZDENI_VLN[0] + (SONAR_ZPOZDENI_VLN[1] - SONAR_ZPOZDENI_VLN[0]) * Math.sqrt(podil);
    if (cas - this.posledniVlna > interval) {
      this.posledniVlna = cas;
      this.vlny.push({ r: 0, zpet: false, sila: 1 });
    }

    const krok = SONAR_RYCHLOST_VLNY * this.dosah * dt / 1000;
    const stena = this.cil >= 0 ? this.polomer(this.zobrazeno) : this.dosah;
    for (const v of this.vlny) {
      if (!v.zpet) {
        v.r += krok;
        if (v.r >= stena) {
          v.r = stena;
          v.zpet = this.cil >= 0;  // Od steny se vrati ozvena, do prazdna vlna jen zmizi
          v.sila = this.cil >= 0 ? 0.55 : 0;
        }
      } else {
        v.r -= krok;
        v.sila -= dt / 900;
      }
    }
    this.vlny = this.vlny.filter((v) => v.sila > 0 && v.r > 0);
  }

  kresli(cas) {
    const { ctx, W, H, stred } = this;
    const nahoru = -Math.PI / 2;
    ctx.clearRect(0, 0, W, H);

    // Pozadi obrazovky
    const pozadi = ctx.createRadialGradient(stred.x, stred.y, 0, stred.x, stred.y, Math.max(W, H));
    pozadi.addColorStop(0, BARVY.hlubinaSvetla);
    pozadi.addColorStop(0.55, BARVY.hlubina);
    pozadi.addColorStop(1, '#091d33');
    ctx.fillStyle = pozadi;
    ctx.fillRect(0, 0, W, H);

    const ztlumeni = this.maData ? 1 : 0.45;
    ctx.globalAlpha = ztlumeni;

    // Merici oblouky s popisky
    ctx.lineWidth = 1;
    ctx.font = '600 11px system-ui, sans-serif';
    ctx.textBaseline = 'middle';
    for (const cm of [10, 25, 50, 100].filter((c) => c <= this.zony.max)) {
      const r = this.polomer(cm);
      ctx.strokeStyle = 'rgba(46, 196, 214, 0.16)';
      ctx.beginPath();
      ctx.arc(stred.x, stred.y, r, Math.PI, 2 * Math.PI);
      ctx.stroke();
      const uhel = nahoru + SONAR_UHEL + 0.12;
      ctx.fillStyle = 'rgba(46, 196, 214, 0.55)';
      ctx.fillText(cm + ' cm', stred.x + Math.cos(uhel) * r + 4, stred.y + Math.sin(uhel) * r);
    }

    // Kuzel ultrazvuku
    const kuzel = ctx.createLinearGradient(0, stred.y, 0, stred.y - this.dosah);
    kuzel.addColorStop(0, 'rgba(46, 196, 214, 0.20)');
    kuzel.addColorStop(1, 'rgba(46, 196, 214, 0.03)');
    ctx.fillStyle = kuzel;
    vysec(ctx, stred, 0, this.dosah, nahoru - SONAR_UHEL, nahoru + SONAR_UHEL);
    ctx.fill();
    ctx.strokeStyle = 'rgba(46, 196, 214, 0.35)';
    ctx.stroke();

    // Zona STOP a hranice "volno"
    ctx.fillStyle = 'rgba(232, 37, 59, 0.22)';
    vysec(ctx, stred, 0, this.polomer(this.zony.stop), nahoru - SONAR_UHEL, nahoru + SONAR_UHEL);
    ctx.fill();
    ctx.setLineDash([4, 4]);
    ctx.strokeStyle = 'rgba(255, 122, 26, 0.7)';
    ctx.beginPath();
    ctx.arc(stred.x, stred.y, this.polomer(this.zony.volno), nahoru - SONAR_UHEL, nahoru + SONAR_UHEL);
    ctx.stroke();
    ctx.setLineDash([]);

    // Vlny a ozveny
    ctx.lineWidth = 2;
    for (const v of this.vlny) {
      ctx.strokeStyle = `rgba(46, 196, 214, ${0.75 * v.sila})`;
      ctx.beginPath();
      ctx.arc(stred.x, stred.y, v.r, nahoru - SONAR_UHEL, nahoru + SONAR_UHEL);
      ctx.stroke();
    }

    // Prekazka: zarici stena + stin za ni
    if (this.viditelnost > 0 && this.zobrazeno >= 0) {
      const r = this.polomer(this.zobrazeno);
      const barva = this.barva(this.zobrazeno);
      const sirka = SONAR_UHEL + 0.14;
      ctx.globalAlpha = ztlumeni * this.viditelnost;
      ctx.fillStyle = 'rgba(5, 14, 26, 0.45)';
      vysec(ctx, stred, r + 6, this.dosah, nahoru - sirka, nahoru + sirka);
      ctx.fill();
      ctx.shadowColor = barva;
      ctx.shadowBlur = 22;
      ctx.strokeStyle = barva;
      ctx.lineWidth = Math.max(6, this.robotSirka * 0.16);
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.arc(stred.x, stred.y, r, nahoru - sirka, nahoru + sirka);
      ctx.stroke();
      ctx.shadowBlur = 0;
      ctx.lineCap = 'butt';
      ctx.globalAlpha = ztlumeni;
    }

    this.kresliRobota();
    ctx.globalAlpha = 1;

    // Couvani: cervene pulzovani cele obrazovky
    if (this.maData && this.rezim === 'Z') {
      const pulz = this.klid.matches ? 0.5 : 0.5 + 0.5 * Math.sin(cas / 90);
      ctx.fillStyle = `rgba(232, 37, 59, ${0.10 + 0.18 * pulz})`;
      ctx.fillRect(0, 0, W, H);
    }
  }

  kresliRobota() {
    const { ctx, stred } = this;
    const s = this.robotSirka;
    const d = this.robotDelka;
    const x = stred.x - s / 2;
    const y = stred.y;
    const cidloX = [stred.x - s * 0.24, stred.x + s * 0.24];

    // Podlaha pod robotem (svetly kruh) a na ni cara pod cidlem, ktere ji vidi
    const podlahaY = y + d * 0.5;
    const podlaha = ctx.createRadialGradient(stred.x, podlahaY, 0, stred.x, podlahaY, s * 1.3);
    podlaha.addColorStop(0, 'rgba(213, 217, 221, 0.30)');
    podlaha.addColorStop(1, 'rgba(213, 217, 221, 0)');
    ctx.fillStyle = podlaha;
    ctx.fillRect(stred.x - s * 1.3, podlahaY - s * 1.3, s * 2.6, s * 2.6);
    const naCare = this.maData ? this.cidla : [false, false];
    if (naCare[0] || naCare[1]) {
      const levyOkraj = naCare[0] ? cidloX[0] - s * 0.17 : cidloX[1] - s * 0.17;
      const pravyOkraj = naCare[1] ? cidloX[1] + s * 0.17 : cidloX[0] + s * 0.17;
      const zacatek = y - d * 0.45;  // Cara vede i kousek pred robota
      ctx.fillStyle = '#020509';
      ctx.fillRect(levyOkraj, zacatek, pravyOkraj - levyOkraj, this.H - zacatek);
      ctx.strokeStyle = 'rgba(234, 244, 251, 0.45)';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(levyOkraj, zacatek);
      ctx.lineTo(levyOkraj, this.H);
      ctx.moveTo(pravyOkraj, zacatek);
      ctx.lineTo(pravyOkraj, this.H);
      ctx.stroke();
    }

    // Kola (zlute motory TT)
    ctx.fillStyle = BARVY.motor;
    zaoblenyObdelnik(ctx, x - s * 0.2, y + d * 0.42, s * 0.2, d * 0.42, 4);
    ctx.fill();
    zaoblenyObdelnik(ctx, x + s, y + d * 0.42, s * 0.2, d * 0.42, 4);
    ctx.fill();

    // Hlinikovy podvozek
    const hlinik = ctx.createLinearGradient(x, 0, x + s, 0);
    hlinik.addColorStop(0, BARVY.hlinikTmavy);
    hlinik.addColorStop(0.5, BARVY.hlinik);
    hlinik.addColorStop(1, BARVY.hlinikTmavy);
    ctx.fillStyle = hlinik;
    zaoblenyObdelnik(ctx, x, y + d * 0.08, s, d * 0.92, s * 0.12);
    ctx.fill();

    // Deska s Arduinem
    ctx.fillStyle = '#2b3a2f';
    zaoblenyObdelnik(ctx, x + s * 0.2, y + d * 0.4, s * 0.6, d * 0.42, 3);
    ctx.fill();

    // Ultrazvuk vpredu: modra deska a dve "oci"
    ctx.fillStyle = BARVY.deska;
    zaoblenyObdelnik(ctx, x + s * 0.06, y - d * 0.02, s * 0.88, d * 0.2, 3);
    ctx.fill();
    for (const ox of [x + s * 0.28, x + s * 0.72]) {
      ctx.fillStyle = '#c7ccd1';
      ctx.beginPath();
      ctx.arc(ox, y + d * 0.08, s * 0.14, 0, 2 * Math.PI);
      ctx.fill();
      ctx.fillStyle = '#3a4148';
      ctx.beginPath();
      ctx.arc(ox, y + d * 0.08, s * 0.08, 0, 2 * Math.PI);
      ctx.fill();
    }

    // Kontrolky cidel cary
    for (let i = 0; i < 2; i++) {
      const sviti = naCare[i];
      ctx.fillStyle = sviti ? BARVY.zluta : '#4a535c';
      if (sviti) {
        ctx.shadowColor = BARVY.zluta;
        ctx.shadowBlur = 12;
      }
      ctx.beginPath();
      ctx.arc(cidloX[i], y + d * 0.28, Math.max(4, s * 0.08), 0, 2 * Math.PI);
      ctx.fill();
      ctx.shadowBlur = 0;
    }
  }
}

// Vysec mezikruzi (od polomeru r1 do r2, mezi uhly)
function vysec(ctx, stred, r1, r2, od, do_) {
  ctx.beginPath();
  ctx.arc(stred.x, stred.y, r2, od, do_);
  if (r1 > 0) ctx.arc(stred.x, stred.y, r1, do_, od, true);
  else ctx.lineTo(stred.x, stred.y);
  ctx.closePath();
}

function zaoblenyObdelnik(ctx, x, y, w, h, r) {
  ctx.beginPath();
  ctx.roundRect(x, y, w, h, r);
}

function smichej(a, b, t) {
  const ca = parseInt(a.slice(1), 16);
  const cb = parseInt(b.slice(1), 16);
  const kanal = (posun) => Math.round(((ca >> posun) & 255) * (1 - t) + ((cb >> posun) & 255) * t);
  return `rgb(${kanal(16)}, ${kanal(8)}, ${kanal(0)})`;
}
