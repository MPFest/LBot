// LBot ovladac - service worker: ulozi ovladac do zarizeni, aby fungoval i bez internetu.
// S internetem se vzdy nacte nejnovejsi verze (a ulozi se), bez internetu ta ulozena.

const CACHE = 'lbot-ovladac-v2';  // Pri zmene seznamu souboru zvys cislo
const SOUBORY = [
  './',
  'index.html',
  'styl.css',
  'spojeni.js',
  'sonar.js',
  'ovladac.js',
  'manifest.webmanifest',
  'ikony/ikona.svg',
  'ikony/ikona-192.png',
  'ikony/ikona-512.png',
  'ikony/ikona-maskable-512.png',
];

// Instalace: stahnout a ulozit vsechny soubory ovladace
self.addEventListener('install', (udalost) => {
  udalost.waitUntil(caches.open(CACHE).then((cache) => cache.addAll(SOUBORY)));
  self.skipWaiting();
});

// Aktivace: smazat ulozene soubory starsich verzi
self.addEventListener('activate', (udalost) => {
  udalost.waitUntil(
    caches.keys().then((nazvy) => Promise.all(nazvy.filter((n) => n !== CACHE).map((n) => caches.delete(n))))
  );
  self.clients.claim();
});

// Nacitani: nejdriv z internetu, kdyz nejde, z ulozene kopie
self.addEventListener('fetch', (udalost) => {
  if (udalost.request.method !== 'GET') return;
  udalost.respondWith(
    fetch(udalost.request)
      .then((odpoved) => {
        if (odpoved.ok) {
          const kopie = odpoved.clone();
          caches.open(CACHE).then((cache) => cache.put(udalost.request, kopie));
        }
        return odpoved;
      })
      .catch(() => caches.match(udalost.request, { ignoreSearch: true }))
  );
});
