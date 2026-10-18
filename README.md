# LaskaKit LBot: vylepšený program a ovladač se sonarem

Vylepšený řídicí program, opravené příklady lekcí a webový ovladač pro stavebnici robota [LaskaKit LBot](https://www.laskakit.cz/). Všechno je vyzkoušené na skutečném robotovi.

Navazuje na původní repozitář [LaskaKit/LBot](https://github.com/LaskaKit/LBot) s demo programem v1.7 a příklady k manuálu.

**[Otevřít ovladač](https://mpfest.github.io/LBot/ovladac/)** (jde vyzkoušet i bez robota)

## Co je nového

### Demo program v2.0
Složka `Codes/_LBot-default-program`. Soubor `.he_` v té složce je původní předkompilovaný tovární program v1.7 od LaskaKitu, k verzi 2.0 nepatří.

- **Plynulé ovládání.** Program nikde nečeká (žádné `delay()`), takže robot reaguje okamžitě. IR ovladač i Bluetooth fungují stejně: robot jede, dokud držíš tlačítko, a po puštění zastaví.
- **Překážky.** Při jízdě na ovladač robot před překážkou zastaví, varovně pípne a couvne. Při sledování čáry zastaví a počká, až překážka zmizí.
- **Sledování čáry.** Robot zatáčí plynule: nejdřív jemně, a když se čára nevrátí, ostře. Ztracenou čáru chvíli hledá na straně, kde ji viděl naposledy.
- **Světla.** Rozsvítí se sama potmě, nebo svítí vždy, nebo nikdy. Pět efektů: policejní majáky, bílá, **blinkry jako auto** (při zatáčení bliká strana zatáčky, i při sledování čáry), **parkovací** (barva podle vzdálenosti překážky) a duha. Přepínají se z webového ovladače nebo tlačítky na IR ovladači.
- **Nastavení na jednom místě.** Sekce `KONFIGURACE` na začátku programu obsahuje všechna čísla a přepínače pro věci, které se u různých kusů robota liší. Viz [Nastavení pro tvůj robot](#nastavení-pro-tvůj-robot).
- **Aktuální knihovna IRremote 4.x** bez úprav knihovny. Manuál ještě popisuje verzi 2.7.0 s ruční úpravou.
- **Data pro webový ovladač:** vzdálenost, režim, čidla čáry, okolní světlo a nastavení světel. Robot je posílá jen na vyžádání, Serial Monitor zůstává čistý.
- **Oprava skryté chyby,** kvůli které robot mohl přestat vidět překážky (viz další odstavec).

### Skrytá chyba: `pinMode()` na pinu A7
Původní demo a příklad Lekce 5 volaly pro tlačítko `pinMode(A7, INPUT)`. Na Arduinu Nano jsou A6 a A7 jen analogové vstupy a jádro Arduina číslo pinu nekontroluje. Přečte nastavení za koncem své tabulky pinů a podle toho, jak je program zrovna uložený v paměti, může přenastavit úplně jiný pin. U nás to byl spouštěcí pin ultrazvuku D11, takže robot přestal vidět překážky.

S původní knihovnou IRremote 2.7.0 to náhodou neškodilo, ale po jakékoli změně programu se to mohlo projevit. Chybu jsme našli v simulátoru (simavr) a potvrdili na robotovi.

**Na Arduinu Nano nikdy nevolej `pinMode()` na A6 ani A7.** Funkce `analogRead()` žádný `pinMode()` nepotřebuje.

### Webový ovladač
Složka `ovladac/`. Ovladač běží v prohlížeči, nic se neinstaluje z obchodu. Má **sonar**, který ukazuje, co robot vidí před sebou: vzdálenost, zónu, kde zastaví, couvání, režim a čidla čáry. V rohu sonaru je ukazatel okolního světla (sluníčko se mění v měsíček) a tlačítkem **Světla** se nastavují světla robota. Podrobnosti v sekci [Webový ovladač](#webový-ovladač-1).

### Opravené příklady lekcí
Složka `Codes/`. Příklady dál odpovídají manuálu, opravené jsou jen skutečné chyby. Tři z dvanácti příkladů původně nešly zkompilovat.

| Lekce | Příklad | Oprava |
|---|---|---|
| 5 Tlačítko | `tlacitko` | Nešel zkompilovat (chyběl pin bzučáku). Odstraněn `pinMode(A7)`. |
| 6 IR přijímač | `IR_prijimac` | Knihovna IRremote 4.x. |
| 7 IR vysílač | `IR_vysilac` | Knihovna IRremote 4.x. Tip, jak robotem vypnout televizi. |
| 8 Motory | `motory` | Nešel zkompilovat ani s IRremote 2.7.0. Knihovna IRremote 4.x, robot zastaví po puštění tlačítka. |
| 9 Ultrazvuk | `ultrazvuk` | Nešel zkompilovat (`distance` místo `vzdalenost`). |
| 10 Sledování čáry | `sledovani_cary` | Přepínače pro logiku čidla a způsob jízdy. Výpis do Serial Monitoru jen při změně. |
| 11 Bluetooth | `bluetooth` | Pojistka: robot zastaví, když z aplikace dlouho nepřijde povel. |

## Webový ovladač
**[mpfest.github.io/LBot/ovladac](https://mpfest.github.io/LBot/ovladac/)**

Funguje v **Chrome** nebo **Edge** na Androidu, Windows, Macu a Linuxu. Na iPhonu a iPadu ne: iOS nedovolí aplikacím mluvit s klasickým Bluetooth modulem HC-06.

**Připojení**
1. Nahraj do robota demo program v2.0 a zapni ho.
2. V nastavení Bluetooth tabletu nebo počítače spáruj **HC-06** (PIN `1234`).
3. Otevři ovladač, klepni na **Připojit robota** a vyber HC-06. Na Macu vyber `cu.HC-06`.
4. Drž šipku a robot jede, pusť ji a zastaví. Přes kříž jde přejet prstem z jednoho směru na druhý. Tlačítko **Čára** zapíná a vypíná sledování čáry. Na počítači fungují i šipky nebo WASD a klávesa C.
5. Tlačítko **Světla** otevře nastavení: kdy svítit (potmě, vždy, nikdy) a jaký efekt. Ukazatel v pravém dolním rohu sonaru ukazuje okolní světlo. Když se změní na měsíček, je pro robota tma a v režimu „potmě“ rozsvítí světla.

**Instalace:** v menu Chrome zvol **Instalovat aplikaci**. Ovladač se pak spouští z plochy přes celou obrazovku a funguje i bez internetu.

**Bez robota:** tlačítko **Vyzkoušet bez robota** spustí napodobeného robota, ať si sonar můžeš prohlédnout.

**Bezpečnost:** když ovladač zmizí z obrazovky (přepnutí aplikace, zhasnutý displej) nebo se spojení přeruší, robot zastaví.

Ovladač jezdí i s jinými programy, které rozumí stejným povelům (třeba Lekce 11). Sonar a nastavení světel ale potřebují demo v2.0.

### Povely pro vlastní aplikaci
Robot poslouchá jednotlivé znaky na sériové lince (Bluetooth i USB, 9600 Bd). Povel k jízdě platí 500 ms (`BT_TIMEOUT_MS`), aplikace ho proto musí při držení tlačítka posílat opakovaně, aspoň třikrát za sekundu.

| Znak | Význam |
|---|---|
| `u` `d` `l` `r` | jízda dopředu, dozadu, doleva, doprava |
| `o` | stop (puštění tlačítka) |
| `2` | sledování čáry zapnout / vypnout |
| `a` `s` `n` | světla: rozsvítit potmě (automaticky), svítit vždy, nesvítit |
| `p` `b` `k` `g` `h` | efekt světel: policie, bílá, blinkry, parkovací, duha |
| `v` | pošli data pro sonar (posílá je 1,5 s po posledním `v`) |

Na `v` robot odpoví dvěma řádky:
- `K:stop,volno,max,práh,hystereze`: vzdálenosti zón v cm a kdy je tma (hodnota čidla světla);
- `S:kdy,efekt`: nastavení světel stejnými písmeny jako povely, například `S:a,p`. Posílá ho znovu po každé změně, i když světla přepneš IR ovladačem.

Pak po každém měření posílá `T:vzdálenost,režim,čidla,světlo,tma`, například `T:23,R,01,512,0`:
- vzdálenost `-1` znamená, že před robotem nic není;
- režim `R` je jízda na ovladač, `C` sledování čáry a `Z` couvání od překážky;
- čidla jsou levé a pravé, `1` znamená, že čidlo vidí čáru;
- světlo je hodnota čidla okolního světla (0 až 1023, menší = tmavší), tma `1` znamená, že je pro robota tma.

## Než začneš

### Napájení
Výchozí rychlosti jsou nastavené pro **6 nabíjecích AA článků NiMH (7,2 V)**.

- **4× AA alkalické** (s nimi počítá manuál): nové mají kolem 6 V a fungují, jak se ale vybíjejí, napětí klesá. Pak začne vypadávat Bluetooth a robot jede pomaleji.
- **4× AA NiMH** (4,8 V) je pro Arduino málo. Bluetooth vypadává hned.
- **6× AA alkalické** (9 V) fungují, robot ale jede zhruba o čtvrtinu rychleji. Rychlosti v `KONFIGURACI` můžeš snížit.

Napájení se připojuje kruhovým konektorem 5,5/2,1 mm.

### Nahrávání programu
- **Bluetooth modul HC-06 při nahrávání vytáhni.** Sdílí s USB sériovou linku a nahrávání by selhalo.
- Knihovny z Library Manageru v Arduino IDE: **IRremote** (4.x), **Adafruit NeoPixel**, **HCSR04** (Martin Sosic). Knihovnu IRremote už není potřeba upravovat.
- Deska **Arduino Nano**, procesor **ATmega328P**. Když nahrávání selže, zkus **ATmega328P (Old Bootloader)**.
- Serial Monitor: příklady lekcí používají **115200** Bd, demo program a Lekce 11 **9600** Bd.
- Mac s procesorem Apple: překladač pro Arduino potřebuje Rosettu. Když kompilace hlásí `bad CPU type in executable`, nainstaluj ji v Terminálu příkazem `softwareupdate --install-rosetta`.

### Čára
- Použij **širokou (asi 3 cm) matnou černou pásku.** Tenkou čáru čidla z výšky, ve které robot jezdí, nevidí, a lesklá páska odráží světlo. Nejjednodušší je **černá papírová maskovací páska**: je matná, dobře se trhá i lepí do zatáček a jde z podlahy sundat.
- Robot startuje **na čáře** (výchozí `JIZDA_PO_CARE = true`).

## Nastavení pro tvůj robot
Všechno je v sekci `KONFIGURACE` na začátku demo programu. Výchozí hodnoty odpovídají našemu robotovi.

| Nastavení | Co dělá | Jak zjistit |
|---|---|---|
| `MOTOR_L_OBRACENY`, `MOTOR_R_OBRACENY` | směr otáčení motorů | Jeď dopředu. Když se některé kolo točí dozadu, přepni jeho hodnotu na `true`. |
| `CIDLO_CARY_SVETLO_JE_HIGH` | logika čidel čáry | Podívej se na LED na čidle. Svítí nad světlou podlahou a nad čárou zhasne: `true`. Je to obráceně: `false`. |
| `JIZDA_PO_CARE` | způsob sledování čáry | `true`: široká čára, obě čidla jsou nad ní. `false`: tenká čára vede mezi čidly. |
| `KOREKCE_L`, `KOREKCE_R` | vyrovnání motorů | Když robot při jízdě rovně táhne doprava, přidej pár procent pravému motoru, když doleva, levému. Náš robot má při 7,2 V obě nuly, se 4,8 V potřeboval pravý motor 5 %. |
| `RYCHLOST_JIZDA`, `RYCHLOST_COUVANI`, `CARA_RYCHLOST_*` | rychlosti (0 až 255) | Nastavené pro 7,2 V. Se slabší baterií je zvyš. |
| `PREKAZKA_STOP_CM`, `PREKAZKA_VOLNO_CM` | kdy zastavit a kdy je zase volno | Výchozí 10 a 13 cm. |
| `SVETLO_PRAH` | kdy je tma | Hodnotu čidla v různém světle ukáže příklad Lekce 3. |
| `SVETLA_KDY`, `SVETLA_EFEKT` | světla po zapnutí robota | Kdy svítit (`SVETLA_POTME`, `SVETLA_VZDY`, `SVETLA_NIKDY`) a efekt (`EFEKT_POLICIE`, `EFEKT_BILA`, `EFEKT_BLINKRY`, `EFEKT_PARKOVACI`, `EFEKT_DUHA`). Za jízdy jde obojí změnit z ovladače. |
| `IR_DOPREDU` a další `IR_*` | tlačítka IR ovladače | Kódy tlačítek vypisuje příklad Lekce 6 jako `Command`. Neznámá tlačítka vypisuje do Serial Monitoru i demo. |

## Ovládání dema

| | IR ovladač | Bluetooth | Webový ovladač |
|---|---|---|---|
| Dopředu | `>>\|` | `u` | ▲, šipka nahoru, W |
| Dozadu | `100+` | `d` | ▼, šipka dolů, S |
| Doleva | `-` | `l` | ◀, šipka doleva, A |
| Doprava | `EQ` | `r` | ▶, šipka doprava, D |
| Sledování čáry zap/vyp | `0` | `2` | Čára, C |
| Efekt světel | `1` policie, `2` bílá, `3` blinkry, `4` parkovací, `5` duha | `p` `b` `k` `g` `h` | Světla |
| Kdy svítit | `7` potmě, `8` vždy, `9` nikdy | `a` `s` `n` | Světla |

Sledování čáry zapíná a vypíná i tlačítko na základní desce. Změnu světel robot potvrdí pípnutím. Ve dne se v režimu „potmě“ efekty neukážou, na vyzkoušení přepni na „vždy“.

## Autor a licence
Úpravy programu, opravy příkladů a webový ovladač: **Martin Petřík ([MPFest](https://github.com/MPFest))**, 2026.

Stavebnice, manuál (`LasKKit LBot.pdf`), schéma a původní program v1.7: [LaskaKit](https://www.laskakit.cz/).

Kód je pod licencí MIT, hardware pod licencí CC BY-SA 4.0. Podrobnosti jsou v souboru [LICENSE](LICENSE).

## O stavebnici (původní popis od LaskaKitu)

![LBot](img/main.jpg)

LasKKit LBot je stavebnice programovatelného robota určená nejen pro děti, ale pro všechny, kteří s programováním podobných zařízení začínají. Díky jeho jednoduché konstrukci, volbě použitých dílů a našeho návodu je jeho sestavení hračkou a zvládnou ho i děti. Je postaven na velmi oblíbené platformě Arduino, konkrétně Arduino NANO s mikrokontrolerem Atmega328P. Software se vyvíjí stejně jako pro ostatní Arduina v programu Arduino IDE. Pro programování je tedy k dispozici nepřeberné množství knihoven, kódů, příkladů a informací, že programování bude hračka a zábava. Všechny základní příklady pro snadný začátek s programováním máme také zpracovány v našem návodu.

V dodávané sadě může LBot jezdit podle trasy připravené pomocí čáry, kterou sleduje dvojicí optických čidel, vyhýbat se překážkám díky ultrazvukovému čidlu měřícímu vzdálenost od překážky, blikat, svítit dvěma tříbarevnými LED diodami, reagovat na intenzitu okolního světla a dát o sobě vědět i zvukově pomocí piezo bzučáku.

LBot se může ovládat pomocí infračerveného dálkového ovladače, nebo z aplikace mobilního telefonu přes Bluetooth modul. LBot obsahuje i infračervenou vysílací LED diodu, kterou lze využít pro dálkové ovládání jiných zařízení. Na základní desce i jedno tlačítko využitelné programově pro nejrůznější funkce, např. přepínání různých režimů.

Jako klíčová vlastnost robota je přítomnost čtyř portů, do kterých se dá připojit velké množství různých přídavných modulů a zařízení. Další je podvozek vyrobený z hliníkového plechu a kromě toho, že je velmi robustní, umožňuje díky rozličným otvorům a závitům montáž různých konstrukcí a zařízení. Lze tak postavit jakoukoli konstrukci, která tě napadne!

Napájení robota je zajištěno čtveřicí AA baterií v praktickém držáku se standardním kruhovým napájecím konektorem 5,5/2,1mm. Lze tedy robota napájet z různých zdrojů.

LBot je dodáván s naším demo softwarem, díky kterému můžeš hned po sestavení jezdit pomocí infračerveného dálkového ovládání, nebo z mobilního telefonu přes Bluetooth a nikdy nenarazíš do žádné překážky, protože software sleduje ultrazvukovým čidlem překážky před sebou, v bezpečné vzdálenosti před překážkou zastaví a pro snadnější objetí překážky i kousek couvne. Další funkcí v tomto základním režimu je, že pokud klesne intenzita okolního osvětlení, rozsvítí obě tříbarevné LED diody bílou barvou a pokud zase intenzita stoupne, automaticky je zhasne.

Tlačítkem na základní desce, nebo tlačítkem v aplikaci na mobilním telefonu/tabletu lze robota přepnout do režimu sledování čáry a LBot začne jezdit tvoji připravenou trasu. Opakovaným stiskem tlačítka na základní desce, nebo tlačítka v aplikaci na mobilním telefonu/tabletu se režim sledování čáry ukončí a LBot se vrátí do základního režimu. Stejnou funkci má i tlačítko 0 na infračerveném dálkovém ovladači.

### Sleduje čáru
![Sleduje čáru](img/cara.jpg)

### Vyhýbá se překážkám
![Vyhýbá se překážkám](img/ultrazvuk.jpg)

### Ovládá se infračerveným dálkovým ovladačem
![Ovládá se infračerveným dálkovým ovladačem](img/ovladac.jpg)
