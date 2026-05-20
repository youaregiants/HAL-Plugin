# HAL — Natural Language Synthesizer: Claude Code Context

> Load this file first. It is the single source of truth for any Claude Code session working on HAL.

## What is HAL?

A polyphonic subtractive synthesizer VST3/AU/CLAP/Standalone built with **iPlug2** (C++/Objective-C++, macOS arm64+x86_64). The user types a plain-English sound description ("warm analog pad with slow attack"), presses Generate, and an AI backend (Claude sonnet-4-6 via FastAPI on a Hetzner VPS) translates it into a full synth patch — setting all 56 parameters automatically.

Two repos make up the system:
- **This repo** (`youaregiants/HAL-Plugin`) — iPlug2 plugin (client side)
- **Server** — FastAPI on VPS at `62.238.41.219`, code at `/home/platform/hal/`; accessible via HTTPS at `https://62-238-41-219.sslip.io`

---

## Directory layout

```
HAL-Plugin/
├── CLAUDE.md                  ← you are here
├── HAL/
│   ├── HAL.cpp                ← main plugin: constructor (UI), DSP, HTTP, JSON apply
│   ├── HAL.h                  ← EParams enum, EControlTags, class declaration
│   ├── HALVoice.h             ← HALSynthParams struct, HALVoice class (DSP)
│   ├── HALEffects.h           ← HALChorus, HALReverb, HALDelay, halSoftSat()
│   ├── HALHTTPClient.h/.mm    ← NSURLSession async HTTP (Obj-C++)
│   ├── HALPresets.h           ← "Not ___" preset bank (HALPreset array)
│   ├── HALExport.h            ← WAV writer (SMPL loop chunk), Vital JSON builder
│   ├── config.h               ← PLUG_WIDTH/HEIGHT (820×520), plugin metadata
│   ├── config/HAL-mac.xcconfig← build settings: IPLUG2_ROOT, EXTRA_INC_PATHS (JSON_INC_PATH)
│   └── projects/
│       └── HAL-macOS.xcodeproj← main Xcode project; targets: APP, VST3, AU, CLAP
└── iPlug2/                    ← iPlug2 framework (symlink / submodule, not committed)
```

---

## Build

```bash
# Standalone app (fastest for development)
xcodebuild -project HAL/projects/HAL-macOS.xcodeproj -target APP -configuration Debug

# VST3
xcodebuild -project HAL/projects/HAL-macOS.xcodeproj -target VST3 -configuration Debug

# Open the built app
open /Users/christopherhlee/Applications/HAL.app

# Install VST3 (already goes to ~/Library/Audio/Plug-Ins/VST3/ automatically)
```

Build output: `HAL/build-mac/` (ignored by git), APP installs to `~/Applications/`.

---

## EParams enum — COMPLETE (indices matter for server ↔ plugin mapping)

```cpp
// HAL/HAL.h
enum EParams {
  // OSC 1
  kOsc1Waveform = 0,   kOsc1Octave,     kOsc1FineTune,
  kOsc1UnisonVoices,   kOsc1UnisonSpread, kOsc1Volume,      // 0–5

  // OSC 2
  kOsc2Waveform = 6,   kOsc2Octave,     kOsc2FineTune,
  kOsc2UnisonVoices,   kOsc2UnisonSpread, kOsc2Volume,      // 6–11

  // OSC 3
  kOsc3Waveform = 12,  kOsc3Octave,     kOsc3FineTune,
  kOsc3UnisonVoices,   kOsc3UnisonSpread, kOsc3Volume,      // 12–17

  // Filter
  kFilterType = 18,    kFilterCutoff,   kFilterResonance,
  kFilterEnvAmount,    kFilterDrive,                         // 18–22

  // Filter Envelope
  kFilterAttack = 23,  kFilterDecay,    kFilterSustain,  kFilterRelease, // 23–26

  // Amp Envelope
  kAmpAttack = 27,     kAmpDecay,       kAmpSustain,     kAmpRelease,    // 27–30

  // LFO 1
  kLFO1Shape = 31,     kLFO1Rate,       kLFO1Depth,      kLFO1Target,    // 31–34

  // LFO 2
  kLFO2Shape = 35,     kLFO2Rate,       kLFO2Depth,      kLFO2Target,    // 35–38

  // Effects
  kChorusDepth = 39,   kChorusMix,      kReverbSize,     kReverbMix,     // 39–42
  kDelayTime,          kDelayFeedback,  kDelayMix,                        // 43–45
  kSaturationDrive,    kEQLowShelf,     kEQHighShelf,                     // 46–48

  // Pitch Envelope (for 808 kicks, stabs)
  kPitchEnvAmount = 49, kPitchEnvAttack, kPitchEnvDecay,                 // 49–51

  // Arpeggiator
  kArpEnabled = 52,    kArpRate,        kArpPattern,     kArpOctaves,    // 52–55

  kNumParams = 56
};
```

**Waveform encoding:** 0=sine, 1=saw, 2=square, 3=triangle  
**Filter type:** 0=lp12, 1=lp24, 2=hp12, 3=bp, 4=notch  
**LFO shape:** 0=sine, 1=tri, 2=saw, 3=sq, 4=random  
**LFO target:** 0=osc_pitch, 1=filter_cutoff, 2=osc_volume, 3=pan  
**Arp pattern:** 0=up, 1=down, 2=updown, 3=random

---

## DSP architecture (HALVoice.h / HAL.cpp ProcessBlock)

### Signal chain (per block)
```
MIDI → Arp engine → Voice NoteOn/Off
Per-sample loop:
  LFO[0].Process(rate) → lfoMod[0]
  LFO[1].Process(rate) → lfoMod[1]
  For each of 8 HALVoice:
    PitchEnv.Process(0.0)   → pitch multiplier (semitone sweep)
    3× OscBank (unison up to 8) → mixedOsc
    Pre-filter drive (tanh)
    FilterEnv.Process(sustain) → cutoff modulation
    applyFilter (SVF cascade) → filtered
    AmpEnv.Process(sustain) → voice output
  Sum voices × 0.2 headroom
halSoftSat → EQ shelves (SVF<double,2>) → Chorus → Reverb → Delay
```

### Critical iPlug2 API patterns

```cpp
// SVF per-sample usage (must use pointer-to-pointer):
double in = x, out = 0.0;
double *pi = &in, *po = &out;
filter.SetFreqCPS(hz); filter.SetQ(Q); filter.SetMode(mode); filter.SetSampleRate(sr);
filter.ProcessBlock(&pi, &po, 1, 1);

// LP24 = two SVF<double,1> in series (mFilter + mFilter2, both kLowPass)
x = run(mFilter, kLowPass); return run(mFilter2, kLowPass);

// LFO shape mapping (plugin enum ≠ LFO<double>::EShape):
// plugin: 0=sine,1=tri,2=saw,3=sq,4=random
// LFO<double>::EShape: kTriangle=0,kSquare=1,kRampUp=2,kRampDown=3,kSine=4
static const LFO<double>::EShape shapeMap[5] = {kSine, kTriangle, kRampUp, kSquare, kRampUp};

// ADSR envelope:
env.SetSampleRate(sr);
env.SetStageTime(ADSREnvelope<double>::kAttack, ms);  // kAttack=0,kDecay=1,kRelease=3
env.Start(1.0);        // note on
env.Retrigger(1.0);    // note on while busy
env.Release();         // note off
env.Process(sustain);  // returns current level; call once per sample
env.GetBusy();         // false = voice is silent

// EQ shelf (stereo, NC=2):
mEQLow.SetMode(SVF<double,2>::kLowPassShelf);
mEQLow.SetFreqCPS(200.0); mEQLow.SetQ(0.707); mEQLow.SetGain(dB);
mEQLow.ProcessBlock(outputs, outputs, 2, nFrames);

// Pitch envelope (decays from amount semitones → 0, sustain=0):
const double pEnv = mPitchEnv.Process(0.0);
double pitchMod = std::pow(2.0, pEnv * p.pitchEnvAmount / 12.0);
```

### Thread safety
- `mSynthParams` written on main thread (OnParamChange), read on audio thread (ProcessBlock) — struct copy at block start (`const HALSynthParams p = mSynthParams`)
- HTTP callback → `mPatchMutex` + `mPatchReady`/`mStatusDirty` atomics → `OnIdle()` (main thread) for all UI updates

---

## HTTP / Server

**Endpoint:** `POST https://62-238-41-219.sslip.io/api/generate-patch`  
**Auth:** `Authorization: Bearer bd639f6ed47b51a5acfd42960abde05a0a995913e13d092de3fa424c11a545ba`  
**Health check:** `GET https://62-238-41-219.sslip.io/api/health`

### Request body (JSON)
```json
{
  "description": "warm analog pad",
  "style_hint": "PAD ambient",         // optional; category tab prepended automatically
  "nudge": "make it darker",           // optional; triggers refine mode
  "current_patch": { ... }             // optional; sent when nudging (BuildCurrentPatchJSON)
}
```

### Response body
```json
{
  "patch": { "osc1": {...}, "osc2": {...}, "osc3": {...},
             "filter": {...}, "filter_env": {...}, "amp_env": {...},
             "pitch_env": {"amount_semitones":0,"attack_ms":2,"decay_ms":150},
             "lfo1": {...}, "lfo2": {...}, "effects": {...} },
  "cached": false
}
```

Patch schema is validated by Pydantic `PatchParameters` on the server. The plugin parses JSON via nlohmann/json and applies via `ApplyPatchJSON()`.

---

## Server (VPS)

**SSH:** `ssh platform@62.238.41.219`  
**Code:** `/home/platform/hal/`  
**Venv:** `/home/platform/hal/venv/`  
**Service:** `sudo systemctl restart hal-api` / `sudo systemctl status hal-api`  
**Logs:** `journalctl -u hal-api -f`  
**Deploy:** `ssh platform@62.238.41.219 "cd /home/platform/hal && ./deploy.sh"`

### Server file layout
```
/home/platform/hal/
├── api/
│   ├── main.py           — FastAPI app, CORS, auth middleware
│   └── routes/generate.py — /api/generate-patch endpoint, Bearer auth
├── core/
│   ├── prompt_engine.py  — SYNTH_SYSTEM_PROMPT + build_generation_prompt()
│   ├── claude_client.py  — Anthropic SDK wrapper, retry, cost tracking
│   ├── patch_schema.py   — Pydantic models: PatchParameters, Oscillator, Filter, etc.
│   └── cache.py          — in-memory SHA256 cache (keyed on description+style_hint)
├── .env                  — HAL_API_KEY, ANTHROPIC_API_KEY
└── deploy.sh             — git pull + pip install + systemctl restart
```

### Model used for patch generation
`claude-sonnet-4-6` (via `anthropic` Python SDK). Upgrade to `claude-opus-4-7` for richer patches if latency allows.

### Adding new patch fields
1. Add to `PatchParameters` in `core/patch_schema.py`
2. Add to `SYNTH_SYSTEM_PROMPT` perceptual mappings in `core/prompt_engine.py`
3. Add to `ApplyPatchJSON()` in `HAL/HAL.cpp`
4. Add to `BuildCurrentPatchJSON()` in `HAL/HAL.cpp` (for nudge round-trip)

---

## UI layout (HAL.cpp mLayoutFunc)

**Canvas:** 820 × 520 px (config.h: PLUG_WIDTH/PLUG_HEIGHT)  
**Font:** Roboto-Regular.ttf (loaded via `pGraphics->LoadFont`)

```
┌─────────────────────────────────────────────────────────────────┐
│ HAL                          natural language synthesizer  [36px]│
├────┬────┬─────┬─────┬──────┬─────┬────────────────────────────┤
│PAD │ARP │BASS │LEAD │STAB  │ 808 │ FX            [tab bar 28px]│
├──────────────┬──────────────────────────────────────────────────┤
│ DESCRIBE     │ OSCILLATORS (blue)                               │
│ [prompt]     │  O1: Wave Oct Fine Uni Sprd Vol                  │
│ STYLE HINT   │  O2: Wave Oct Fine Uni Sprd Vol                  │
│ [style]      │  O3: Wave Oct Fine Uni Sprd Vol                  │
│ [GENERATE]   ├──────────────────────────────────────────────────┤
│ status...    │ FILTER (orange) + PITCH ENV (green)              │
│──────────────│  FLT: Type Cut Res Env Drv | PEnv PAtk PDec     │
│ NUDGE/REFINE │  FEV: Atk Dec Sus Rel                            │
│ [nudge]      ├──────────────────────────────────────────────────┤
│──────────────│ AMP ENV / LFO                                    │
│ NOT PRESETS  │  AMP: Atk Dec Sus Rel                            │
│ [Choose...]  │  LFO: Shp Rate Dep Tgt | Shp Rate Dep Tgt       │
│ [WAV][VITAL] ├──────────────────────────────────────────────────┤
│              │ EFFECTS / ARP (yellow/orange)                    │
│              │  FX: ChD ChM RvS RvM DlT DlF DlM Sat EQL EQH   │
│              │  ARP: On Rate Pat Oct                            │
└──────────────┴──────────────────────────────────────────────────┘
200px left      620px right
```

**Color palette:**
- Background: `#0A0A0C` — `IColor{255,10,10,12}`
- Panel: `#141418` — `IColor{255,20,20,24}`
- Accent (orange): `#FF5500` — `IColor{255,255,85,0}` (`kColAccent`)
- OSC section: blue `IColor{255,70,140,255}`
- Filter: orange (same as accent)
- Env: green `IColor{255,80,210,130}`
- LFO: purple `IColor{255,170,80,255}`
- FX: yellow `IColor{255,255,200,40}`

---

## Preset bank (HALPresets.h)

`kHALPresets[]` — array of `HALPreset` structs, `kNumHALPresets = 10`.

Each preset has a `params[56]` array matching EParams ordering. Presets:
- Not Minimoog (LEAD), Not Jupiter-8 (PAD), Not Prophet-5 (LEAD)
- Not DX7 (LEAD), Not TB-303 (BASS), Not SH-101 (BASS)
- Not Juno-60 (PAD), Not OB-Xa (PAD), Not 808 (808), Not Arp 2600 (ARP)

---

## Export (HALExport.h)

**WAV export (`ExportWAV()`):**
- Renders 3s hold + 2s tail at current sample rate via `renderPatch()` (offline, 4 voices)
- Writes 24-bit stereo WAV with SMPL loop chunk (loopStart = after attack+decay, loopEnd = near release)
- Saves to `~/Desktop/HAL_export_<timestamp>.wav`; reveals in Finder via `open -R`

**Vital export (`ExportVital()`):**
- Builds `.vital` JSON preset from current `HALSynthParams`
- Envelope times stored as `log2(seconds)` (Vital's format)
- Filter cutoff stored as MIDI note units: `12*log2(hz/440)+69`
- Modulation slot: env_2 → filter_1_cutoff with `filterEnvAmount × 36` semitones depth
- Saves to `~/Desktop/HAL_<timestamp>.vital`

---

## Telegram Bot workflow

**Bot:** `@combo_dev_bot` on Telegram  
**Code on VPS:** `/home/platform/telegram-claude/bot.py`  
**VPS-wide context:** `/home/platform/CLAUDE.md`

To direct the bot to work on HAL server code:
```
/home/platform/hal: <task description>
```
Example:
```
/home/platform/hal: add 808 category to SYNTH_SYSTEM_PROMPT with sub-bass perceptual mappings
```

The bot reads `/home/platform/CLAUDE.md` for VPS-level context. For HAL-specific context, the relevant section is in there (or reference this file via the GitHub URL).

---

## Known gotchas

1. **`iPlug2/` is NOT committed** — it's a sibling directory at `../iPlug2/` relative to `HAL/`. The xcconfig references it via `IPLUG2_ROOT = ../../../iPlug2`.

2. **JSON_INC_PATH** — nlohmann/json is included via `EXTRA_INC_PATHS = $(JSON_INC_PATH)` in xcconfig. `#include "json.hpp"` works because of this path.

3. **EQ ProcessBlock direction** — `mEQLow.ProcessBlock(outputs, outputs, 2, nFrames)` passes `outputs` as both in and out (in-place). This is correct for iPlug2's SVF.

4. **No HAL_API_KEY in code** — The key `bd639f6ed47b51a5acfd42960abde05a0a995913e13d092de3fa424c11a545ba` is hardcoded in `HALHTTPClient.mm`. This is intentional for a closed client; rotate it on the server if compromised.

5. **Arp fires voices every `1/rate` seconds** — The arp phase accumulates in ProcessBlock. When arpEnabled=0, the engine is bypassed entirely and normal polyphony applies.

6. **Pitch envelope sustain is 0** — Pitch env always decays to 0 (no sustain), making it a one-shot sweep on note-on. This is by design for 808 kicks.

7. **`system("open -R ...")` in export** — This is macOS-only and fine for the APP build; the call is silent on VST3 when Finder is unavailable (no side-effects).
