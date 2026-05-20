#include "HAL.h"
#include "IPlug_include_in_plug_src.h"

HAL::HAL(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(kNumParams, kNumPresets))
{
  // OSC 1
  GetParam(kOsc1Waveform)->InitInt("Osc1 Waveform", 1, 0, 3, "", 0, "Oscillator 1");
  GetParam(kOsc1Octave)->InitInt("Osc1 Octave", 0, -2, 2, "", 0, "Oscillator 1");
  GetParam(kOsc1FineTune)->InitDouble("Osc1 Fine Tune", 0.0, -100.0, 100.0, 0.1, "cents", 0, "Oscillator 1");
  GetParam(kOsc1UnisonVoices)->InitInt("Osc1 Unison Voices", 1, 1, 8, "", 0, "Oscillator 1");
  GetParam(kOsc1UnisonSpread)->InitDouble("Osc1 Unison Spread", 0.0, 0.0, 1.0, 0.01, "", 0, "Oscillator 1");
  GetParam(kOsc1Volume)->InitDouble("Osc1 Volume", 0.8, 0.0, 1.0, 0.01, "", 0, "Oscillator 1");

  // OSC 2
  GetParam(kOsc2Waveform)->InitInt("Osc2 Waveform", 1, 0, 3, "", 0, "Oscillator 2");
  GetParam(kOsc2Octave)->InitInt("Osc2 Octave", 0, -2, 2, "", 0, "Oscillator 2");
  GetParam(kOsc2FineTune)->InitDouble("Osc2 Fine Tune", 0.0, -100.0, 100.0, 0.1, "cents", 0, "Oscillator 2");
  GetParam(kOsc2UnisonVoices)->InitInt("Osc2 Unison Voices", 1, 1, 8, "", 0, "Oscillator 2");
  GetParam(kOsc2UnisonSpread)->InitDouble("Osc2 Unison Spread", 0.0, 0.0, 1.0, 0.01, "", 0, "Oscillator 2");
  GetParam(kOsc2Volume)->InitDouble("Osc2 Volume", 0.7, 0.0, 1.0, 0.01, "", 0, "Oscillator 2");

  // OSC 3
  GetParam(kOsc3Waveform)->InitInt("Osc3 Waveform", 1, 0, 3, "", 0, "Oscillator 3");
  GetParam(kOsc3Octave)->InitInt("Osc3 Octave", 0, -2, 2, "", 0, "Oscillator 3");
  GetParam(kOsc3FineTune)->InitDouble("Osc3 Fine Tune", 0.0, -100.0, 100.0, 0.1, "cents", 0, "Oscillator 3");
  GetParam(kOsc3UnisonVoices)->InitInt("Osc3 Unison Voices", 1, 1, 8, "", 0, "Oscillator 3");
  GetParam(kOsc3UnisonSpread)->InitDouble("Osc3 Unison Spread", 0.0, 0.0, 1.0, 0.01, "", 0, "Oscillator 3");
  GetParam(kOsc3Volume)->InitDouble("Osc3 Volume", 0.6, 0.0, 1.0, 0.01, "", 0, "Oscillator 3");

  // Filter
  GetParam(kFilterType)->InitInt("Filter Type", 1, 0, 4, "", 0, "Filter");
  GetParam(kFilterCutoff)->InitDouble("Filter Cutoff", 8000.0, 20.0, 20000.0, 1.0, "Hz", 0, "Filter");
  GetParam(kFilterResonance)->InitDouble("Filter Resonance", 0.0, 0.0, 1.0, 0.01, "", 0, "Filter");
  GetParam(kFilterEnvAmount)->InitDouble("Filter Env Amount", 0.0, -1.0, 1.0, 0.01, "", 0, "Filter");
  GetParam(kFilterDrive)->InitDouble("Filter Drive", 0.0, 0.0, 1.0, 0.01, "", 0, "Filter");

  // Filter Envelope
  GetParam(kFilterAttack)->InitDouble("Filter Attack", 10.0, 0.0, 10000.0, 1.0, "ms", 0, "Filter Env");
  GetParam(kFilterDecay)->InitDouble("Filter Decay", 200.0, 0.0, 10000.0, 1.0, "ms", 0, "Filter Env");
  GetParam(kFilterSustain)->InitDouble("Filter Sustain", 0.7, 0.0, 1.0, 0.01, "", 0, "Filter Env");
  GetParam(kFilterRelease)->InitDouble("Filter Release", 300.0, 0.0, 10000.0, 1.0, "ms", 0, "Filter Env");

  // Amp Envelope
  GetParam(kAmpAttack)->InitDouble("Amp Attack", 10.0, 0.0, 10000.0, 1.0, "ms", 0, "Amp Env");
  GetParam(kAmpDecay)->InitDouble("Amp Decay", 200.0, 0.0, 10000.0, 1.0, "ms", 0, "Amp Env");
  GetParam(kAmpSustain)->InitDouble("Amp Sustain", 0.7, 0.0, 1.0, 0.01, "", 0, "Amp Env");
  GetParam(kAmpRelease)->InitDouble("Amp Release", 300.0, 0.0, 10000.0, 1.0, "ms", 0, "Amp Env");

  // LFO 1
  GetParam(kLFO1Shape)->InitInt("LFO1 Shape", 0, 0, 4, "", 0, "LFO 1");
  GetParam(kLFO1Rate)->InitDouble("LFO1 Rate", 1.0, 0.01, 20.0, 0.01, "Hz", 0, "LFO 1");
  GetParam(kLFO1Depth)->InitDouble("LFO1 Depth", 0.0, 0.0, 1.0, 0.01, "", 0, "LFO 1");
  GetParam(kLFO1Target)->InitInt("LFO1 Target", 0, 0, 3, "", 0, "LFO 1");

  // LFO 2
  GetParam(kLFO2Shape)->InitInt("LFO2 Shape", 0, 0, 4, "", 0, "LFO 2");
  GetParam(kLFO2Rate)->InitDouble("LFO2 Rate", 1.0, 0.01, 20.0, 0.01, "Hz", 0, "LFO 2");
  GetParam(kLFO2Depth)->InitDouble("LFO2 Depth", 0.0, 0.0, 1.0, 0.01, "", 0, "LFO 2");
  GetParam(kLFO2Target)->InitInt("LFO2 Target", 0, 0, 3, "", 0, "LFO 2");

  // Effects
  GetParam(kChorusDepth)->InitDouble("Chorus Depth", 0.0, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kChorusMix)->InitDouble("Chorus Mix", 0.0, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kReverbSize)->InitDouble("Reverb Size", 0.5, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kReverbMix)->InitDouble("Reverb Mix", 0.0, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kDelayTime)->InitDouble("Delay Time", 250.0, 0.0, 2000.0, 1.0, "ms", 0, "Effects");
  GetParam(kDelayFeedback)->InitDouble("Delay Feedback", 0.0, 0.0, 0.95, 0.01, "", 0, "Effects");
  GetParam(kDelayMix)->InitDouble("Delay Mix", 0.0, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kSaturationDrive)->InitDouble("Saturation Drive", 0.0, 0.0, 1.0, 0.01, "", 0, "Effects");
  GetParam(kEQLowShelf)->InitDouble("EQ Low Shelf", 0.0, -12.0, 12.0, 0.1, "dB", 0, "Effects");
  GetParam(kEQHighShelf)->InitDouble("EQ High Shelf", 0.0, -12.0, 12.0, 0.1, "dB", 0, "Effects");
}

void HAL::OnReset()
{
  mSampleRate = GetSampleRate();
}

void HAL::OnParamChange(int paramIdx)
{
  // Parameter changes will be handled by the synthesis engine
}

void HAL::ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames)
{
  // Synthesis engine will be implemented here
  // For now, pass silence
  const int nChans = NOutChansConnected();
  for (int c = 0; c < nChans; c++)
    for (int s = 0; s < nFrames; s++)
      outputs[c][s] = 0.0;
}

void HAL::GeneratePatch(const char* description, const char* styleHint)
{
  // HTTP call to middleware will be implemented here
}

void HAL::ApplyPatchJSON(const char* jsonStr)
{
  // JSON parsing and parameter application will be implemented here
}
