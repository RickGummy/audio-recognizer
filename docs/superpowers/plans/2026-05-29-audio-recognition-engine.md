# Audio Recognition Engine — Implementation Plan

> **How we use this plan:** This is a learning build. We go step by step. For each step,
> I (Claude) write the code and explain *why* it works the way it does; you type it out
> and ask questions until it clicks. Each step is small enough to understand in one sitting.
> Steps use checkbox (`- [ ]`) syntax so we can track progress.

**Goal:** Build a C++ "mini Shazam" — record audio, fingerprint it, match it against a database
of known songs — implementing the FFT, spectrogram, and constellation-hash algorithm from scratch.

**Architecture:** Pipeline of pure, independently testable stages:
`samples → spectrogram → peaks → fingerprints → database match`. Each stage is one module with a
narrow interface. Audio I/O (PortAudio + WAV) sits at the edges; the DSP/fingerprint core is pure
data-in/data-out so it's trivial to test without a microphone.

**Tech Stack:** C++17, CMake, PortAudio (mic + WAV), hand-rolled FFT/DSP, simple assert-based test
binaries (upgrade to Catch2 later if we want).

**Grill-prep note:** Every task has a **WHY** block. Those are the exact questions a quant interviewer
asks. If you can answer every WHY block out loud, you are grill-proof on this project.

---

## File Structure (what we build, and what each file owns)

```
audio_engine/
├── CMakeLists.txt              # build config: library + test binaries + main exe
├── src/
│   ├── main.cpp                # CLI entry: build | listen | identify
│   ├── recognizer.h/.cpp       # orchestrates the full pipeline end-to-end
│   ├── dsp/
│   │   ├── fft.h/.cpp          # Cooley-Tukey FFT + magnitude spectrum  (Task 1)
│   │   ├── spectrogram.h/.cpp  # Hann window + STFT                     (Task 2)
│   │   └── peaks.h/.cpp        # local-maxima peak extraction           (Task 3)
│   ├── fingerprint/
│   │   ├── constellation.h/.cpp# peak pairing → 32-bit hashes           (Task 4)
│   │   └── database.h/.cpp     # hash table + time-coherent matching    (Task 5)
│   └── audio/
│       ├── wav_reader.h/.cpp   # decode PCM WAV → samples               (Task 6)
│       └── capture.h/.cpp      # PortAudio mic recording                (Task 6)
└── tests/
    ├── test_fft.cpp
    ├── test_spectrogram.cpp
    ├── test_peaks.cpp
    ├── test_constellation.cpp
    └── test_database.cpp
```

**Design rule we follow:** the DSP/fingerprint core never touches PortAudio or files. It takes
`std::vector<double>` in and returns plain structs out. That keeps it pure → testable without hardware.

---

## Task 0: Project skeleton + build + test harness

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/main.cpp` (temporary "hello" stub)
- Create: `tests/test_fft.cpp` (temporary trivial assert, proves the test wiring works)

- [ ] **Step 1:** Write `CMakeLists.txt` — C++17, define a `core` static library (initially empty/stub),
      the `audio-recognition` executable, and one test executable wired through `enable_testing()` + `add_test`.
- [ ] **Step 2:** Write a stub `main.cpp` that prints usage and exits.
- [ ] **Step 3:** Write a one-line `assert(1 == 1)` test in `test_fft.cpp` just to confirm the test target builds and runs.
- [ ] **Step 4:** Configure + build: `cmake -B build && cmake --build build`. Expected: clean build, no errors.
- [ ] **Step 5:** Run `ctest --test-dir build`. Expected: 1 test passes.
- [ ] **Step 6:** Commit: `chore: project skeleton, cmake build, test harness`.

> **WHY (build system):** Why CMake + a separate `core` library instead of one big `main.cpp`?
> So tests link against the *same* compiled core the exe uses — no copy-paste, no drift. Separating
> the pure core from I/O is what makes the algorithm testable without a mic.

---

## Task 1: FFT from scratch

**Files:**
- Create: `src/dsp/fft.h`, `src/dsp/fft.cpp`
- Test: `tests/test_fft.cpp`

**Interface:**
```cpp
// fft.h
void fft(std::vector<std::complex<double>>& x);              // in-place, x.size() must be power of 2
std::vector<double> magnitude_spectrum(const std::vector<double>& samples); // real in → |X[k]| out
```

- [ ] **Step 1: Write the failing tests first.**
  - Test A: an 8-point FFT of a known input matches hand-computed DFT values (within epsilon).
  - Test B: generate `sin(2π·440·t)` sampled at 44100 Hz over a power-of-2 window, run
    `magnitude_spectrum`, assert the **argmax bin** corresponds to ~440 Hz
    (bin = round(440 · N / 44100)).
  - Test C: a power-of-2-size assertion / zero-pad check.
- [ ] **Step 2:** Build + run tests, confirm they FAIL (function not implemented).
- [ ] **Step 3:** I write `fft()` — recursive Cooley-Tukey radix-2 (split even/odd, recurse, butterfly combine).
- [ ] **Step 4:** I write `magnitude_spectrum()` — zero-pad to next power of 2, copy reals into complex,
      call `fft`, take `std::abs` of each bin.
- [ ] **Step 5:** Build + run tests, confirm PASS.
- [ ] **Step 6:** Commit: `feat(dsp): radix-2 Cooley-Tukey FFT + magnitude spectrum`.

> **WHY (the core grill):**
> - *Why FFT over DFT?* DFT is O(N²); FFT is O(N log N) by reusing the symmetry of the twiddle
>   factors. For N=4096 that's ~49k ops vs ~16M.
> - *Why power of 2?* Radix-2 splits the input in half each recursion; that only works cleanly when N halves to 1.
> - *What's the butterfly?* The combine step: `X[k] = E[k] + W·O[k]`, `X[k+N/2] = E[k] − W·O[k]`, where `W = e^(−2πik/N)`.
> - *What does bin k mean in Hz?* `freq(k) = k · sample_rate / N`. Memorize this — it's asked every time.
> - *Why only the lower half of bins?* Real input → spectrum is conjugate-symmetric; the upper half mirrors the lower.
> - *(Optimization talking point, build later):* recursion is the textbook version; the fast version is
>   iterative in-place with bit-reversal + precomputed twiddles, then SIMD. We note this now, do it in Task 9.

---

## Task 2: Spectrogram (STFT)

**Files:**
- Create: `src/dsp/spectrogram.h`, `src/dsp/spectrogram.cpp`
- Test: `tests/test_spectrogram.cpp`

**Interface:**
```cpp
struct SpectrogramFrame { double time_seconds; std::vector<double> magnitudes; };

std::vector<SpectrogramFrame> compute_spectrogram(
    const std::vector<double>& samples, int sample_rate,
    int window_size = 4096, int hop_size = 512);
```

- [ ] **Step 1: Failing tests.**
  - Test A: a constant 1 kHz sine → every frame's argmax bin sits at the 1 kHz bin.
  - Test B: frame count == `1 + (samples - window_size) / hop_size`; `time_seconds` of frame i == `i·hop/sample_rate`.
  - Test C: Hann window endpoints are ~0 (sanity-check the window function in isolation).
- [ ] **Step 2:** Build + run, confirm FAIL.
- [ ] **Step 3:** I write a `hann_window(N)` helper.
- [ ] **Step 4:** I write `compute_spectrogram` — slide window by hop, multiply chunk by Hann, call
      `magnitude_spectrum`, keep lower half of bins, stamp `time_seconds`.
- [ ] **Step 5:** Build + run, confirm PASS.
- [ ] **Step 6:** Commit: `feat(dsp): Hann-windowed STFT spectrogram`.

> **WHY:**
> - *Why a window at all (Hann)?* Chopping a signal into chunks creates fake discontinuities at the
>   edges → spectral leakage (energy smears across bins). Hann tapers the edges to zero to suppress it.
> - *Window size vs hop size tradeoff?* Bigger window = finer **frequency** resolution but coarser **time**
>   resolution (uncertainty principle). Smaller hop = more overlap = finer time resolution but more frames/compute.
> - *Why 4096 @ 44100?* ~93 ms window, ~10.7 Hz/bin — good enough to separate musical notes. Hop 512 → ~11.6 ms steps.

---

## Task 3: Peak extraction

**Files:**
- Create: `src/dsp/peaks.h`, `src/dsp/peaks.cpp`
- Test: `tests/test_peaks.cpp`

**Interface:**
```cpp
struct Peak { int time_frame; int freq_bin; double magnitude; };

std::vector<Peak> extract_peaks(
    const std::vector<SpectrogramFrame>& spectrogram,
    int neighborhood_size = 10, double threshold = 10.0);
```

- [ ] **Step 1: Failing tests.**
  - Test A: a synthetic spectrogram with one obvious bright spot → exactly that (frame,bin) is returned.
  - Test B: points below `threshold` are ignored.
  - Test C: two peaks closer than `neighborhood_size` → only the louder survives.
- [ ] **Step 2:** Build + run, confirm FAIL.
- [ ] **Step 3:** I write `extract_peaks` — for each (time,freq) point, check it's the max within
      ±`neighborhood_size` in both axes AND above `threshold`; if so, emit a Peak.
- [ ] **Step 4:** Build + run, confirm PASS.
- [ ] **Step 5:** Commit: `feat(dsp): local-maxima peak extraction`.

> **WHY:**
> - *Why peaks instead of the whole spectrogram?* Peaks are the loudest, most stable points — they survive
>   noise and compression. Background chatter raises the noise floor a little but doesn't create new *dominant* peaks.
> - *Why local maxima in a neighborhood?* Gives a roughly even spread of peaks across time/freq instead of
>   clumping all of them in the loudest second.
> - *Tuning?* ~3–10 peaks/sec. Too many → slow matching + more false collisions; too few → poor recall.

---

## Task 4: Constellation map + hashing

**Files:**
- Create: `src/fingerprint/constellation.h`, `src/fingerprint/constellation.cpp`
- Test: `tests/test_constellation.cpp`

**Interface:**
```cpp
struct Fingerprint { uint32_t hash; int time_offset; };  // time_offset = anchor peak's time_frame

std::vector<Fingerprint> generate_fingerprints(
    const std::vector<Peak>& peaks, int fan_out = 15, int time_window = 200);
```

- [ ] **Step 1: Failing tests.**
  - Test A: two anchors with known freq bins + known delta → assert the exact packed hash value
    (`(fA << 18) | (fB << 9) | dt`, using 9 bits/freq + a clamped delta — exact bit layout fixed in Step 3).
  - Test B: **determinism** — same peaks in → identical fingerprint set out (this is the property the
    whole algorithm depends on).
  - Test C: fan_out cap respected; only pairs within `time_window` frames are formed.
- [ ] **Step 2:** Build + run, confirm FAIL.
- [ ] **Step 3:** I write `generate_fingerprints` — sort peaks by time; for each anchor, pair it with the
      next `fan_out` peaks inside `time_window`; pack `(freqA, freqB, deltaTime)` into a `uint32_t`.
      We clamp freq bins to the lower 512 (9 bits each) and delta to 8–14 bits so it fits 32 bits cleanly.
- [ ] **Step 4:** Build + run, confirm PASS.
- [ ] **Step 5:** Commit: `feat(fingerprint): constellation pairing + 32-bit hashing`.

> **WHY:**
> - *Why pair peaks instead of hashing single peaks?* One frequency isn't distinctive — lots of songs have
>   a 440 Hz peak. A *pair* `(freqA, freqB, Δt)` is far more unique, so collisions across songs drop sharply.
> - *Why include Δt and not absolute time in the hash?* Δt is translation-invariant — it's the same whether
>   you start recording at the chorus or the bridge. Absolute time is stored *separately* (`time_offset`) for the
>   alignment step in Task 5, not baked into the hash.
> - *Why bit-pack into one uint32?* One integer key = fast, cache-friendly hash-map lookups.
> - *Why does determinism matter?* Matching only works if the same audio always produces the same hashes.

---

## Task 5: Fingerprint database + time-coherent matching

**Files:**
- Create: `src/fingerprint/database.h`, `src/fingerprint/database.cpp`
- Test: `tests/test_database.cpp`

**Interface:**
```cpp
struct SongMatch { std::string song_name; int match_count; double confidence; };

class FingerprintDatabase {
public:
    void add_song(const std::string& name, const std::vector<Fingerprint>& fps);
    SongMatch query(const std::vector<Fingerprint>& fps);
    void save(const std::string& path);
    void load(const std::string& path);
private:
    std::unordered_map<uint32_t, std::vector<std::pair<int,int>>> table_; // hash → [(song_id, db_time)]
    std::vector<std::string> song_names_;
};
```

- [ ] **Step 1: Failing tests.**
  - Test A: add one song's fingerprints, query with the *same* set → returns that song with high match_count.
  - Test B: query a time-shifted copy (shift every `time_offset` by +K) → still matches; this proves
    **time-coherence** works (all matches share delta = +K).
  - Test C: query against a different random fingerprint set → low/zero match_count.
  - Test D: `save` then `load` round-trips an identical database.
- [ ] **Step 2:** Build + run, confirm FAIL.
- [ ] **Step 3:** I write `add_song` (assign song_id, push `(song_id, db_time)` per hash).
- [ ] **Step 4:** I write `query` — the clever part: for each query fp, look up hash, and for each hit
      increment a counter keyed by `(song_id, db_time − query_time)`. Winner = the single `(song_id, delta)`
      bucket with the highest count. `confidence` = winner_count / total_query_fingerprints.
- [ ] **Step 5:** I write `save`/`load` (binary serialization of the table + song names).
- [ ] **Step 6:** Build + run, confirm PASS.
- [ ] **Step 7:** Commit: `feat(fingerprint): hash DB with time-coherent matching + persistence`.

> **WHY (the most important grill question):**
> - *Why not just count matching hashes?* Random hash collisions from wrong songs inflate raw counts → false
>   positives. The fix: the *right* song produces matches that all share the **same time offset** (`db_time − query_time`),
>   because the recording is just a shifted slice of the original. Wrong-song collisions scatter across random deltas.
>   So we histogram by delta and take the tallest spike. The *alignment*, not the raw count, is the signal.
> - *Complexity of query?* O(total query fingerprints × avg postings per hash). Hash lookups are ~O(1).
> - *Why `unordered_map<uint32, vector<pair>>`?* Inverted index: hash → everywhere it occurs. Classic IR structure.

---

## Task 6: Audio I/O (WAV reader + mic capture)

**Files:**
- Create: `src/audio/wav_reader.h`, `src/audio/wav_reader.cpp`
- Create: `src/audio/capture.h`, `src/audio/capture.cpp`

**Interface:**
```cpp
std::vector<double> read_wav(const std::string& path, int& sample_rate);   // mono, normalized [-1,1]
std::vector<double> record_audio(int duration_seconds, int sample_rate = 44100); // PortAudio mic
```

- [ ] **Step 1:** Add PortAudio to `CMakeLists.txt` (`find_package`/link). Confirm it links with a no-op call.
- [ ] **Step 2:** I write `read_wav` — parse the RIFF/`fmt `/`data` chunks, handle 16-bit PCM, downmix to
      mono, normalize to `[-1,1]`, return sample_rate via the out-param.
- [ ] **Step 3:** Test `read_wav` against a tiny known WAV checked into `tests/fixtures/` (assert sample_rate +
      first few sample values). *This is the only I/O thing we unit-test; mic is verified manually.*
- [ ] **Step 4:** I write `record_audio` — open a blocking PortAudio input stream, capture N seconds mono @ 44100,
      normalize, return.
- [ ] **Step 5:** Manual check: record 3 s, print sample count == duration·sample_rate, print min/max in range.
- [ ] **Step 6:** Commit: `feat(audio): WAV reader + PortAudio mic capture`.

> **WHY:**
> - *Why normalize to [-1,1]?* Decouples the DSP from input bit-depth/volume so thresholds behave consistently.
> - *Why mono?* Fingerprinting needs one spectral view; stereo doubles work for no recognition benefit.
> - *Blocking vs callback capture?* Blocking is simpler and fine for a 5 s record-then-process flow. (Callback +
>   ring buffer is the upgrade path if we add live streaming — Task 9.)

---

## Task 7: Recognizer + CLI integration

**Files:**
- Create: `src/recognizer.h`, `src/recognizer.cpp`
- Modify: `src/main.cpp` (replace stub)

**Interface:**
```cpp
// recognizer.h — the full pipeline, reused by every CLI mode
std::vector<Fingerprint> fingerprint_samples(const std::vector<double>& samples, int sample_rate);
void build_database(const std::string& songs_dir, const std::string& db_path);
SongMatch identify_samples(const std::vector<double>& samples, int sample_rate, FingerprintDatabase& db);
```

- [ ] **Step 1:** I write `fingerprint_samples` — chains spectrogram → peaks → fingerprints (the shared pipeline).
- [ ] **Step 2:** I write `build_database` — loop WAVs in `data/songs/`, fingerprint each, `add_song`, `save`.
- [ ] **Step 3:** I write `identify_samples` — fingerprint input, `db.query`, return match.
- [ ] **Step 4:** Wire `main.cpp`: `build` | `listen` (record 5 s → identify) | `identify <file.wav>`.
- [ ] **Step 5:** End-to-end manual test: drop 2–3 WAVs in `data/songs/`, run `build`, then `identify` a
      clip of one → correct song with high confidence. Then the **noise test**: play a song through speakers,
      `listen` in a slightly noisy room → still matches.
- [ ] **Step 6:** Commit: `feat: end-to-end recognizer + CLI (build/listen/identify)`.

> **WHY:** This is where you tell the project's story in an interview: "raw samples → STFT → peaks →
> constellation hashes → time-coherent DB match." Be able to trace one sample all the way to a song name.

---

## Task 8: README + resume bullets (the payoff)

- [ ] **Step 1:** Rewrite `README.md`: what it is, how to build/run, a one-paragraph algorithm explainer, and a
      results line (N songs in DB, recognition latency, accuracy under noise — fill from Task 7's manual runs).
- [ ] **Step 2:** Fill the resume bullets in `audio_recognition_engine.md` with **real measured numbers**
      (track count, match latency, accuracy). Numbers > adjectives for quant.
- [ ] **Step 3:** Commit: `docs: README + measured results`.

---

## Task 9 (OPTIONAL — quant flex, only if you want it): performance + concurrency

Do this *after* the thing works end-to-end. This is what turns "neat project" into "quant-relevant systems work."

- [ ] **Iterative in-place FFT** with bit-reversal + precomputed twiddle factors; benchmark vs the recursive version.
- [ ] **SIMD butterflies** (AVX) on the inner FFT loop; report the speedup.
- [ ] **Parallel `build_database`** — thread pool over WAV files, per-thread local tables merged at the end
      (no locks needed — that's the better story than a contended lock-free map).
- [ ] **Live streaming `listen`** — PortAudio callback producer + matcher consumer connected by an
      **SPSC lock-free ring buffer**. *This* is the genuine lock-free talking point.
- [ ] **Google Benchmark** harness; record latency/throughput numbers and put them in the README.

> **WHY this matters for quant specifically:** quant SWE is judged on latency, throughput, and correctness
> under load — not "does it work." Measured before/after optimization numbers and a real concurrency story
> (SPSC ring buffer, lock-free reasoning, SIMD) are exactly the signals they probe for.

---

## Self-Review (plan vs spec)

- **Spec coverage:** Spec stages 1–7 map to Tasks 1–7. Spec "Testing Plan" → folded into per-task TDD steps.
  Spec "Resume Bullets" → Task 8. The benchmark/concurrency gap the spec lacked → Task 9 (optional).
- **Type consistency:** `Fingerprint{hash, time_offset}`, `Peak{time_frame, freq_bin, magnitude}`,
  `SpectrogramFrame{time_seconds, magnitudes}`, `SongMatch{song_name, match_count, confidence}` — names used
  identically across Tasks 1–7.
- **Hash bit-layout note:** exact bit widths (9/9/14 vs the spec's 9/9/8) get locked in Task 4 Step 3 — we pick
  one layout and the test asserts that exact packed value, so it can't drift.

---

## How we'll run it

We go **Task 0 → Task 9 in order**. Per step: I write the code and explain the *why*; you type it and
poke holes until you understand it; we build + run the test; we only move on when it's green and you can
explain it. We commit at the end of each task. Task 9 is optional and we decide on it after the core works.
