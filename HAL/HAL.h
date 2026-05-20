#pragma once
#include "IPlug_include_in_plug_hdr.h"

const int kNumPresets = 1;

enum EParams
{
  // OSC 1
  kOsc1Waveform = 0,
  kOsc1Octave,
  kOsc1FineTune,
  kOsc1UnisonVoices,
  kOsc1UnisonSpread,
  kOsc1Volume,

  // OSC 2
  kOsc2Waveform,
  kOsc2Octave,
  kOsc2FineTune,
  kOsc2UnisonVoices,
  kOsc2UnisonSpread,
  kOsc2Volume,

  // OSC 3
  kOsc3Waveform,
  kOsc3Octave,
  kOsc3FineTune,
  kOsc3UnisonVoices,
  kOsc3UnisonSpread,
  kOsc3Volume,

  // Filter
  kFilterType,
  kFilterCutoff,
  kFilterResonance,
  kFilterEnvAmount,
  kFilterDrive,

  // Filter Envelope
  kFilterAttack,
  kFilterDecay,
  kFilterSustain,
  kFilterRelease,

  // Amp Envelope
  kAmpAttack,
  kAmpDecay,
  kAmpSustain,
  kAmpRelease,

  // LFO 1
  kLFO1Shape,
  kLFO1Rate,
  kLFO1Depth,
  kLFO1Target,

  // LFO 2
  kLFO2Shape,
  kLFO2Rate,
  kLFO2Depth,
  kLFO2Target,

  // Effects
  kChorusDepth,
  kChorusMix,
  kReverbSize,
  kReverbMix,
  kDelayTime,
  kDelayFeedback,
  kDelayMix,
  kSaturationDrive,
  kEQLowShelf,
  kEQHighShelf,

  kNumParams
};

enum EControlTags
{
  kCtrlTagPromptInput = 0,
  kCtrlTagGenerateButton,
  kCtrlTagStyleHint,
  kCtrlTagStatus,
  kCtrlTagHistory,
  kNumCtrlTags
};

class HAL final : public iplug::Plugin
{
public:
  HAL(const iplug::InstanceInfo& info);
  void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;

private:
  void GeneratePatch(const char* description, const char* styleHint);
  void ApplyPatchJSON(const char* jsonStr);

  double mSampleRate = 44100.0;
};
