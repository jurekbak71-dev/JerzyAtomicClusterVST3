# Próbki 0.1.2 — ustawienia inspirowane filmem

Każda próbka: ta sama oryginalna, syntetyczna fraza szarpana (9 s) oraz 3 s
ogona. Rzeczywisty Windows VST3 z ZIP-a wydania 0.1.2, uruchomiony pod Wine.
Bez zewnętrznych efektów, limitera i normalizacji. VOL = unity. To propozycje
brzmieniowe do odsłuchu, nie odtworzenie dokładnych ustawień urządzenia.

| Próbka | Tryb | ATOMS | Odświeżanie | BLEND |
|---|---|---:|---:|---:|
| [00 — sygnał wejściowy](00_dry.mp3) | dry | — | — | 0% |
| [01 — rzadkie, skokowe tony](01_sharp_sparse.mp3) | SHARP | 4 | 160 ms | 100% |
| [02 — długie podtrzymanie](02_smooth_hold.mp3) | SMOOTH | 6 | 900 ms | 100% |
| [03 — płynne akordy](03_smooth_chords.mp3) | SMOOTH | 18 | 450 ms | 85% |
| [04 — rytmiczny robot](04_flat_glitch.mp3) | SHARP + FLAT | 3 | 300 ms | 100% |
| [05 — szybki, gęsty klaster](05_fast_cluster.mp3) | SMOOTH | 48 | 28 ms | 90% |

[Wszystkie próbki MP3 w jednym ZIP-ie](JerzyAtomicCluster-0.1.2-examples.zip).
Przy wolnym odświeżaniu SMOOTH stopniowo przenika pomiędzy przechwyconymi
widmami; w SHARP zmiany są wyraźne i rytmiczne. Do pierwszych eksperymentów
z gitarą polecam ustawienia 01 i 03.

Film referencyjny zawiera też sprzęt delay/reverb i wzmacniacz. Rzeczywisty
sygnał instrumentu oraz stan pozostałych efektów wpływają na wynik. W tej
wersji poprawiono samą dekompozycję; nie dodano pogłosu do wtyczki.

Odtworzenie: `bash tools/render-examples-012.sh`. Parametry i sumy kontrolne
znajdują się w `manifest.json`. Materiał użytkownika nie jest publikowany.
