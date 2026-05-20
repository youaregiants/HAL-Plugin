#pragma once
#include <cstring>

// Preset parameter values — indices match EParams ordering in HAL.h
// Layout: [osc1×6][osc2×6][osc3×6][filter×5][fEnv×4][ampEnv×4][lfo1×4][lfo2×4][fx×10][pitchEnv×3][arp×4]
// Total: 56 params

struct HALPreset
{
  const char* name;
  const char* category;   // PAD/BASS/LEAD/STAB/808/ARP/FX
  const char* description;
  double      params[56]; // kNumParams
};

// osc: waveform(0-3), octave(-2..2), fine, unisonVoices, unisonSpread, volume
// filter: type(0-4), cutoff, resonance, envAmt, drive
// fEnv: atk, dec, sus, rel (ms, ms, 0-1, ms)
// ampEnv: atk, dec, sus, rel
// lfo1: shape, rate, depth, target
// lfo2: shape, rate, depth, target
// fx: chorusDepth, chorusMix, reverbSize, reverbMix, delayTime, delayFeedback, delayMix, satDrive, eqLow, eqHigh
// pitchEnv: amount(semitones), attack(ms), decay(ms)
// arp: enabled(0/1), rate(hz), pattern(0=up,1=down,2=updown,3=random), octaves

static const HALPreset kHALPresets[] =
{
  // ── "Not Minimoog" — fat monophonic lead/bass ────────────────────────────
  {
    "Not Minimoog", "LEAD",
    "Three sawtooth oscillators, ladder-style LP24, punchy amp env",
    {
    // osc1: saw, oct 0, fine 0, unison 1, spread 0, vol 0.9
    1, 0, 0.0, 1, 0.0, 0.9,
    // osc2: saw, oct 0, fine -7, unison 1, spread 0, vol 0.8
    1, 0, -7.0, 1, 0.0, 0.8,
    // osc3: saw, oct -1, fine 0, unison 1, spread 0, vol 0.6
    1, -1, 0.0, 1, 0.0, 0.6,
    // filter: lp24(1), cutoff 1200, res 0.35, envAmt 0.6, drive 0.3
    1, 1200.0, 0.35, 0.6, 0.3,
    // fEnv: atk 5, dec 400, sus 0.2, rel 300
    5.0, 400.0, 0.2, 300.0,
    // ampEnv: atk 8, dec 100, sus 0.9, rel 150
    8.0, 100.0, 0.9, 150.0,
    // lfo1: sine, 4hz, depth 0.05, target filter_cutoff(1)
    0, 4.0, 0.05, 1,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: chorus 0/0, reverb 0.3/0.05, delay 0/0/0, sat 0.2, eq -2/+3
    0.0, 0.0, 0.3, 0.05, 250.0, 0.0, 0.0, 0.2, -2.0, 3.0,
    // pitchEnv: 0 amt, 2ms, 50ms
    0.0, 2.0, 50.0,
    // arp: off, 4hz, up, 1oct
    0, 4.0, 0, 1
    }
  },

  // ── "Not Jupiter-8" — lush poly pad ─────────────────────────────────────
  {
    "Not Jupiter-8", "PAD",
    "Detuned unison saws, warm LP, slow attack, chorus+reverb",
    {
    // osc1: saw, oct 0, fine 0, unison 4, spread 0.4, vol 0.85
    1, 0, 0.0, 4, 0.4, 0.85,
    // osc2: saw, oct 0, fine +12, unison 4, spread 0.35, vol 0.6
    1, 0, 12.0, 4, 0.35, 0.6,
    // osc3: sine, oct -1, fine 0, unison 1, spread 0, vol 0.4
    0, -1, 0.0, 1, 0.0, 0.4,
    // filter: lp12(0), cutoff 2500, res 0.1, envAmt 0.2, drive 0.0
    0, 2500.0, 0.1, 0.2, 0.0,
    // fEnv: atk 800, dec 1200, sus 0.5, rel 1500
    800.0, 1200.0, 0.5, 1500.0,
    // ampEnv: atk 600, dec 800, sus 0.8, rel 1800
    600.0, 800.0, 0.8, 1800.0,
    // lfo1: sine, 0.3hz, depth 0.15, target osc_pitch(0)
    0, 0.3, 0.15, 0,
    // lfo2: sine, 0.5hz, depth 0.1, target filter_cutoff(1)
    0, 0.5, 0.1, 1,
    // fx: chorus 0.6/0.4, reverb 0.7/0.35, delay 0/0/0, sat 0, eq +1/+2
    0.6, 0.4, 0.7, 0.35, 250.0, 0.0, 0.0, 0.0, 1.0, 2.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not Prophet-5" — classic poly lead ──────────────────────────────────
  {
    "Not Prophet-5", "LEAD",
    "Saw + square mix, resonant LP, medium attack",
    {
    // osc1: saw, oct 0, fine 0, unison 1, spread 0, vol 0.85
    1, 0, 0.0, 1, 0.0, 0.85,
    // osc2: square, oct 0, fine +5, unison 1, spread 0, vol 0.7
    2, 0, 5.0, 1, 0.0, 0.7,
    // osc3: off
    1, -1, 0.0, 1, 0.0, 0.0,
    // filter: lp24(1), cutoff 3000, res 0.45, envAmt 0.5, drive 0.1
    1, 3000.0, 0.45, 0.5, 0.1,
    // fEnv: atk 30, dec 500, sus 0.3, rel 400
    30.0, 500.0, 0.3, 400.0,
    // ampEnv: atk 20, dec 300, sus 0.75, rel 500
    20.0, 300.0, 0.75, 500.0,
    // lfo1: tri, 5hz, 0.06 depth, osc_pitch(0)
    1, 5.0, 0.06, 0,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: chorus 0.3/0.2, reverb 0.4/0.12, delay 0/0/0, sat 0.05, eq 0/+1
    0.3, 0.2, 0.4, 0.12, 250.0, 0.0, 0.0, 0.05, 0.0, 1.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not DX7" — FM-style electric piano ──────────────────────────────────
  {
    "Not DX7", "LEAD",
    "Sine-heavy attack transient, bright decay, clean no-fx tone",
    {
    // osc1: sine, oct 0, fine 0, unison 1, spread 0, vol 0.9
    0, 0, 0.0, 1, 0.0, 0.9,
    // osc2: sine, oct +1, fine +2, unison 1, spread 0, vol 0.5
    0, 1, 2.0, 1, 0.0, 0.5,
    // osc3: triangle, oct 0, fine +701(perfect 5th cents), unison 1, spread 0, vol 0.2
    3, 0, 2.0, 1, 0.0, 0.2,
    // filter: hp12(2), cutoff 200, res 0.0, envAmt 0.0, drive 0.0
    2, 200.0, 0.0, 0.0, 0.0,
    // fEnv: flat
    5.0, 300.0, 0.9, 400.0,
    // ampEnv: atk 2, dec 800, sus 0.4, rel 600
    2.0, 800.0, 0.4, 600.0,
    // lfo1: off
    0, 5.0, 0.0, 1,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: no chorus, tiny reverb, no delay, no sat, eq +0/+0
    0.0, 0.0, 0.25, 0.05, 250.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not TB-303" — acid bass ─────────────────────────────────────────────
  {
    "Not TB-303", "BASS",
    "Sawtooth, screaming resonant LP24, short punchy amp env, pitch glide via slow arp",
    {
    // osc1: saw, oct -1, fine 0, unison 1, spread 0, vol 1.0
    1, -1, 0.0, 1, 0.0, 1.0,
    // osc2: off
    1, -1, 0.0, 1, 0.0, 0.0,
    // osc3: off
    1, -1, 0.0, 1, 0.0, 0.0,
    // filter: lp24(1), cutoff 600, res 0.85, envAmt 0.9, drive 0.3
    1, 600.0, 0.85, 0.9, 0.3,
    // fEnv: atk 3, dec 200, sus 0.0, rel 200
    3.0, 200.0, 0.0, 200.0,
    // ampEnv: atk 2, dec 300, sus 0.6, rel 100
    2.0, 300.0, 0.6, 100.0,
    // lfo1: off
    0, 1.0, 0.0, 1,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: no chorus, small reverb, delay 1/16 at 120bpm=125ms with 0.3fb, sat 0.25, eq +2/-1
    0.0, 0.0, 0.2, 0.05, 125.0, 0.3, 0.15, 0.25, 2.0, -1.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not SH-101" — mono synth bass/lead ─────────────────────────────────
  {
    "Not SH-101", "BASS",
    "Pulse wave with PWM via LFO, resonant LP, classic mono synth tone",
    {
    // osc1: square, oct -1, fine 0, unison 1, spread 0, vol 0.95
    2, -1, 0.0, 1, 0.0, 0.95,
    // osc2: saw, oct -1, fine -5, unison 1, spread 0, vol 0.4
    1, -1, -5.0, 1, 0.0, 0.4,
    // osc3: off
    1, -2, 0.0, 1, 0.0, 0.0,
    // filter: lp24(1), cutoff 900, res 0.4, envAmt 0.5, drive 0.1
    1, 900.0, 0.4, 0.5, 0.1,
    // fEnv: atk 5, dec 300, sus 0.1, rel 250
    5.0, 300.0, 0.1, 250.0,
    // ampEnv: atk 5, dec 150, sus 0.8, rel 200
    5.0, 150.0, 0.8, 200.0,
    // lfo1: square, 1.5hz, 0.15 depth, osc_volume(2) — simulates PWM
    3, 1.5, 0.15, 2,
    // lfo2: sine, 0.4hz, 0.08 depth, filter_cutoff
    0, 0.4, 0.08, 1,
    // fx: no chorus, room reverb, no delay, slight sat, eq +1/+0
    0.0, 0.0, 0.3, 0.08, 250.0, 0.0, 0.0, 0.1, 1.0, 0.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not Juno-60" — chorus pad ───────────────────────────────────────────
  {
    "Not Juno-60", "PAD",
    "Sawtooth with DCO character, signature chorus, warm LP",
    {
    // osc1: saw, oct 0, fine 0, unison 2, spread 0.08, vol 0.9
    1, 0, 0.0, 2, 0.08, 0.9,
    // osc2: square, oct 0, fine +3, unison 1, spread 0, vol 0.4
    2, 0, 3.0, 1, 0.0, 0.4,
    // osc3: sine, oct -1, fine 0, unison 1, spread 0, vol 0.3
    0, -1, 0.0, 1, 0.0, 0.3,
    // filter: lp12(0), cutoff 1800, res 0.2, envAmt 0.25, drive 0.0
    0, 1800.0, 0.2, 0.25, 0.0,
    // fEnv: atk 300, dec 600, sus 0.5, rel 700
    300.0, 600.0, 0.5, 700.0,
    // ampEnv: atk 200, dec 400, sus 0.85, rel 900
    200.0, 400.0, 0.85, 900.0,
    // lfo1: sine, 0.6hz, 0.1 depth, osc_pitch(0)
    0, 0.6, 0.1, 0,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: chorus 0.7/0.5, reverb 0.5/0.2, delay 0/0/0, sat 0, eq +1/+2
    0.7, 0.5, 0.5, 0.2, 250.0, 0.0, 0.0, 0.0, 1.0, 2.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not OB-Xa" — super-wide poly pad ───────────────────────────────────
  {
    "Not OB-Xa", "PAD",
    "8-voice super saw, enormous unison, reverb wall",
    {
    // osc1: saw, oct 0, fine 0, unison 8, spread 0.8, vol 0.85
    1, 0, 0.0, 8, 0.8, 0.85,
    // osc2: saw, oct 0, fine +5, unison 4, spread 0.5, vol 0.55
    1, 0, 5.0, 4, 0.5, 0.55,
    // osc3: sine, oct -1, fine 0, unison 1, spread 0, vol 0.35
    0, -1, 0.0, 1, 0.0, 0.35,
    // filter: lp24(1), cutoff 2200, res 0.15, envAmt 0.15, drive 0.05
    1, 2200.0, 0.15, 0.15, 0.05,
    // fEnv: atk 1000, dec 1500, sus 0.4, rel 2000
    1000.0, 1500.0, 0.4, 2000.0,
    // ampEnv: atk 800, dec 1000, sus 0.9, rel 2500
    800.0, 1000.0, 0.9, 2500.0,
    // lfo1: sine, 0.2hz, 0.2 depth, osc_pitch(0)
    0, 0.2, 0.2, 0,
    // lfo2: sine, 0.35hz, 0.12 depth, filter_cutoff(1)
    0, 0.35, 0.12, 1,
    // fx: chorus 0.8/0.5, reverb 0.85/0.5, delay 0/0/0, sat 0.05, eq +2/+3
    0.8, 0.5, 0.85, 0.5, 250.0, 0.0, 0.0, 0.05, 2.0, 3.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not 808" — sub kick with pitch sweep ───────────────────────────────
  {
    "Not 808", "808",
    "Sine sub with pitch envelope drop, punchy transient, long decay",
    {
    // osc1: sine, oct -1, fine 0, unison 1, spread 0, vol 1.0
    0, -1, 0.0, 1, 0.0, 1.0,
    // osc2: sine, oct -1, fine 0, unison 1, spread 0, vol 0.3 (body reinforcement)
    0, -1, 0.0, 1, 0.0, 0.3,
    // osc3: off
    0, -2, 0.0, 1, 0.0, 0.0,
    // filter: lp12(0), cutoff 300, res 0.0, envAmt 0.0, drive 0.15
    0, 300.0, 0.0, 0.0, 0.15,
    // fEnv: flat
    2.0, 100.0, 0.9, 200.0,
    // ampEnv: atk 2, dec 1500, sus 0.0, rel 800
    2.0, 1500.0, 0.0, 800.0,
    // lfo1: off
    0, 1.0, 0.0, 1,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: no chorus, no reverb, no delay, slight sat, eq +3/-1
    0.0, 0.0, 0.0, 0.0, 250.0, 0.0, 0.0, 0.2, 3.0, -1.0,
    // pitchEnv: 24 semitones, 2ms attack, 300ms decay
    24.0, 2.0, 300.0,
    // arp: off
    0, 4.0, 0, 1
    }
  },

  // ── "Not Arp 2600" — arp sequence ───────────────────────────────────────
  {
    "Not Arp 2600", "ARP",
    "Saw with resonant filter, 8th-note up arp over 2 octaves",
    {
    // osc1: saw, oct 0, fine 0, unison 1, spread 0, vol 0.9
    1, 0, 0.0, 1, 0.0, 0.9,
    // osc2: saw, oct 0, fine +7, unison 1, spread 0, vol 0.4
    1, 0, 7.0, 1, 0.0, 0.4,
    // osc3: off
    1, -1, 0.0, 1, 0.0, 0.0,
    // filter: lp24(1), cutoff 2000, res 0.55, envAmt 0.7, drive 0.1
    1, 2000.0, 0.55, 0.7, 0.1,
    // fEnv: atk 5, dec 200, sus 0.1, rel 200
    5.0, 200.0, 0.1, 200.0,
    // ampEnv: atk 3, dec 200, sus 0.6, rel 150
    3.0, 200.0, 0.6, 150.0,
    // lfo1: sine, 3hz, 0.1 depth, filter_cutoff(1)
    0, 3.0, 0.1, 1,
    // lfo2: off
    0, 1.0, 0.0, 1,
    // fx: no chorus, reverb 0.35/0.12, delay 375ms/0.4fb/0.25mix, sat 0.1, eq 0/+1
    0.0, 0.0, 0.35, 0.12, 375.0, 0.4, 0.25, 0.1, 0.0, 1.0,
    // pitchEnv: none
    0.0, 2.0, 150.0,
    // arp: enabled(1), 8hz=8th@120bpm, up(0), 2 octaves
    1, 8.0, 0, 2
    }
  },
};

static constexpr int kNumHALPresets = (int)(sizeof(kHALPresets) / sizeof(kHALPresets[0]));
