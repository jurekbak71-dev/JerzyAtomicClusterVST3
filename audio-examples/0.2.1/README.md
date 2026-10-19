# Stereo i BPM SYNC — test wydania 0.2.1

[▶ Jedno nagranie porównawcze — 1 min 41 s](comparison.mp3).
Wszystkie próbki używają prawdziwej konfiguracji **mono-in → stereo-out**
w wydanym Windows VST3. To ta sama oryginalna fraza szarpana, bez zewnętrznych
efektów i normalizacji poziomu. VOL = unity; efektowe próbki mają 100% wet.
Stereo najlepiej ocenić na słuchawkach.

| Początek | Fragment |
|---|---|
| 0:00 | [Wejście bez efektu](00_dry.mp3) |
| 0:12,75 | [Wet, stereo wyłączone](01_wet_mono.mp3) |
| 0:25,50 | [Ten sam wet, stereo 100%](02_wet_stereo.mp3) |
| 0:38,25 | [Jeden atom krążący L/R](03_single_atom_orbit.mp3) |
| 0:51 | [ARP — 120 BPM, ósemki](04_arp_120bpm_eighths.mp3) |
| 1:03,75 | [ARP — 90 BPM, ósemki](05_arp_90bpm_eighths.mp3) |
| 1:16,50 | [RANDOM — 120 BPM, szesnastki](06_random_120bpm_sixteenths.mp3) |
| 1:29,25 | [SMOOTH — 120 BPM, ćwierćnuty](07_smooth_120bpm_quarters.mp3) |

[Wszystkie MP3 w ZIP-ie](JerzyAtomicCluster-0.2.1-examples.zip).

W BPM SYNC renderujący host przekazuje rzeczywiste `ProcessContext` z tempem,
pozycją PPQ i stanem odtwarzania. Wtyczka wyrównuje obwiednię do tej siatki,
z uwzględnieniem zgłaszanej latencji; nie jest to tylko ręcznie wpisany czas w ms.
Sygnał ma 9 sekund, pliki 12 sekund z ogonem; między fragmentami porównania
jest 0,75 sekundy ciszy. Sumy kontrolne, ustawienia i poziomy L/R:
`manifest.json`. Odtworzenie: `bash tools/render-examples-021.sh`.
