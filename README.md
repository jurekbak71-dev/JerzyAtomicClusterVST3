# Jerzy Atomic Cluster — VST3 0.2.1

**LV2 dla Raspberry Pi 4B / MODEP / Zynthian:**
[Pobierz ARM64 + ARMHF](releases/lv2/0.2.1/JerzyAtomicCluster-LV2-RPi4B.zip) ·
[Instrukcja instalacji](docs/LV2_RPI4_PL.md) · [Testy LV2/ARM](docs/LV2_VALIDATION.md).


Efekt dla Windows x64: harmoniczne atomy, rytmiczne arpeggio, losowe wzory
i niezależny ruch głosów w stereo. Własny algorytm inspirowany Pico Atomic
Cluster, rozwinięty według uwag odsłuchowych użytkownika.

**[Pobierz Windows VST3 0.2.1](releases/0.2.1/JerzyAtomicClusterVST3-Win64.zip)**

**[▶ Test stereo i BPM: mono-in → stereo-out, 90/120 BPM](audio-examples/0.2.1/README.md)**

W 0.2.1 poprawiono mono-in → stereo-out oraz wzmocniono słyszalny ruch
panoramy. Widoczny przycisk **BPM SYNC** synchronizuje SPEED z tempem i siatką
muzyczną hosta. W trybie sync gałka SPEED wybiera 1/16, 1/8, 1/4, 1/2 lub takt.

Od 0.2.0 wet pochodzi wyłącznie z banku oscylatorów. Zwiększenie ATOMS dodaje
głosy i harmoniczne, zamiast przywracać oryginalne widmo. Ich poziom jest
wyrównywany, aby gęsty efekt nie zanikał. BLEND = 100% wyklucza tor dry.
SHARP nadaje atomom puls, SMOOTH łagodnie zmienia ich poziomy i wysokości.

[Archiwalne próbki 0.1.2](audio-examples/0.1.2/README.md) ·
[Analiza filmu referencyjnego](docs/reference-analysis/video/VIDEO_FINDINGS.md)

## Instalacja w Windows / FL Studio

1. Rozpakuj `JerzyAtomicClusterVST3-Win64.zip`.
2. Skopiuj **cały folder** `JerzyAtomicCluster.vst3` do
   `C:\Program Files\Common Files\VST3`.
3. W FL Studio otwórz Plugin Manager i wykonaj ponowne wyszukiwanie wtyczek.
4. Wstaw **Jerzy Atomic Cluster** jako efekt na mikserze.

To Windows x64 VST3, nie plik VST2 `.dll`. Folder `.vst3` jest kompletną wtyczką;
zachowaj znajdujące się w nim podfoldery `Contents`.

## Obsługa

- **VOL** — poziom wyjściowy; środek = unity, maksimum około +6 dB.
- **BLEND** — 0% = wyłącznie dry, 100% = wyłącznie atomy (wet). Domyślnie 100%.
- **SPEED** — w FREE: szybkość odświeżania; w BPM SYNC: podział nutowy.
- **FREE / BPM SYNC** — przycisk po lewej stronie diody. Włącza synchronizację
  do tempa hosta i jego pozycji muzycznej podczas odtwarzania. Przycisk po prawej
  wybiera podział: 1/16, 1/8, 1/4, 1/2, 1 bar. Nie trzeba otwierać menu.
- **ATOMS** — 1–64 głosy. Więcej atomów zagęszcza harmoniczne, również przy
  prostym wejściu z niewielką liczbą składowych.
- **CLUSTER / ARP / RANDOM** — widoczne przyciski pod gałkami. CLUSTER łączy
  rozpoznane składowe z dodatkowymi harmonicznymi. ARP prowadzi rytmiczną
  sekwencję oktaw, kwint i tercji. RANDOM losuje harmonicznie powiązane głosy.
- **STEREO ON/OFF** — osobny przełącznik pod gałkami; obok przeciągany suwak
  WIDTH 0–100%. Domyślnie 100%. Skoordynowane orbity głosów dają wyraźny ruch
  całego klastra przy zachowaniu różnych pozycji poszczególnych atomów.
  Na ścieżce z sygnałem mono wybierz wariant mono-in → stereo-out.
- **MODE** — kliknięcie przełącza SHARP / SMOOTH. Przytrzymanie około sekundy
  przełącza pełną / wyrównaną dynamikę. SHARP = puls, SMOOTH = płynność.
- **Footswitch** — bypass; dwa kliknięcia ustawiają również tap tempo.
- Środkowy przycisk myszy — tap tempo bez przełączania bypassu.
- Prawy przycisk — dynamika, włączenie tap tempo, synchronizacja do tempa hosta
  i podział rytmiczny, wzór atomów oraz szerokość stereo. W BPM SYNC obrót SPEED zmienia podział, zachowując synchronizację.
- Gałki: przeciąganie w pionie; Shift pozwala regulować dokładniej.

Wszystkie parametry, również dynamika i ustawienia tap/sync, zapisują się w projekcie.
Starsze projekty (format 0.1.x) są odczytywane: ATOMS zachowuje liczbę głosów
do 64, a ruch stereo pozostaje wyłączony. Nowy stan zapisuje format 2 z 12 parametrami.

## Zakres i ograniczenia

Silnik analizuje okna 2048 próbek co 256 próbek i steruje bankiem 64 oscylatorów.
Rozpoznane lokalne maksima wyznaczają składowe i harmoniczną bazę. CLUSTER dodaje
harmoniczne ponad to, co rozpoznano w wejściu; ARP/RANDOM tworzą muzyczne wzory.
Poziom głosów jest wyrównywany energetycznie; łagodna saturacja ogranicza szczyty
samego wet. SHARP ma krótką obwiednię rytmiczną, SMOOTH płynnie zmienia amplitudy
i częstotliwości. Nie jest to przezroczysta rekonstrukcja widma.

SPEED żąda kroku od około 2,67 ms do 2 s, z rozdzielczością 256 próbek.
Przy bardzo szybkim SPEED impulsy zlewają się w barwę. Zakresy i arpeggiator są
wyborem tej implementacji; nie pochodzą z pomiaru oryginalnego urządzenia.

Mono-in → stereo-out, mono-in → mono-out i stereo-in → stereo-out; obliczenia wejścia/wyjścia 32/64-bit, sample rate 8–384 kHz.
Oryginalna kostka jest mono; stereo, ARP/RANDOM i host sync są rozszerzeniami tej wtyczki.
Na wyjściu mono host otrzymuje sumę L/R; do ruchu stereo potrzebne jest wyjście stereo.
Host przekazuje BPM i pozycję muzyczną. Gdy przekazuje tylko BPM, działa zegar
rytmiczny bez siatki pozycji; bez tempa używane jest 120 BPM. Podczas odtwarzania
siatka uwzględnia zgłoszoną latencję, zmianę tempa i skoki pozycji.
Zgłaszana latencja wynosi 2048 próbek, czyli około 42,7 ms przy 48 kHz.
Host może kompensować ją w projekcie, ale jest słyszalna przy grze na żywo.

Brzmienie wymaga porównania z fizyczną kostką. Status testów konkretnego wydania
znajduje się w `docs/VALIDATION.md`.

## Budowanie

Wymagane: CMake >=3.25, C++17, Ninja lub Visual Studio 2022 oraz oficjalny SDK
Steinberga 3.8.1, commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96` wraz z submodułami.

```sh
git clone https://github.com/steinbergmedia/vst3sdk.git dependencies/vst3sdk
git -C dependencies/vst3sdk checkout 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
git -C dependencies/vst3sdk submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVST3_SDK_ROOT=dependencies/vst3sdk
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Na Windows uruchom w terminalu deweloperskim MSVC. Native build uruchamia
walidator SDK po kompilacji. Linux build udostępnia parametry w edytorze hosta;
graficzny edytor kostki jest implementowany dla Windows.

Cross-kompilacja w przygotowanym środowisku:

```sh
export PATH=/workspace/.atomic-cluster-tools/bin:$PATH
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/mingw.cmake
cmake --build build-win --parallel 4
python3 tools/package.py --build build-win
```

SDK pozostaje niezmieniony. W buildzie MinGW pomijany jest nieużywany pomocnik
DataExchange SDK, który wymaga niedostępnego `std::aligned_alloc` w MSVCRT.
Testy i asercje nie są wyłączane. Windows CI używa MSVC i pełnego SDK.

Budowanie/testowanie odbywa się w istniejącym checkoutcie, bez tworzenia worktree.
Workflow `.github/workflows/windows.yml` automatycznie buduje, testuje i pakuje
wersję Windows po pushu. Artefakt ZIP ma nazwę `JerzyAtomicClusterVST3-Win64`.
