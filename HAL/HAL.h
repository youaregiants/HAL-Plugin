#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "LFO.h"
#include "SVF.h"
#include "HALVoice.h"
#include "HALEffects.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

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

  // Pitch Envelope (for 808 kicks, stabs, etc.)
  kPitchEnvAmount,
  kPitchEnvAttack,
  kPitchEnvDecay,

  // Arpeggiator
  kArpEnabled,
  kArpRate,
  kArpPattern,
  kArpOctaves,

  kNumParams
};

enum EControlTags
{
  kCtrlTagPromptInput = 0,
  kCtrlTagGenerateButton,
  kCtrlTagStyleHint,
  kCtrlTagStatus,
  kCtrlTagNudgeInput,
  kCtrlTagCategoryTab,
  kCtrlTagExportButton,
  kNumCtrlTags
};

class HAL final : public iplug::Plugin
{
public:
  HAL(const iplug::InstanceInfo& info);

  void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
  void ProcessMidiMsg(const iplug::IMidiMsg& msg) override;
  void OnParamChange(int paramIdx) override;
  void OnReset() override;
  void OnIdle() override;

private:
  void GeneratePatch(const std::string& prompt, const std::string& style,
                     const std::string& nudge = "");
  void ApplyPatchJSON(const std::string& json);
  void SetStatus(const std::string& msg);
  void InitDSP();
  void UpdateSynthParams();
  int  FindFreeVoice();
  std::string BuildCurrentPatchJSON() const;

  double mSampleRate = 44100.0;

  // ── Synthesis engine ────────────────────────────────────────────────────────
  HALSynthParams mSynthParams;

  static constexpr int kNumVoices = 8;
  HALVoice mVoices[kNumVoices];
  int      mVoiceAge = 0;

  iplug::LFO<double> mLFO[2];

  // Master EQ: low shelf + high shelf (stereo, NC=2)
  iplug::SVF<double,2> mEQLow;
  iplug::SVF<double,2> mEQHigh;

  HALChorus mChorus;
  HALReverb mReverb;
  HALDelay  mDelay;

  // ── Thread-safe patch handoff from HTTP callback → OnIdle ──────────────────
  std::mutex          mPatchMutex;
  std::string         mPendingPatch;
  std::string         mPendingStatus;
  std::atomic<bool>   mPatchReady{false};
  std::atomic<bool>   mStatusDirty{false};
  std::atomic<bool>   mGenerating{false};

  // ── UX state ────────────────────────────────────────────────────────────────
  void ExportWAV();
  void ExportVital();

  std::string mLastPrompt;   // last successful description, used by nudge
  std::string mActiveCategory = "PAD";
  int         mAnimTick  = 0;
  int         mAnimFrame = 0;

  // Arpeggiator state
  std::vector<int> mHeldNotes;
  int    mArpIdx        = 0;
  double mArpPhase      = 0.0;
  bool   mArpUp         = true;
};
