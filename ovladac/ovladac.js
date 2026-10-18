// LBot ovladac - ovladani robota z prohlizece
// Autor: Martin Petrik (MPFest) - https://github.com/MPFest/LBot

// =====================================================================
// KONFIGURACE
// =====================================================================

// Znaky povelu - musi sedet s BT_... v KONFIGURACI demo programu robota
const POVELY = {
  dopredu: 'u',
  dozadu: 'd',
  doleva: 'l',
  doprava: 'r',
  stop: 'o',
  cara: '2',
  data: 'v',  // Zadost o data pro sonar (BT_TELEMETRIE)
};

// Pri drzeni tlacitka se povel posila porad dokola. Robot zastavi, kdyz povel neprijde
// dele nez BT_TIMEOUT_MS (500 ms) - opakovani musi byt vyrazne rychlejsi.
const OPAKOVANI_MS = 150;

// Jak casto si rict o data pro sonar - robot je posila jeste TELEMETRIE_DOBA_MS (1500 ms) po zadosti
const ZADOST_O_DATA_MS = 500;
// Kdyz data neprijdou tak dlouho, robot je asi neumi posilat (napr. Lekce 11)
const DATA_CHYBI_MS = 2000;

// Svetla: znak povelu -> nazev. Znaky musi sedet s BT_SVETLA_... a BT_EFEKT_... v demu;
// robot hlasi nastaveni radkem "S:kdy,efekt" se stejnymi znaky.
const SVETLA_KDY = { a: 'Potmě', s: 'Vždy', n: 'Nikdy' };
const SVETLA_EFEKTY = { p: 'Policie', b: 'Bílá', k: 'Blinkry', g: 'Parkovací', h: 'Duha' };

// Klavesnice na pocitaci
const KLAVESY = {
  ArrowUp: 'dopredu', KeyW: 'dopredu',
  ArrowDown: 'dozadu', KeyS: 'dozadu',
  ArrowLeft: 'doleva', KeyA: 'doleva',
  ArrowRight: 'doprava', KeyD: 'doprava',
};

// Texty v ovladaci
const NAZVY_SMERU = { dopredu: 'dopředu', dozadu: 'dozadu', doleva: 'doleva', doprava: 'doprava' };
const STAVY = {
  odpojeno: { lista: 'Nepřipojeno', nadpis: 'Připoj robota', text: 'Zapni robota a v nabídce vyber HC-06.', pripojit: 'Připojit robota' },
  pripojuji: { lista: 'Připojuji…', nadpis: 'Vyber robota', text: 'V nabídce prohlížeče vyber HC-06.' },
  pripojeno: { lista: 'Připojeno' },
  simulace: { lista: 'Simulace' },
  ztraceno: { lista: 'Spojení ztraceno', nadpis: 'Spojení se ztratilo', text: 'Robot zastavil sám. Zkontroluj, že je zapnutý a nablízku.', pripojit: 'Připojit znovu' },
  nepodporovano: {
    lista: 'Prohlížeč neumí Bluetooth',
    nadpis: 'Tady s robotem mluvit nejde',
    text: 'Otevři ovladač v Chrome nebo Edge na Androidu, Windows, Macu nebo Linuxu. Na iPhonu a iPadu to s modulem HC-06 nejde.',
  },
};
const STUPNE_SVETLA = ['jasno', 'světlo', 'šero', 'tma', 'hluboká tma'];
const SVETLA_HLASKY = {
  odpojeno: 'Připoj robota a pak tu nastavíš jeho světla.',
  neumi: 'Program v robotovi nastavení světel neumí. Potřebuje demo program v2.0.',
};
const REZIMY = {
  R: 'Ruční jízda',
  C: 'Sleduje čáru',
  cekam: 'Čeká, až překážka zmizí',
  Z: 'Couvá od překážky',
};

// =====================================================================
// STAV A SPOJENI
// =====================================================================

const el = (id) => document.getElementById(id);
const sonar = new Sonar(el('sonar'));

let spojeni = null;           // SerialSpojeni nebo SimulaceSpojeni
let zadostOData = null;       // Casovac zadosti o data
let posledniData = 0;         // Kdy prisel posledni radek "T:" (ms)
let zony = { stop: 10, volno: 13, max: 100 };
let rezim = null;             // Posledni rezim robota z dat (R / C / Z)
let cekam = false;            // Na care stoji pred prekazkou
let svetloZony = null;        // Kdy je tma: { prah, hystereze } z radku "K:"
let svetla = { kdy: null, efekt: null, tma: false };  // Nastaveni svetel z robota ("S:")

const jePripojeno = () => spojeni !== null && spojeni.pripojeno;

function nastavStav(stav) {
  const texty = STAVY[stav];
  document.body.dataset.stav = stav;
  el('stav').textContent = texty.lista;

  const pripojeno = (stav === 'pripojeno' || stav === 'simulace');
  el('spojeniTlacitko').textContent = (stav === 'simulace') ? 'Ukončit simulaci' : (pripojeno ? 'Odpojit' : 'Připojit');
  el('spojeniTlacitko').hidden = (stav === 'nepodporovano' || stav === 'pripojuji');

  // Vyzva pres obrazovku, kdyz robot neni pripojeny
  el('vyzva').hidden = pripojeno;
  if (!pripojeno) {
    el('vyzvaNadpis').textContent = texty.nadpis;
    el('vyzvaText').textContent = texty.text;
    el('pripojitRobota').hidden = !texty.pripojit;
    if (texty.pripojit) el('pripojitRobota').textContent = texty.pripojit;
    el('simulace').hidden = (stav === 'pripojuji');
    zobrazRezim(null);
    sonar.bezDat();
    el('cm').textContent = '–';
    el('cm').style.color = '';
    el('bezDat').hidden = true;
    zobrazSvetlo(null, false);
    svetla = { kdy: null, efekt: null, tma: false };
    zobrazNastaveniSvetel();
  }
}

async function pripoj(simulace) {
  spojeni = simulace ? new SimulaceSpojeni() : new SerialSpojeni();
  spojeni.priRadku = prijatRadek;
  spojeni.priOdpojeni = () => {
    ukonciSpojeni();
    nastavStav('ztraceno');
    zapis('• spojení ztraceno');
  };

  nastavStav('pripojuji');
  try {
    await spojeni.pripoj();
  } catch (chyba) {
    // Uzivatel zavrel vyber zarizeni, nebo se port nepodarilo otevrit
    spojeni = null;
    nastavStav(SerialSpojeni.jeDostupne() ? 'odpojeno' : 'nepodporovano');
    if (chyba.name !== 'NotFoundError') zapis('• nepodařilo se připojit: ' + chyba.message);
    return;
  }

  nastavStav(simulace ? 'simulace' : 'pripojeno');
  zobrazNastaveniSvetel();  // Dokud robot nenahlasi svetla ("S:"), jsou prepinace zakazane
  zapis(simulace ? '• simulace spuštěna' : '• připojeno');
  posledniData = performance.now();
  posli(POVELY.stop);
  posli(POVELY.data);
  zadostOData = setInterval(hlidejData, ZADOST_O_DATA_MS);
}

async function odpoj() {
  const simulace = (document.body.dataset.stav === 'simulace');
  const stare = spojeni;
  ukonciSpojeni();
  await stare.odpoj();
  nastavStav(SerialSpojeni.jeDostupne() ? 'odpojeno' : 'nepodporovano');
  zapis(simulace ? '• simulace ukončena' : '• odpojeno');
}

function ukonciSpojeni() {
  clearInterval(zadostOData);
  zadostOData = null;
  uvolniVse();  // Jeste pri spojeni - posle robotovi stop, pokud jel
  spojeni = null;
}

function posli(znak) {
  if (jePripojeno()) spojeni.posli(znak);
}

// Pravidelne: rict si o data a zkontrolovat, jestli chodi
function hlidejData() {
  posli(POVELY.data);
  const chybi = performance.now() - posledniData > DATA_CHYBI_MS;
  el('bezDat').hidden = !chybi;
  if (chybi) {
    sonar.bezDat();
    zobrazSvetlo(null, false);
    zobrazRezim(null);
    el('cm').textContent = '–';
    el('cm').style.color = '';
  }
}

// =====================================================================
// DATA OD ROBOTA
// =====================================================================

function prijatRadek(radek) {
  if (radek === '') return;

  // Nastaveni zon: "K:stop,volno,max"
  if (radek.startsWith('K:')) {
    const [stop, volno, max, prah, hystereze] = radek.slice(2).split(',').map(Number);
    if (stop > 0 && volno >= stop && max > volno) {
      zony = { stop, volno, max };
      sonar.nastavZony(stop, volno, max);
    }
    // Kdy je tma (jen novejsi program v robotovi)
    if (prah > 0 && hystereze >= 0) svetloZony = { prah, hystereze };
    return;
  }

  // Nastaveni svetel: "S:kdy,efekt"
  if (radek.startsWith('S:')) {
    const [kdy, efekt] = radek.slice(2).split(',');
    if (SVETLA_KDY[kdy] && SVETLA_EFEKTY[efekt]) {
      svetla.kdy = kdy;
      svetla.efekt = efekt;
      zobrazNastaveniSvetel();
    }
    return;
  }

  // Data pro sonar: "T:vzdalenost,rezim,cidla" - chodi 16x za sekundu, do vypisu nepatri
  if (radek.startsWith('T:')) {
    const [cm, r, cidla, svetlo, tma] = radek.slice(2).split(',');
    const vzdalenost = Number(cm);
    if (!Number.isFinite(vzdalenost) || !'RCZ'.includes(r) || !/^[01]{2}$/.test(cidla || '')) return;
    zpracujData(vzdalenost, r, cidla[0] === '1', cidla[1] === '1');
    // Okolni svetlo posila jen novejsi program v robotovi
    const hodnota = (svetlo === undefined) ? NaN : Number(svetlo);
    zobrazSvetlo(Number.isFinite(hodnota) ? hodnota : null, tma === '1');
    return;
  }

  zapis('← ' + radek);
  // Robot hlasi zapnuti a vypnuti sledovani cary (plati i pro starsi program bez dat)
  if (radek.startsWith('Sledovani cary zapnuto')) el('cara').setAttribute('aria-pressed', 'true');
  if (radek.startsWith('Sledovani cary vypnuto')) el('cara').setAttribute('aria-pressed', 'false');
}

function zpracujData(cm, r, cidloL, cidloP) {
  posledniData = performance.now();
  el('bezDat').hidden = true;
  sonar.data(cm, r, cidloL, cidloP);

  el('cm').textContent = (cm < 0) ? 'volno' : cm;
  el('cm').style.color = (cm < 0) ? '' : sonar.barva(cm);

  // Na care robot pred prekazkou stoji, dokud neni dal nez "volno" (stejne jako v robotovi)
  if (r !== 'C') cekam = false;
  else if (!cekam && cm >= 0 && cm <= zony.stop) cekam = true;
  else if (cekam && (cm < 0 || cm >= zony.volno)) cekam = false;

  zobrazRezim(cekam ? 'cekam' : r);
  el('cara').setAttribute('aria-pressed', String(r === 'C'));
}

function zobrazRezim(novy) {
  if (novy === rezim) return;
  // Zacatek couvani: obrazovka se otrese
  if (novy === 'Z') {
    const obrazovka = el('obrazovka');
    obrazovka.classList.remove('otres');
    void obrazovka.offsetWidth;  // Restart animace
    obrazovka.classList.add('otres');
  }
  rezim = novy;
  el('rezim').hidden = !novy;
  el('rezim').dataset.rezim = novy || '';
  el('rezim').textContent = novy ? REZIMY[novy] : '';
}

// =====================================================================
// SVETLA - ukazatel okolniho svetla a nastaveni
// =====================================================================

// Slunicko -> mesicek podle okolniho svetla. Mesic je presne tehdy, kdy je podle robota tma
// (v rezimu "potme" se rozsviti svetla). null = data o svetle nejsou.
function zobrazSvetlo(svetlo, tma) {
  const ukazatel = el('svetloUkazatel');
  if (svetlo === null || svetloZony === null) {
    ukazatel.hidden = true;
    return;
  }
  const { prah, hystereze } = svetloZony;
  let stupen;
  if (tma) stupen = (svetlo < prah - hystereze - 200) ? 4 : 3;
  else if (svetlo > prah + hystereze + 200) stupen = 0;
  else if (svetlo > prah + hystereze) stupen = 1;
  else stupen = 2;

  ukazatel.hidden = false;
  ukazatel.style.setProperty('--hladina', Math.round(svetlo / 1023 * 100) + '%');
  ukazatel.style.setProperty('--prah', Math.round(prah / 1023 * 100) + '%');
  if (ukazatel.dataset.stupen !== String(stupen)) {
    ukazatel.dataset.stupen = stupen;
    ukazatel.setAttribute('aria-label', 'Okolní světlo: ' + STUPNE_SVETLA[stupen]);
    ukazatel.title = 'Okolní světlo: ' + STUPNE_SVETLA[stupen];
  }
  if (svetla.tma !== tma) {
    svetla.tma = tma;
    zobrazNastaveniSvetel();
  }
}

// Tlacitko Svetla a panel podle toho, co hlasi robot
function zobrazNastaveniSvetel() {
  const umi = jePripojeno() && svetla.kdy !== null;
  const tlacitko = el('svetla');
  tlacitko.dataset.efekt = svetla.efekt || '';
  // LED na robotovi sviti: "vzdy", nebo "potme" a je tma
  const sviti = umi && (svetla.kdy === 's' || (svetla.kdy === 'a' && svetla.tma));
  tlacitko.dataset.sviti = sviti ? '1' : '0';
  tlacitko.setAttribute('aria-label', umi
    ? `Světla: ${SVETLA_EFEKTY[svetla.efekt]}, ${SVETLA_KDY[svetla.kdy].toLowerCase()}, ${sviti ? 'svítí' : 'nesvítí'}`
    : 'Světla');

  el('kdySvitit').disabled = !umi;
  el('efekt').disabled = !umi;
  el('svetlaHlaska').textContent = umi ? '' : (jePripojeno() ? SVETLA_HLASKY.neumi : SVETLA_HLASKY.odpojeno);
  for (const volba of document.querySelectorAll('#svetlaPanel input')) {
    volba.checked = umi && (volba.value === svetla.kdy || volba.value === svetla.efekt);
  }
}

// Vytvori prepinace v panelu Svetla (znaky povelu jsou hodnoty prepinacu)
function vytvorVolby(id, volby, efekty) {
  const kontejner = el(id).querySelector('.volby');
  for (const [znak, nazev] of Object.entries(volby)) {
    const volba = document.createElement('label');
    const prepinac = document.createElement('input');
    prepinac.type = 'radio';
    prepinac.name = id;
    prepinac.value = znak;
    prepinac.addEventListener('change', () => posli(znak));  // Robot odpovi radkem "S:"
    const popis = document.createElement('span');
    if (efekty) {
      popis.dataset.efekt = znak;
      popis.innerHTML = '<i class="tecka leva"></i><i class="tecka prava"></i>';
    }
    popis.append(nazev);
    volba.append(prepinac, popis);
    kontejner.append(volba);
  }
}

// =====================================================================
// JIZDA - drzeni smeru
// =====================================================================

// Co je prave drzeno: prst nebo klavesa -> smer. Plati naposledy stisknuty smer;
// kdyz ho pustis a drzis jeste jiny, plati zase ten.
const drzene = new Map();
let aktivniSmer = null;
let casovac = null;

function drz(kdo, smer) {
  if (drzene.get(kdo) === smer) return;
  drzene.delete(kdo);  // Presunout na konec = nejnovejsi
  if (smer) drzene.set(kdo, smer);
  prepocitejSmer();
}

function pust(kdo) {
  if (!drzene.has(kdo)) return;
  drzene.delete(kdo);
  prepocitejSmer();
}

function uvolniVse() {
  drzene.clear();
  prepocitejSmer();
}

function prepocitejSmer() {
  const smer = drzene.size ? [...drzene.values()].pop() : null;
  if (smer === aktivniSmer) return;
  aktivniSmer = smer;

  document.querySelectorAll('[data-smer]').forEach((b) => b.classList.toggle('drzeno', b.dataset.smer === smer));
  clearInterval(casovac);
  casovac = null;

  if (smer) {
    posli(POVELY[smer]);
    casovac = setInterval(() => posli(POVELY[smer]), OPAKOVANI_MS);
    if (navigator.vibrate) navigator.vibrate(15);
    if (jePripojeno()) zapis('→ ' + NAZVY_SMERU[smer]);
  } else if (jePripojeno()) {
    posli(POVELY.stop);
    zapis('→ stop');
  }
}

// Krizovy ovladac: smer podle toho, kam prst ukazuje od stredu krize,
// takze jde prejet prstem z jedne sipky na druhou bez pusteni.
function smerVKrizi(kriz, x, y) {
  const r = kriz.getBoundingClientRect();
  const dx = x - (r.left + r.width / 2);
  const dy = y - (r.top + r.height / 2);
  if (Math.hypot(dx, dy) < r.width * 0.12) return null;  // Stred krize = nic
  if (Math.abs(dx) > Math.abs(dy)) return dx > 0 ? 'doprava' : 'doleva';
  return dy > 0 ? 'dozadu' : 'dopredu';
}

function nastavKriz(kriz) {
  kriz.addEventListener('pointerdown', (e) => {
    kriz.setPointerCapture(e.pointerId);  // Sledovat prst, i kdyz vyjede z krize
    drz('prst' + e.pointerId, smerVKrizi(kriz, e.clientX, e.clientY));
  });
  kriz.addEventListener('pointermove', (e) => {
    if (kriz.hasPointerCapture(e.pointerId)) drz('prst' + e.pointerId, smerVKrizi(kriz, e.clientX, e.clientY));
  });
  const konec = (e) => pust('prst' + e.pointerId);
  kriz.addEventListener('pointerup', konec);
  kriz.addEventListener('pointercancel', konec);
  kriz.addEventListener('lostpointercapture', konec);
}

function prepniCaru() {
  uvolniVse();
  posli(POVELY.cara);
}

// =====================================================================
// VYPIS
// =====================================================================

function zapis(text) {
  const vypis = el('vypis');
  const radek = document.createElement('div');
  radek.textContent = text;
  vypis.appendChild(radek);
  while (vypis.childElementCount > 200) vypis.firstChild.remove();
  vypis.scrollTop = vypis.scrollHeight;
}

function prepniVypis(otevrit) {
  el('vypisPanel').hidden = !otevrit;
  el('vypisTlacitko').setAttribute('aria-expanded', String(otevrit));
  if (otevrit) el('vypis').scrollTop = el('vypis').scrollHeight;
}

// =====================================================================
// START
// =====================================================================

function start() {
  nastavStav(SerialSpojeni.jeDostupne() ? 'odpojeno' : 'nepodporovano');

  el('spojeniTlacitko').addEventListener('click', () => (jePripojeno() ? odpoj() : pripoj(false)));
  el('pripojitRobota').addEventListener('click', () => pripoj(false));
  el('simulace').addEventListener('click', () => pripoj(true));
  el('cara').addEventListener('click', prepniCaru);
  vytvorVolby('kdySvitit', SVETLA_KDY, false);
  vytvorVolby('efekt', SVETLA_EFEKTY, true);
  zobrazNastaveniSvetel();
  el('vypisTlacitko').addEventListener('click', () => prepniVypis(el('vypisPanel').hidden));
  el('vypisZavrit').addEventListener('click', () => prepniVypis(false));
  el('obrazovka').addEventListener('animationend', () => el('obrazovka').classList.remove('otres'));
  nastavKriz(el('kriz'));

  // Klavesnice: sipky / WASD = jizda, C = cara
  window.addEventListener('keydown', (e) => {
    if (e.target.closest && e.target.closest('#svetlaPanel')) return;  // Sipky v panelu prepinaji volby
    if (KLAVESY[e.code]) {
      e.preventDefault();
      if (!e.repeat) drz('klavesa' + e.code, KLAVESY[e.code]);  // Opakovani klavesy ridi casovac, ne system
    } else if (e.code === 'KeyC' && !e.repeat) {
      prepniCaru();
    }
  });
  window.addEventListener('keyup', (e) => pust('klavesa' + e.code));

  // Bezpecnost: kdyz ovladac zmizi z obrazovky (jina appka, zhasnuty displej), robot zastavi
  document.addEventListener('visibilitychange', () => { if (document.hidden) uvolniVse(); });
  window.addEventListener('blur', uvolniVse);
  // Dlouhy stisk na tabletu nema otevirat menu
  document.addEventListener('contextmenu', (e) => e.preventDefault());

  // Ulozit ovladac do zarizeni, aby fungoval i bez internetu (jde jen na zabezpecene strance)
  if ('serviceWorker' in navigator) navigator.serviceWorker.register('sw.js').catch(() => {});
}

start();
