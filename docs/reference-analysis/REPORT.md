# Materiał referencyjny i poprawka 0.1.1

Materiał: przesłany przez użytkownika `Pico Atomic Cluster Spectral Decomposer
Glitch Synth (Demo)_01.mp3`. Długość zdekodowanego nagrania: **385,63 s**,
oryginalne próbkowanie 44,1 kHz, stereo. Kanały są niemal identyczne
(korelacja 0,9998). Nie są to osobne ślady dry/wet.

Wykonano **analizę pomiarową**, nie ocenę odsłuchową ani analizę obrazu filmu.
Nie udostępniamy kopii nagrania referencyjnego w repozytorium ani paczce wtyczki.

## Co widać w sygnale

Obraz całości: [widmo i zmiany w czasie](overview.png).

| Fragment | Pomiar / obserwacja | Czego z niego nie wnioskujemy |
|---|---|---|
| 0:25–0:40 | Mediana 4 wyraźnych lokalnych maksimów widma powyżej progu –20 dB względem najsilniejszej składowej | Nie oznacza to ustawienia ATOMS=4 |
| 0:55–1:10 | Kilka dominujących, utrzymywanych pasm; mediana 71,5% energii pasma 60–8000 Hz przypada na 5 najsilniejszych binów analizy | Nie dowodzi konkretnego algorytmu oscylatorów ani rozdzielczości FFT urządzenia |
| 1:45–2:00 | Długie prążki z krótkimi szerszymi transjentami, m.in. około 1:49 i 1:55 | Nie przypisujemy tego pewnie do SMOOTH; brak widoku gałek i śladu wejściowego |
| 2:40–2:55 | Więcej szybkich zmian widma niż w wyżej wymienionych fragmentach | Część może wynikać ze sposobu gry lub źródła, nie z SPEED |
| 5:25–5:40 | Znacznie gęstsze widmo: mediana 43 maksimów według tego samego kryterium | Nie jest to pomiar liczby atomów pedału |

Parametry analizy: mono jako średnia kanałów, 22,05 kHz, okno Hann 4096 próbek,
krok 256 próbek. Rozdzielczość binów 5,38 Hz; czas okna około 186 ms. Ten obraz
nie rozstrzyga czasów przejść krótszych od okna. Wyniki liczbowe zapisano
w [summary.json](summary.json).

Do ponownego wykonania pomiarów potrzebny jest WAV 22,05 kHz oraz Python
z NumPy, SciPy i Matplotlib:

```sh
ffmpeg -i demo.mp3 -ar 22050 -c:a pcm_f32le reference-stereo.wav
python3 tools/analyze-reference.py reference-stereo.wav --output /workspace/atomic-reference-analysis
```

Skrypt uśrednia kanały i zapisuje wykresy oraz statystyki. Do odtworzenia tych
samych wyników należy użyć tego samego MP3 przekazanego przez użytkownika.

Nie można skalibrować zakresu SPEED, BLEND, VOL ani przełącznika dynamiki
z tego MP3 bez ustawień i izolowanego śladu dry. Nie można też odzyskać unikalnego
algorytmu urządzenia z nagrania końcowego. Poprzednie syntetyczne przykłady
wtyczki i demo EHX mają różne wejścia, więc nie są kontrolowanym porównaniem A/B.

## Potwierdzony problem naszej wersji 0.1.0

Przegląd sposobu liczenia atomów doprowadził do osobnego, kontrolowanego testu:
podano **440 Hz o amplitudzie 0,20 oraz 880 Hz o amplitudzie 0,08**, ustawiono
ATOMS=2, 100% wet, unity VOL, Sharp i szybkie odświeżanie.

W 0.1.0 dwa najgłośniejsze biny mogły należeć do **jednego** tonu 440 Hz.
Przy 880 Hz wyjście wyniosło około 0,00000018: stosunek 880/440 spadł z –7,96 dB
na wejściu do **–120,32 dB**. To potwierdza ograniczenie naszej selekcji, nie
wewnętrzną budowę EHX.

Po poprawce stosunek wynosi **–7,9615 dB**, blisko wejściowego –7,9588 dB.
To pomiar kontrolny Linux VST3, 48 kHz, środkowa sekunda trzysekundowego tonu;
odrębne testy Windows pod Wine potwierdzają zachowanie dwóch składowych
w trybach Sharp i Smooth.

## Poprawka 0.1.1

- ATOMS wybiera odrębne lokalne maksima zamiast pojedynczych najgłośniejszych binów.
- Biny okna do ±2 wokół maksimum są grupowane z nim; ich fazy są powiązane
  z fazą środka. Zapobiega to zużyciu dwóch atomów na sąsiednie biny tego samego tonu.
- Flat wyrównuje poziom wybranych maksimów, skalując ich grupy wspólnie.
- Maksima poniżej –60 dB względem najsilniejszego są pomijane. To wybór naszej
  implementacji, nie zmierzony parametr EHX.
- Pierwsze oszacowanie częstotliwości wykorzystuje interpolację maksimum.
- Pełne ATOMS/Full dynamics nadal zachowuje całe widmo.

Dodano test zachowania drugiego tonu dla Sharp i Smooth oraz test wyrównania
dwóch składowych w Flat. Identyfikatory parametrów i format zapisu projektu
pozostały zgodne; przy tych samych ustawieniach brzmienie może się zmienić.

Wyniki kontrolne przed/po: [probe-results.json](probe-results.json).
Status wszystkich testów: [VALIDATION.md](../VALIDATION.md).

**Poprawka nie jest potwierdzeniem zgodności brzmienia z EHX.** Do kontrolowanego
dopasowania potrzebne są ustawienia gałek, oznaczenia bypass/dry/wet i najlepiej
ten sam izolowany sygnał wejściowy przepuszczony przez fizyczną kostkę.
