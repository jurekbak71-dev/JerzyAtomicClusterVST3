# Odsłuch 0.2.0 — głośne atomy, arpeggio, losowanie harmoniczne, stereo

[▶ Całe porównanie — 1 min 41 s](comparison.mp3).
Każdy fragment to ta sama oryginalna syntetyczna fraza szarpana, przepuszczona
przez rzeczywisty Windows VST3 z wydania 0.2.0. VOL = unity, BLEND = 100% wet
we wszystkich przetworzonych próbkach. Bez normalizacji, dodatkowego delayu
lub pogłosu. Sygnał wejściowy ma 9 s, każda próbka zawiera też 3 s ogona.

| Początek | Próbka | ATOMS | Tryb / pattern | Stereo |
|---|---|---:|---|---:|
| 0:00 | [Wejście bez efektu](00_dry.mp3) | — | dry | 0% |
| 0:12,75 | [Jeden atom](01_one_atom.mp3) | 1 | SHARP / CLUSTER | 0% |
| 0:25,50 | [16 atomów](02_sixteen_atoms.mp3) | 16 | SHARP / CLUSTER | 0% |
| 0:38,25 | [64 atomy](03_sixtyfour_atoms.mp3) | 64 | SHARP / CLUSTER | 0% |
| 0:51 | [Rytmiczne arpeggio w stereo](04_arpeggio_stereo.mp3) | 24 | SHARP / ARP | 100% |
| 1:03,75 | [Losowe harmoniczne](05_random_harmonic.mp3) | 48 | SHARP + FLAT / RANDOM | 100% |
| 1:16,50 | [Płynne przejścia w stereo](06_smooth_stereo.mp3) | 48 | SMOOTH / RANDOM | 100% |
| 1:29,25 | [Krążący klaster](07_orbiting_cluster.mp3) | 64 | SMOOTH / CLUSTER | 100% |

[Pobierz wszystkie MP3 w ZIP-ie](JerzyAtomicCluster-0.2.0-examples.zip).
Stereo najlepiej porównać na słuchawkach. Trzy próbki z ATOMS 1/16/64 mają
identyczne pozostałe ustawienia, aby było słychać samą zmianę zagęszczenia.

Dokładne ustawienia, RMS, poziom różnicy L/R i sumy kontrolne DLL oraz plików
znajdują się w `manifest.json`. Odtworzenie:
`bash tools/render-examples-020.sh`. Materiał z filmu użytkownika nie jest
publikowany; nie jest to pomiar zgodności z fizyczną kostką EHX.
