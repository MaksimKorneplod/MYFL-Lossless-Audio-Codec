# MYFL — Lossless Audio Codec

> Прототип алгоритма сжатия аудиофайлов без потери качества на основе линейного предсказания и кодирования Райса.

**Автор:** Скориков М.А., группа РК6-42Б, МГТУ им. Н.Э. Баумана
**Руководитель:** Данилина Е.А.
**Год:** 2025

---

## Содержание

- [Обзор](#обзор)
- [Архитектура проекта](#архитектура-проекта)
- [Математические основы](#математические-основы)
- [Формат MYFL](#формат-myfl)
- [API модулей](#api-модулей)
- [Сборка и запуск](#сборка-и-запуск)
- [Результаты тестирования](#результаты-тестирования)
- [Зависимости](#зависимости)
- [Источники](#источники)

---

## Обзор

MYFL — пользовательский lossless-формат хранения аудио, реализующий полный цикл обработки:

1. Чтение WAV-файла (PCM 16/24/32-bit, float)
2. Разбиение на блоки
3. Вычисление LPC-коэффициентов (автокорреляция + Левинсон-Дурбин)
4. Кодирование остатков методом Райса с ZigZag-преобразованием
5. Сохранение в бинарный контейнер `.MYFL`
6. Декодирование и восстановление сигнала без потерь

---

## Архитектура проекта

Проект разделён на независимые модули:

```
myfl_types.hpp      — общие типы и структуры (MYFLHeader, BlockHeader, AudioData)
lpc.hpp / lpc.cpp   — линейное предсказание (LPC, автокорреляция, Левинсон-Дурбин)
rice.hpp / rice.cpp — кодирование Райса + ZigZag-преобразование
wav_io.hpp / wav_io.cpp  — чтение/запись WAV через libsndfile
myfl_codec.hpp / myfl_codec.cpp — кодирование и декодирование MYFL
spectrogram.hpp / spectrogram.cpp — FFT-анализ и визуализация спектрограмм
main.cpp            — точка входа (кодирование → декодирование → сравнение)
```

```
┌──────────┐   ┌──────────────┐   ┌──────────────┐
│ wav_io   │──▶│ myfl_codec   │──▶│ myfl_codec   │
│ loadAudio│   │  saveAsMYFL  │   │  decodeMYFL  │
└──────────┘   └──────┬───────┘   └──────┬───────┘
                      │                  │
              ┌───────┴───────┐  ┌───────┴───────┐
              │     lpc       │  │     rice      │
              │ calculateLPC  │  │   riceDecode  │
              │ linearPredict │  │   riceEncode  │
              └───────────────┘  └───────────────┘
                      │                  │
              ┌───────┴───────┐          │
              │    rice       │          │
              │ estimateSize  │          │
              │ riceEncode    │          │
              └───────────────┘          │
                                         │
┌────────────────────────────────────────┘
│
│  ┌──────────┐          ┌───────────────┐
│  │ wav_io   │          │ spectrogram   │
│  │ saveAsWav│◀────────▶│ computeSpectrum│
│  └──────────┘          │ drawSpectrogram│
│                        └───────────────┘
│                              │
│                        ┌─────┴─────┐
│                        │   FFTW    │
│                        └───────────┘
```

---

## Математические основы

### Линейное предсказание (LPC)

Текущее значение сигнала аппроксимируется на основе предыдущих:

```
s'[n] = Σ(k=1..p) a[k] · s[n-k]
```

Остаток (residual):

```
e[n] = s[n] - s'[n]
```

Коэффициенты рассчитываются **автокорреляционным методом** с оконной функцией Ханна и решаются **алгоритмом Левинсона-Дурбина**.

### Кодирование Райса

Частный случай Голомбовского кодирования для неотрицательных целых чисел. Параметр `k ∈ [1, 15]` подбирается динамически для каждого блока.

### ZigZag-кодирование

Преобразует знаковые целые в беззнаковые:

```
ZigZag(x) = (x << 1) ^ (x >> 31)
```

Обратное преобразование:

```
x = (z >> 1) ^ -(z & 1)
```

---

## Формат MYFL

### Заголовок файла

```c
struct MYFLHeader {
    char     magic[4];      // {'M','Y','F','L'}
    uint32_t sampleRate;    // Частота дискретизации
    uint16_t channels;      // Количество каналов
    uint16_t bitsPerSample; // Битность (16)
    uint32_t totalSamples;  // Общее число сэмплов
    uint32_t numBlocks;     // Количество блоков
};
```

### Заголовок блока

```c
struct BlockHeader {
    uint16_t param;        // Оптимальный параметр Райса
    uint16_t sample_count; // Сэмплов в блоке
    uint32_t encoded_size; // Размер закодированных данных (байт)
};
```

### Содержимое блока

| Поле | Тип | Описание |
|------|-----|----------|
| LPC-коэффициенты | `float[PREDICTOR_ORDER]` | 2 коэффициента предсказания |
| Остатки | Rice-encoded | Закодированные остатки (ZigZag + Rice) |

---

## API модулей

### myfl_types.hpp

| Тип | Описание |
|-----|----------|
| `AudioData` | Сэмплы (`vector<float>`) + `SF_INFO` |
| `MYFLHeader` | Заголовок MYFL-файла (24 байта) |
| `BlockHeader` | Заголовок блока данных (8 байт) |
| `PREDICTOR_ORDER` | Порядок предсказания = 2 |

### lpc.hpp

| Функция | Описание |
|---------|----------|
| `calculateLPCoefficients(samples, order)` | Вычисляет LPC-коэффициенты (автокорреляция + Ханн + Левинсон-Дурбин) |
| `linearPredict(samples, coeffs, order)` | Возвращает остатки предсказания |

### rice.hpp

| Функция | Описание |
|---------|----------|
| `zigZagEncode(val)` | Знаковое → беззнаковое |
| `zigZagDecode(z)` | Беззнаковое → знаковое |
| `riceEncode(residuals, param)` | Rice-кодирование в битовый поток |
| `riceDecode(encoded, param)` | Декодирование битового потока |
| `estimateRiceSize(residuals, param)` | Оценка размера без реального кодирования |

### wav_io.hpp

| Функция | Описание |
|---------|----------|
| `loadAudio(filename)` | Загрузка WAV через libsndfile |
| `saveAsWav(samples, filename, ...)` | Сохранение в WAV (PCM 16/24/32, float) |

### myfl_codec.hpp

| Функция | Описание |
|---------|----------|
| `saveAsMYFL(samples, filename, sampleRate, channels)` | Кодирование в MYFL с подбором оптимального параметра Райса |
| `decodeMYFL(filename)` | Декодирование MYFL → `vector<float>` |

### spectrogram.hpp

| Функция | Описание |
|---------|----------|
| `initFFTW()` | Инициализация FFTW (вызывать один раз) |
| `cleanupFFTW()` | Освобождение ресурсов FFTW |
| `computeSpectrum(audio, pos, channel, totalChannels)` | Расчёт амплитудного спектра (FFT, окно Ханна) |
| `drawSpectrogram(window, audio, sampleRate, channels, title)` | Отрисовка спектрограммы в SFML |

---

## Сборка и запуск

### Зависимости

| Библиотека | Назначение | URL |
|------------|------------|-----|
| **libsndfile** | Чтение/запись WAV | http://www.mega-nerd.com/libsndfile/ |
| **FFTW** | Быстрое преобразование Фурье | http://www.fftw.org/ |
| **SFML** | Графическая визуализация | https://www.sfml-dev.org/ |

### Команда сборки

```bash
g++ main.cpp lpc.cpp rice.cpp myfl_codec.cpp wav_io.cpp spectrogram.cpp \
  -o myfl \
  -lsndfile -lfftw3 -lsfml-graphics -lsfml-window -lsfml-system -lm
```

### Использование

```cpp
#include "myfl_types.hpp"
#include "myfl_codec.hpp"
#include "wav_io.hpp"
#include "spectrogram.hpp"

int main() {
    initFFTW();

    // 1. Загрузка WAV
    AudioData originalAudio = loadAudio("input.wav");

    // 2. Кодирование в MYFL
    saveAsMYFL(originalAudio.samples, "output.MYFL",
               originalAudio.info.samplerate,
               originalAudio.info.channels);

    // 3. Декодирование
    auto decoded = decodeMYFL("output.MYFL");

    // 4. Сохранение обратно в WAV
    saveAsWav(decoded, "decoded.wav",
              originalAudio.info.samplerate,
              originalAudio.info.channels,
              originalAudio.info.format & SF_FORMAT_SUBMASK);

    // 5. Визуализация спектрограмм
    sf::RenderWindow win1(sf::VideoMode({800, 400}), "Original");
    sf::RenderWindow win2(sf::VideoMode({800, 400}), "Decoded");

    while (win1.isOpen() && win2.isOpen()) {
        // ... обработка событий ...
        drawSpectrogram(win1, originalAudio.samples, ...);
        drawSpectrogram(win2, decoded, ...);
    }

    cleanupFFTW();
    return 0;
}
```

---

## Результаты тестирования

### A-Ha — Take On Me

| Параметр | Значение |
|----------|----------|
| Исходный файл | WAV, 44.1 кГц, 16-bit, стерео |
| Размер WAV | 40.3 МБ |
| Размер MYFL | 36.1 МБ |
| Сжатие | ~11 % |
| Размер FLAC (сравнение) | 26.5 МБ (~52 % сжатие) |

![Рис. 1 — Пример вывода в консоль при кодировании](images/image2.png)

![Рис. 2 — Результат сжатия](images/image3.png)

![Рис. 3 — Спектрограммы оригинального и декодированного файла](images/image4.png)

![Рис. 4 — Демонстрация полученных файлов](images/image5.png)

### Синусоида 55 Гц

| Параметр | Значение |
|----------|----------|
| Формат | WAV, PCM 32-bit float, 44.1 кГц, стерео |
| Длительность | 32 сек |
| Размер WAV | 11.3 МБ |
| Размер MYFL | 5.6 МБ |
| Сжатие | ~50 % |

![Рис. 5 — Спектрограммы синусоидального файла](images/image6.png)

### Transient (резкие переходы)

| Параметр | Значение |
|----------|----------|
| Формат | WAV, PCM 16-bit, 44.1 кГц, стерео |
| Длительность | 5 сек |
| Размер WAV | 947 КБ |
| Размер MYFL | 377 КБ |
| Сжатие | ~60 % |

![Рис. 6 — Спектрограммы транзиент аудио-файла](images/image7.png)

### Ambient-аудио

| Параметр | Значение |
|----------|----------|
| Формат | WAV, PCM 16-bit, 48 кГц, стерео |
| Длительность | 96 сек |
| Размер WAV | 18.4 МБ |
| Размер MYFL | 17.2 МБ |
| Сжатие | ~2–5 % |

![Рис. 7 — Спектрограммы эмбиент аудио-файла](images/image8.png)

### Поп-трек

| Параметр | Значение |
|----------|----------|
| Формат | WAV, PCM 16-bit, 44.1 кГц, стерео |
| Длительность | 214 сек |
| Размер WAV | 37.7 МБ |
| Размер MYFL | 37.0 МБ |
| Сжатие | ~2–5 % |

![Рис. 8 — Спектрограммы поп-трека](images/image9.png)

### Выводы

- **Lossless** — спектрограммы и осциллограммы декодированных файлов идентичны оригиналам
- **Максимальное сжатие** достигается на предсказуемых сигналах (синусоида — ~50 %, транзиенты — ~60 %)
- **Сложные спектральные структуры** (поп-музыка, ambient) сжимаются незначительно
- Артефакты отсутствуют даже на резких переходах

---

## Зависимости

```
libsndfile  — http://www.mega-nerd.com/libsndfile/
FFTW        — http://www.fftw.org/
SFML        — https://www.sfml-dev.org/
```

---

## Источники

1. Панченко А. В. Цифровая обработка сигналов: основы и практика. — М.: Солон-Пресс, 2019.
2. Итенберг А. Мультимедиа: кодирование, сжатие, передача. — СПб.: БХВ-Петербург, 2015.
3. Саломон Д., Мотта Г. Сжатие данных. Полный справочник. — М.: Техносфера, 2008.
4. Сайуд К. Введение в сжатие данных. — 5-е изд. — М.: ДМК Пресс, 2020.
5. [FLAC Format Specification](https://xiph.org/flac/format.html)
6. [Linear predictive coding — Wikipedia](https://en.wikipedia.org/wiki/Linear_predictive_coding)
7. [Rice coding — Wikipedia](https://en.wikipedia.org/wiki/Rice_coding)
