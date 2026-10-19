# Pięć przykładów brzmienia — Jerzy Atomic Cluster 0.1.0

Każdy plik zawiera tę samą syntetyczną frazę szarpanych strun: pojedyncze nuty,
akord i szybszy przebieg. To oryginalny sygnał testowy, nie nagranie prawdziwej
gitary ani próbka oryginalnego pedału EHX. `00_dry` pozwala porównać sygnał suchy.

Audio zostało przepuszczone przez **wydany Windows VST3**, wyodrębniony z
`releases/0.1.0/JerzyAtomicClusterVST3-Win64.zip`, uruchomiony w prostym hoście
offline pod Wine. Nie jest to render niezależnej kopii algorytmu w Pythonie.

| Plik | Charakter | ATOMS | Odświeżanie | BLEND | MODE | Dynamika |
|---|---|---:|---:|---:|---|---|
| 00_dry | suchy sygnał porównawczy | 23 | 27,1 ms | 0% | Sharp | Full |
| 01_single_atom | pojedyncza oscylacja, schodkowy przebieg melodii | 1 | 125 ms | 100% | Sharp | Full |
| 02_rhythmic_glitch | rytmicznie zatrzymywane widmo | 6 | 250 ms | 85% | Sharp | Full |
| 03_smooth_pad | miękkie przejścia i podtrzymane tony | 32 | 600 ms | 100% | Smooth | Full |
| 04_flat_robot | syntetyczne oscylacje z wyrównaną dynamiką | 4 | 70 ms | 100% | Sharp | Flat |
| 05_dense_spectrum | gęstsze widmo i szybkie, płynne zmiany | 256 | 12 ms | 100% | Smooth | Full |

VOL ustawiony na unity we wszystkich przykładach. Nie dodawano pogłosu,
kompresji ani osobnej normalizacji głośności. Różnice poziomów są częścią
działania tych ustawień. Materiał wejściowy ma 9 s; pliki wynikowe mają 12 s,
w tym 3 s na wybrzmienie. Latencja 2048 próbek została skompensowana.

MP3: 192 kb/s do odsłuchu. ZIP zawiera też WAV PCM16 stereo 48 kHz, suche
źródło mono i ustawienia w `manifest.json`.

[Pobierz wszystkie przykłady WAV i MP3](JerzyAtomicCluster-audio-examples.zip)

## Ponowne wygenerowanie

W przygotowanym środowisku:

```sh
/workspace/.atomic-cluster-tools/bin/cmake --build build-win --target render_audio --parallel 4
bash tools/render-examples.sh
python3 tools/package-examples.py
```

Te pliki prezentują brzmienie tej eksperymentalnej wtyczki. Nie stanowią
porównania odsłuchowego z fizycznym EHX Pico Atomic Cluster.
