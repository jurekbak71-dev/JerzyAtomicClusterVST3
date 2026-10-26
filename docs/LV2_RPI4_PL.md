# Jerzy Atomic Cluster LV2 — Raspberry Pi 4B / MODEP / Zynthian

Paczka zawiera gotowe biblioteki **ARMHF 32-bit** i **AArch64 64-bit**, ten sam
silnik brzmieniowy co VST3 0.2.1, panel MODEP i trzy presety. Nie wymaga SDK
Steinberga, Wine, serwera graficznego ani zewnętrznych bibliotek C++.

## Instalacja gotowej paczki

Pobierz `JerzyAtomicCluster-LV2-RPi4B.zip` na Raspberry Pi, rozpakuj i wejdź do
folderu `JerzyAtomicCluster-LV2-RPi4B`.

**MODEP / Patchbox OS:**

```sh
sudo bash install.sh modep
```

Folder trafia do `/usr/modep/lv2/JerzyAtomicCluster.lv2`. Przeskanuj wtyczki albo
uruchom ponownie host MODEP, a następnie odśwież stronę pedalboardu.
Na standardowej instalacji usługa interfejsu nazywa się `modep-mod-ui`.
Instalator sam nie restartuje usług i nie zmienia połączeń audio.

**Zynthian:**

```sh
sudo bash install.sh zynthian
```

Folder trafia do `/usr/local/lib/lv2/JerzyAtomicCluster.lv2`, standardowego
katalogu `LV2_PATH` Zynthiana. W Webconf uruchom wyszukiwanie wtyczek LV2,
włącz **Jerzy Atomic Cluster** i dodaj go do łańcucha jako efekt audio.
Zynthian używa własnego interfejsu parametrów; nie wymaga okna Windows.

**Dowolny inny host LV2:** `bash install.sh user` instaluje do `~/.lv2`.
Własny katalog: `bash install.sh user --prefix /własna/ścieżka/lv2`.
`--dry-run` pokazuje wariant i cel bez zapisywania plików.

Instalator rozpoznaje architekturę **systemu użytkownika**. Pi 4 z kernelem
64-bitowym i systemem 32-bitowym dostaje ARMHF. Przy aktualizacji poprzedni
bundle jest zachowywany w sąsiednim `.atomic-cluster-backups`, poza katalogiem
skanowania LV2. Ustawienia pedalboardu/łańcucha przechowuje host.

## Połączenia stereo

Podłącz gitarę do **Input L / Mono**. Jeżeli Input R pozostaje odłączony
(NULL w API LV2), wtyczka powiela wejście mono do analizy. Niektóre hosty
przekazują odłączony port jako bufor zer; dla przewidywalnego poziomu można
wtedy rozdzielić ten sam sygnał mono na **oba wejścia**.
Podłącz **Output L i Output R** do dwóch kanałów wyjściowych. Jedno wyjście
albo zsumowanie L/R do mono usuwa słyszalne przemieszczanie panoramy.
**Stereo width = 100%**, **Blend = 100%** daje same atomy krążące L/R.

## Parametry

- **Volume**: -60 do +6 dB, domyślnie 0 dB.
- **Blend**: 0–100%; 100% = wyłącznie atomy, bez domieszki dry.
- **Speed (Free)**: około 2,67–2000 ms; działa przy BPM Sync wyłączonym.
- **Atoms**: 1–64 harmoniczne głosy; więcej głosów zagęszcza efekt.
- **Sharp / Smooth**: 0 = rytmiczny puls, 1 = płynne przejścia.
- **Flat dynamics**: wyrównanie poziomów składowych.
- **Enabled**: 1 = efekt, 0 = dry z zachowaną zgłaszaną latencją.
- **BPM Sync**: włącza zegar rytmiczny.
- **Sync division**: 0 = 1/16, 1 = 1/8, 2 = 1/4, 3 = 1/2, 4 = takt (4 ćwierćnuty).
- **Atom pattern**: 0 = Cluster, 1 = Arpeggio, 2 = Random harmonic.
- **Stereo width**: 0–100%; domyślnie 100%.
- **BPM / fallback**: 20–400 BPM; używane, gdy host nie przekazuje tempa.
- **Tap BPM (Sync)**: dwa impulsy przy włączonym BPM Sync ustawiają tempo
  awaryjnego zegara. Tempo dostarczane przez host ma pierwszeństwo.

MODEP może przypisywać kontrolki do MIDI/footswitchy. Standardowe wartości
portów zapisują się w pedalboardzie MODEP lub snapshotach Zynthiana.

## Synchronizacja tempa

Host wysyła standardowe atomy LV2 `time:Position` z `beatsPerMinute`, `speed`
oraz `beat` lub `bar` + `barBeat`. Wtedy silnik śledzi tempo i siatkę muzyczną,
z uwzględnieniem zgłaszanej latencji. Obsługuje też zmiany w środku bloku audio.
Gdy host przekazuje tylko tempo, działa wewnętrzny zegar BPM. Gdy nie przekazuje
tempa, ustaw kontrolkę BPM. Samo włączenie BPM Sync nie uruchamia transportu
MODEP/Zynthiana — to ustawienie hosta.

## Zakres testów i praca na Pi

- Zbudowano binaria ARMHF i ARM64 dla Cortex-A72 (RPi 4B).
- Rzeczywiste biblioteki LV2 uruchomiono pod QEMU ARM; nie jest to test na fizycznym Pi.
- Przeszły testy DSP i testy mini-hosta LV2: dry/wet, stereo, latencja,
  in-place, opcjonalny prawy kanał, BPM i zdarzenia o określonym numerze próbki.
- Lilv rozpoznaje wtyczkę, 19 portów i presety; panel MOD ma złącza audio
  oraz kontrolki. Nie uruchomiono pełnej sesji MODEP ani Zynthiana na sprzęcie.
- Latencja: **2048 próbek**, około **42,7 ms przy 48 kHz**. To odczuwalne
  przy grze na żywo. Host może dodatkowo wprowadzać własną latencję.
- Zacznij od **16 atomów, 48 kHz, bufor 256 próbek**. Podnoś liczbę atomów
  po sprawdzeniu obciążenia i xrunów. Nie deklarujemy zmierzonego limitu CPU
  ani bezxrunowej pracy na fizycznym RPi 4B.
- Binaria korzystają z bazowego ABI glibc ARMHF 2.4 / AArch64 2.17 i bibliotek
  systemowych libc/libm (oraz loadera ARMHF). Nie wymagają glibc 2.29 z nowego
  ABI funkcji exp/log/pow. Uruchomienie testów wykonano z sysroot glibc 2.31.

## Budowanie natywnie na Raspberry Pi

Potrzebne: C++17, CMake >=3.25, Ninja, nagłówki `lv2-dev`.

```sh
cmake -S . -B build-lv2 -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DATOMIC_BUILD_PLUGIN=OFF -DATOMIC_BUILD_LV2=ON
cmake --build build-lv2 --parallel 2
ctest --test-dir build-lv2 --output-on-failure --no-tests=error
```

Gotowy bundle jest w `build-lv2/lv2/JerzyAtomicCluster.lv2`. Zawiera bibliotekę
`.so`, pliki `.ttl` i katalog `modgui` — kopiuj **cały folder**.
Starsze obrazy mogą wymagać nowszego CMake do budowania; instalacja gotowej
paczki nie wymaga CMake. Reprodukcja cross-buildów i testów w środowisku
chmurowym: `bash tools/setup-lv2-cloud.sh`.
