# Analiza obrazu filmu referencyjnego

Film użytkownika `Pico Atomic Cluster Spectral Decomposer Glitch Synth (Demo) (1).mp4`
udało się odczytać: **385,72 s, 1066×600, H.264, audio AAC stereo 44,1 kHz**.
Materiały wideo wskazują autora **Pedal of the Day**. To demo muzyczne urządzenia,
nie dokumentacja pomiarowa producenta.

Przejrzano kadry co 10 s, dodatkowe kadry w wybranych fragmentach oraz sekwencje
dwóch przełączeń. Nie przypisujemy ustawień do całej ścieżki na podstawie
pojedynczego kadru ani ozdobnych ujęć montażowych.

## Dwie zweryfikowane czynności

- **0:23,1 ±0,15 s**: palec naciska footswitch; dioda zmienia się ze zgaszonej
  na czerwoną. Fragment 0:18–0:22 można oznaczyć jako widoczny bypass Atomic,
  a 0:24–0:28 jako włączony Sharp. Nie jest to surowa gitara bez całego toru.
- **3:25,3 ±0,15 s**: naciśnięcie MODE; zielona dioda przechodzi w czerwoną.
  SPEED pozostaje około 8:30, ATOMS około 3:00, BLEND około 5:00 i VOL około 1:30.
  To przydatne porównanie Smooth/Sharp przy wizualnie podobnych ustawieniach,
  lecz grany sygnał wejściowy nie jest dostępny jako osobny identyczny ślad.

Czasy oszacowano z próbek koloru diody co 100 ms i sprawdzono na kadrach
przed/po. Sygnał MP4 jest opóźniony względem wcześniejszego MP3 o około
**23,2 ms**. Korelacja przebiegów po wyrównaniu wynosi około 0,996–0,999
w trzech czterosekundowych fragmentach. Poprzednią analizę MP3 można więc
powiązać z obrazem z uwzględnieniem tego przesunięcia.

## Przybliżone ustawienia z czytelnych ujęć

Pozycje są opisem wskazówek zegara z dokładnością około pół godziny.
Nie są pomiarem wartości potencjometrów. **Nie przeliczamy ich automatycznie
na Hz, liczbę atomów, procent wet ani dB oryginału.**

| Czas filmu | Stan | VOL | BLEND | SPEED | ATOMS |
|---|---|---|---|---|---|
| 0:20 | bypass | 1:30 | 5:00 | 1:30 | 9:30 |
| 0:30 / 1:00 | Sharp | 1:30 | 5:00 | 1:30 | 9:30 |
| 1:10 | Smooth | 1:30 | 5:00 | 1:30 | 9:30 |
| 1:30 | Smooth | 1:30 | 2:00 | 1:30 | 9:30 |
| 1:50 | Smooth | 1:30 | 12:00 | 1:30 | 9:30 |
| 2:00 | Smooth | 1:30 | 12:00 | 1:30 | 12:00 |
| 2:20 | Smooth | 1:30 | 1:00 | 4:30 | 12:00 |
| 3:10 | Smooth | 1:30 | 5:00 | 11:00 | 3:00 |
| 3:20 | Smooth | 1:30 | 5:00 | 8:30 | 3:00 |
| 3:30 | Sharp | 1:30 | 5:00 | 8:30 | 3:00 |
| 4:10 | Sharp | 1:30 | 5:00 | 8:30 | 9:30 |
| 5:20 | Sharp | 1:30 | 3:00 | 2:30 | 1:30 |
| 5:30 | Sharp | 1:30 | 3:00 | 4:30 | 3:30 |
| 5:50 | Smooth | 1:30 | 3:00 | 4:30 | 3:30 |

Zielona/czerwona dioda identyfikuje Smooth/Sharp zgodnie z instrukcją.
Nie identyfikuje automatycznie Flat/Full dynamics: tę funkcję sygnalizuje
sekwencja migania po przytrzymaniu MODE. Także ciemny kadr diody później
w filmie nie wystarcza do rozpoznania bypassu, bo działa miganie tap tempo.

## Co mówią pomiary w wizualnie opisanych fragmentach

[Spektrogram z oznaczonymi przełączeniami](annotated-spectrum.png).

| Fragment | Widoczny stan | Mediana maksimów widma* | Energia w 5 najsilniejszych binach* |
|---|---|---:|---:|
| 0:18–0:22 | bypass Atomic | 14 | 49,3% |
| 0:24–0:28 | Sharp, niskie ATOMS | 4 | 69,2% |
| 3:18–3:24 | Smooth | 17 | 37,0% |
| 3:27–3:33 | Sharp, podobne gałki | 15 | 38,0% |

\* Te same kryteria co w analizie MP3: 60–8000 Hz, Hann 4096 przy 22,05 kHz,
krok 256, maksima do 20 dB poniżej najsilniejszego. Maksima nie oznaczają
bezpośrednio liczby atomów urządzenia. Dane pochodzą z MP3 wyrównanego do filmu.

Włączeniu Atomic towarzyszy tu bardziej skupione widmo. W parze Smooth/Sharp
95. percentyl miary szybkiej zmiany widma wynosi odpowiednio 0,00862 i 0,01238.
Nie interpretujemy tej różnicy jako wyizolowanej charakterystyki przejścia
urządzenia: inne frazy, kolejne dźwięki i reszta toru też wpływają na pomiar.
Okno około 186 ms nie pozwala wyznaczyć krótszych przejść.

## Istotne uzupełnienie: pozostały sprzęt

Około **1:00** film wyświetla:

- Paul Reed Smith CE24 i Porter Pickups,
- Boss BP-1W Booster / Preamp,
- **Strymon Timeline Delay**,
- **Strymon Flint Tremolo & Reverb**,
- Milkman The Amp Stereo,
- Universal Audio Volt 476P.

Nie wiadomo, które efekty są aktywne w każdym fragmencie ani jaka jest kolejność
połączeń. Lista wystarcza jednak, żeby **nie przypisywać całego pogłosu, powtórek,
wybrzmienia i barwy wzmacniacza samemu Atomic Cluster**. Bypass Atomic nadal
może zawierać pozostałe elementy toru.

## Wniosek dla obecnej wtyczki

Obraz pozwala oznaczyć tryby i przybliżone pozycje gałek; usuwa część ograniczeń
pierwszej analizy MP3. Nie dostarcza cyfrowych zakresów oryginału, izolowanego
wejścia przed pedałem ani pomiaru jego krzywej Smooth.

Wersja 0.1.1 pozostaje zweryfikowaną, niezależną implementacją z poprawionym
wyborem maksimów. Z tych klatek nie wynika uzasadniona nowa wartość maksimum
ATOMS, krzywej SPEED czy stałej zanikania. Nie dodano do jej silnika pogłosu
ani delayu tylko po to, żeby kopiować brzmienie całego toru z filmu.

Obserwacje i przybliżone ustawienia: [observations.json](observations.json).
Próbki LED: [led_samples.json](led_samples.json).
Pomiary fragmentów: [visual_sections_metrics.json](visual_sections_metrics.json).
