#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <fstream>
#include "HALVoice.h"
#include "HALEffects.h"
#include "LFO.h"
#include "json.hpp"

using json = nlohmann::json;

// ──────────────────────────────────────────────────────────────────────────────
// WAV writer with SMPL loop chunk for sampler compatibility
// ──────────────────────────────────────────────────────────────────────────────

static inline void writeU32LE(std::ofstream& f, uint32_t v)
{
  uint8_t b[4] = { (uint8_t)v, (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24) };
  f.write((char*)b, 4);
}

static inline void writeU16LE(std::ofstream& f, uint16_t v)
{
  uint8_t b[2] = { (uint8_t)v, (uint8_t)(v>>8) };
  f.write((char*)b, 2);
}

static inline void writeTag(std::ofstream& f, const char* tag)
{
  f.write(tag, 4);
}

// Writes a 24-bit stereo WAV with an SMPL chunk marking the loop region.
// loopStart/loopEnd are sample-frame indices (0-based).
// If loopStart >= loopEnd, no SMPL chunk is written.
static bool writeWAV(const std::string& path,
                     const std::vector<float>& L,
                     const std::vector<float>& R,
                     double sampleRate,
                     uint32_t loopStart,
                     uint32_t loopEnd)
{
  std::ofstream f(path, std::ios::binary);
  if (!f.is_open()) return false;

  const uint32_t nFrames    = (uint32_t)L.size();
  const uint32_t nChans     = 2;
  const uint32_t bitDepth   = 24;
  const uint32_t byteDepth  = 3;
  const uint32_t sr         = (uint32_t)sampleRate;
  const uint32_t dataSize   = nFrames * nChans * byteDepth;

  const bool hasLoop = (loopEnd > loopStart) && (loopEnd <= nFrames);

  // SMPL chunk: 36 + 24 per loop = 60 bytes for one loop
  const uint32_t smplSize = hasLoop ? 60u : 0u;
  const uint32_t smplChunkTotal = hasLoop ? (8 + smplSize) : 0u;

  // RIFF size = 4 (WAVE) + fmt chunk (8+16) + data chunk (8+dataSize) + smpl
  const uint32_t riffSize = 4 + 24 + 8 + dataSize + smplChunkTotal;

  // RIFF header
  writeTag(f, "RIFF");
  writeU32LE(f, riffSize);
  writeTag(f, "WAVE");

  // fmt chunk
  writeTag(f, "fmt ");
  writeU32LE(f, 16);
  writeU16LE(f, 1);                            // PCM
  writeU16LE(f, (uint16_t)nChans);
  writeU32LE(f, sr);
  writeU32LE(f, sr * nChans * byteDepth);      // byte rate
  writeU16LE(f, (uint16_t)(nChans * byteDepth)); // block align
  writeU16LE(f, (uint16_t)bitDepth);

  // data chunk
  writeTag(f, "data");
  writeU32LE(f, dataSize);
  for (uint32_t i = 0; i < nFrames; ++i)
  {
    for (int ch = 0; ch < 2; ++ch)
    {
      const float s   = ch == 0 ? L[i] : R[i];
      const float sc  = s < -1.f ? -1.f : s > 1.f ? 1.f : s;
      const int32_t v = (int32_t)(sc * 8388607.f);
      f.put((char)(v & 0xFF));
      f.put((char)((v >> 8) & 0xFF));
      f.put((char)((v >> 16) & 0xFF));
    }
  }

  // SMPL chunk (loop points)
  if (hasLoop)
  {
    // Manufacturer MIDI note: C3 = 60, use middle C
    const uint32_t midiNote = 60;
    writeTag(f, "smpl");
    writeU32LE(f, smplSize);
    writeU32LE(f, 0);          // manufacturer
    writeU32LE(f, 0);          // product
    writeU32LE(f, (uint32_t)(1000000000.0 / sampleRate)); // sample period ns
    writeU32LE(f, midiNote);   // MIDI unity note
    writeU32LE(f, 0);          // pitch fraction
    writeU32LE(f, 0);          // SMPTE format
    writeU32LE(f, 0);          // SMPTE offset
    writeU32LE(f, 1);          // num sample loops
    writeU32LE(f, 0);          // sampler data bytes

    // Loop record
    writeU32LE(f, 0);          // cue point ID
    writeU32LE(f, 0);          // type: forward loop
    writeU32LE(f, loopStart);
    writeU32LE(f, loopEnd);
    writeU32LE(f, 0);          // fraction
    writeU32LE(f, 0);          // play count (infinite)
  }

  return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Offline render: hold a note for `holdMs` ms, release, let tail decay
// Returns interleaved stereo samples (L[0], R[0], L[1], R[1], ...)
// Actually returns L and R separately.
// ──────────────────────────────────────────────────────────────────────────────
static bool renderPatch(const HALSynthParams& params,
                        int midiNote,
                        double holdMs,
                        double tailMs,
                        double sampleRate,
                        std::vector<float>& outL,
                        std::vector<float>& outR,
                        uint32_t& loopStart,
                        uint32_t& loopEnd)
{
  const int holdFrames = (int)(holdMs * 0.001 * sampleRate);
  const int tailFrames = (int)(tailMs * 0.001 * sampleRate);
  const int totalFrames = holdFrames + tailFrames;

  outL.resize(totalFrames, 0.f);
  outR.resize(totalFrames, 0.f);

  // Use first 4 voices for unison thickness in offline render
  static constexpr int kRenderVoices = 4;
  HALVoice voices[kRenderVoices];
  for (int v = 0; v < kRenderVoices; ++v)
    voices[v].Init(sampleRate);

  // Detune voices slightly for thickness
  HALSynthParams p = params;
  for (int v = 0; v < kRenderVoices; ++v)
    voices[v].NoteOn(midiNote, 0.9, p, v);

  iplug::LFO<double> lfo[2];
  for (int li = 0; li < 2; ++li) {
    lfo[li].SetSampleRate(sampleRate);
    lfo[li].SetPolarity(true);
    lfo[li].Reset();
  }

  using LFOShape = iplug::LFO<double>::EShape;
  static const LFOShape shapeMap[5] = {
    LFOShape::kSine, LFOShape::kTriangle, LFOShape::kRampUp,
    LFOShape::kSquare, LFOShape::kRampUp
  };
  for (int li = 0; li < 2; ++li)
    lfo[li].SetShape((int)shapeMap[std::max(0, std::min(p.lfo[li].shape, 4))]);

  HALChorus chorus; HALReverb reverb; HALDelay delay;
  chorus.Init(sampleRate); reverb.Init(sampleRate); delay.Init(sampleRate);

  double lfoMod[2] = {0.0, 0.0};

  // Find loop region: attack+decay portion, then loop from just after
  const double attackMs  = p.ampAttack;
  const double decayMs   = p.ampDecay;
  loopStart = (uint32_t)((attackMs + decayMs) * 0.001 * sampleRate);
  loopEnd   = (uint32_t)(holdFrames - 100); // a little before release
  if (loopStart >= loopEnd) { loopStart = 0; loopEnd = 0; }

  // Render
  std::vector<double> tmpL(totalFrames), tmpR(totalFrames);

  for (int s = 0; s < totalFrames; ++s)
  {
    if (s == holdFrames)
      for (int v = 0; v < kRenderVoices; ++v)
        voices[v].NoteOff();

    for (int li = 0; li < 2; ++li)
      lfoMod[li] = lfo[li].Process(p.lfo[li].rate);

    double mono = 0.0;
    for (int v = 0; v < kRenderVoices; ++v)
      mono += voices[v].Process(p, lfoMod);
    mono *= 0.25;

    tmpL[s] = tmpR[s] = mono;
  }

  // Effects
  if (p.satDrive > 0.001)
    for (int s = 0; s < totalFrames; ++s) {
      tmpL[s] = halSoftSat(tmpL[s], p.satDrive);
      tmpR[s] = halSoftSat(tmpR[s], p.satDrive);
    }

  chorus.Process(tmpL.data(), tmpR.data(), totalFrames, p.chorusDepth, p.chorusMix);
  reverb.Process(tmpL.data(), tmpR.data(), totalFrames, p.reverbSize,  p.reverbMix);
  delay.Process (tmpL.data(), tmpR.data(), totalFrames, p.delayTime,   p.delayFeedback, p.delayMix);

  // Normalise
  double peak = 0.0;
  for (int s = 0; s < totalFrames; ++s) {
    peak = std::max(peak, std::abs(tmpL[s]));
    peak = std::max(peak, std::abs(tmpR[s]));
  }
  const double gain = (peak > 0.001) ? 0.95 / peak : 1.0;
  for (int s = 0; s < totalFrames; ++s) {
    outL[s] = (float)(tmpL[s] * gain);
    outR[s] = (float)(tmpR[s] * gain);
  }
  return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Vital .vital preset JSON builder
// Wave frames: 0=sine, ~85=triangle, ~171=square, ~256=saw (approx positions)
// ──────────────────────────────────────────────────────────────────────────────
static double halWaveFrame(int waveform)
{
  // Vital wavetable: sine at 0, triangle ~85, square ~171, saw ~0 (back is saw)
  // Use broadly accepted community mappings
  switch (waveform) {
    case 0: return 0.0;    // sine
    case 1: return 0.0;    // saw — frame 0 but Vital treats first wt as sine; use a different slot
    case 2: return 116.64; // square (observed in presets)
    case 3: return 46.55;  // triangle (observed in presets)
    default: return 0.0;
  }
}

// Vital envelope time: stored as log2(seconds), seconds = 2^value
static double msToVitalEnv(double ms)
{
  const double sec = ms * 0.001;
  return (sec > 0.0) ? std::log2(sec) : -10.0;
}

static std::string buildVitalPreset(const std::string& name, const HALSynthParams& p)
{
  json settings;

  // Oscillators
  const char* oscNames[] = {"osc_1", "osc_2", "osc_3"};
  for (int i = 0; i < 3; ++i) {
    const auto& o = p.osc[i];
    settings[std::string(oscNames[i]) + "_on"]         = (o.volume > 0.01) ? 1.0 : 0.0;
    settings[std::string(oscNames[i]) + "_wave_frame"]  = halWaveFrame(o.waveform);
    settings[std::string(oscNames[i]) + "_transpose"]   = (double)(o.octave * 12);
    settings[std::string(oscNames[i]) + "_tune"]        = o.fineTune / 100.0; // semitones
    settings[std::string(oscNames[i]) + "_unison_voices"] = (double)o.unisonVoices;
    settings[std::string(oscNames[i]) + "_unison_detune"] = o.unisonSpread * 0.5;
    settings[std::string(oscNames[i]) + "_level"]       = o.volume;
  }

  // Filter — map to Vital's Analog (Sallen-Key) ladder
  settings["filter_1_on"]     = 1.0;
  settings["filter_1_model"]  = 0.0; // Analog
  // LP12=0→style 0, LP24=1→style 1, HP=2→style 2, BP=3→style 3
  settings["filter_1_style"]  = (double)std::min(p.filterType, 3);
  // Vital uses MIDI note units for cutoff: note = 12*log2(hz/440)+69
  const double cutMidi = 12.0 * std::log2(p.filterCutoff / 440.0) + 69.0;
  settings["filter_1_cutoff"] = cutMidi;
  settings["filter_1_resonance"] = p.filterResonance;
  settings["filter_1_drive"]     = p.filterDrive * 30.0; // dB-ish

  // Amp envelope → env_1 (standard mapping in Vital)
  settings["env_1_attack"]  = msToVitalEnv(p.ampAttack);
  settings["env_1_decay"]   = msToVitalEnv(p.ampDecay);
  settings["env_1_sustain"] = p.ampSustain;
  settings["env_1_release"] = msToVitalEnv(p.ampRelease);

  // Filter envelope → env_2
  settings["env_2_attack"]  = msToVitalEnv(p.filterAttack);
  settings["env_2_decay"]   = msToVitalEnv(p.filterDecay);
  settings["env_2_sustain"] = p.filterSustain;
  settings["env_2_release"] = msToVitalEnv(p.filterRelease);

  // LFOs
  const char* lfoVitalNames[] = {"lfo_1", "lfo_2"};
  // Vital LFO phase: sine→0, triangle→0.25, square→0.5, saw→0.75
  static const double lfoPhaseMap[5] = {0.0, 0.25, 0.75, 0.5, 0.0};
  for (int li = 0; li < 2; ++li) {
    const auto& l = p.lfo[li];
    int sh = std::max(0, std::min(l.shape, 4));
    settings[std::string(lfoVitalNames[li]) + "_phase"]      = lfoPhaseMap[sh];
    settings[std::string(lfoVitalNames[li]) + "_frequency"]  = l.rate;
    settings[std::string(lfoVitalNames[li]) + "_sync_type"]  = 0.0;
  }

  // Effects
  settings["chorus_on"]       = (p.chorusMix > 0.01) ? 1.0 : 0.0;
  settings["chorus_mix"]      = p.chorusMix;
  settings["chorus_depth"]    = p.chorusDepth;
  settings["reverb_on"]       = (p.reverbMix > 0.01) ? 1.0 : 0.0;
  settings["reverb_dry_wet"]  = p.reverbMix;
  settings["reverb_size"]     = p.reverbSize;
  settings["delay_on"]        = (p.delayMix > 0.01) ? 1.0 : 0.0;
  settings["delay_dry_wet"]   = p.delayMix;
  settings["delay_feedback"]  = p.delayFeedback;
  // Vital stores delay tempo in beats; approximate from ms (assume 120bpm)
  settings["delay_tempo"]     = 9.0; // 1/8 note

  // Modulations: env_2 → filter_1_cutoff
  json modulations = json::array();
  if (std::abs(p.filterEnvAmount) > 0.01) {
    json m;
    m["source"]      = "env_2";
    m["destination"] = "filter_1_cutoff";
    modulations.push_back(m);
    // Slot 1 amount
    settings["modulation_1_amount"] = p.filterEnvAmount * 36.0; // scale to MIDI semitones
    settings["modulation_1_bipolar"] = 1.0;
    settings["modulation_1_bypass"]  = 0.0;
    settings["modulation_1_power"]   = 1.0;
    settings["modulation_1_stereo"]  = 0.0;
  }
  settings["modulations"] = modulations;

  json root;
  root["preset_name"]   = name;
  root["author"]        = "HAL";
  root["comments"]      = "Generated by HAL — natural language synthesizer";
  root["preset_style"]  = "";
  root["synth_version"] = "1.5.5";
  root["settings"]      = settings;

  return root.dump(2);
}
