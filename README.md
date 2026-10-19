# Jerzy Atomic Cluster — VST3 0.1.2

Eksperymentalny dekompozytor widma dla Windows x64, inspirowany zachowaniem
EHX Pico Atomic Cluster opisanym w instrukcji producenta. Własny algorytm DSP;
nie jest to oficjalna wtyczka EHX ani potwierdzona kopia brzmienia kostki.

[Pobierz Windows VST3 0.1.2](releases/0.1.1/JerzyAtomicClusterVST3-Win64.zip)

[Nowe próbki 0.1.2 — pięć ustawień pod film](audio-examples/0.1.2/README.md)

[Archiwalne próbki 0.1.0](audio-examples/README.md)

[Analiza materiału referencyjnego i poprawka ATOMS](docs/reference-analysis/REPORT.md)
— w 0.1.1 poprawiono selekcję składowych. W 0.1.2 SMOOTH ma dłuższe przejścia,
a wygaszane tony zachowują wysokość po kolejnych pobraniach widma.

[Analiza filmu: ustawienia gałek, tryby i pozostały tor efektów](docs/reference-analysis/video/VIDEO_FINDINGS.md).

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
- **BLEND** — proporcja sygnału dry/wet; oba tory są wyrównane w czasie.
- **SPEED** — szybkość aktualizacji widma; w prawo = szybciej.
- **ATOMS** — liczba zachowanych składowych widma; w lewo = bardziej syntetycznie.
- **MODE** — kliknięcie przełącza SHARP / SMOOTH. Przytrzymanie około sekundy
  przełącza pełną / wyrównaną dynamikę.
- **Footswitch** — bypass; dwa kliknięcia ustawiają również tap tempo.
- Środkowy przycisk myszy — tap tempo bez przełączania bypassu.
- Prawy przycisk — dynamika, włączenie tap tempo, synchronizacja do tempa hosta
  i podział rytmiczny. Obrót SPEED wyłącza synchronizację do hosta.
- Gałki: przeciąganie w pionie; Shift pozwala regulować dokładniej.

Wszystkie parametry, również dynamika i ustawienia tap/sync, zapisują się w projekcie.
Po niskim ustawieniu SPEED aktualizacja efektu może nastąpić dopiero w kolejnym kroku.

## Zakres i ograniczenia

Silnik analizuje okna 2048 próbek, wybiera najsilniejsze lokalne maksima widma
i resyntezuje je z podtrzymaniem fazy oraz nakładaniem okien. Sąsiednie biny okna
są grupowane z maksimum i nie zużywają kolejnych atomów; ich fazy są powiązane.
ATOMS określa maksymalną liczbę wybieranych maksimów (1–1023), ograniczoną
liczbą rozpoznawalnych składowych i rozdzielczością FFT. Pełne ATOMS z normalną
dynamiką zachowuje całe widmo. SPEED żąda odświeżania od około 2,67 ms do 2 s (z rozdzielczością
kroku 256 próbek). **To przyjęte zakresy tej implementacji**: instrukcja EHX
nie podaje oryginalnych zakresów ani wewnętrznego algorytmu.

Mono i stereo, obliczenia wejścia/wyjścia 32/64-bit, sample rate 8–384 kHz.
Oryginalna kostka jest mono; stereo i host sync są rozszerzeniami tej wtyczki.
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
