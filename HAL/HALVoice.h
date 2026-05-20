#pragma once
#include <cmath>
#include <cstring>
#include <algorithm>
#include "ADSREnvelope.h"
#include "SVF.h"

static constexpr int    kHALMaxUnison = 8;
static constexpr double kHALTwoPi    = 6.283185307179586;

// ──────────────────────────────────────────────────────────────────────────────
// HALSynthParams — shared between audio thread (read) and main thread (write)
// ──────────────────────────────────────────────────────────────────────────────
struct HALSynthParams
{
  struct OscParams {
    int    waveform     = 1;    // 0=sine,1=saw,2=square,3=triangle
    int    octave       = 0;    // -2..+2
    double fineTune     = 0.0;  // cents
    int    unisonVoices = 1;    // 1..8
    double unisonSpread = 0.0;  // 0..1
    double volume       = 0.8;  // 0..1
  } osc[3];

  int    filterType      = 0;       // 0=lp12,1=lp24,2=hp12,3=bp,4=notch
  double filterCutoff    = 8000.0;  // Hz
  double filterResonance = 0.0;     // 0..1
  double filterEnvAmount = 0.0;     // -1..+1
  double filterDrive     = 0.0;     // 0..1

  double filterAttack  = 10.0;   // ms
  double filterDecay   = 200.0;  // ms
  double filterSustain = 0.7;    // 0..1
  double filterRelease = 300.0;  // ms

  double ampAttack  = 10.0;   // ms
  double ampDecay   = 200.0;  // ms
  double ampSustain = 0.7;    // 0..1
  double ampRelease = 300.0;  // ms

  struct LFOParams {
    int    shape  = 0;   // 0=sine,1=tri,2=saw,3=sq,4=random
    double rate   = 1.0; // Hz
    double depth  = 0.0; // 0..1
    int    target = 1;   // 0=osc_pitch,1=filter_cutoff,2=osc_volume,3=pan
  } lfo[2];

  // Pitch envelope (for 808 kicks, stabs)
  double pitchEnvAmount = 0.0;  // semitones (0..48)
  double pitchEnvAttack = 2.0;  // ms
  double pitchEnvDecay  = 150.0; // ms

  // Effects (read in ProcessBlock, not in voice)
  double chorusDepth   = 0.0;
  double chorusMix     = 0.0;
  double reverbSize    = 0.5;
  double reverbMix     = 0.0;
  double delayTime     = 250.0; // ms
  double delayFeedback = 0.0;
  double delayMix      = 0.0;
  double satDrive      = 0.0;
  double eqLow         = 0.0;   // dB
  double eqHigh        = 0.0;   // dB
};

// ──────────────────────────────────────────────────────────────────────────────
// HALVoice — one polyphonic voice: 3 osc banks, SVF filter, ADSR envelopes
// ──────────────────────────────────────────────────────────────────────────────
class HALVoice
{
public:
  void Init(double sampleRate)
  {
    mSampleRate = sampleRate;
    mAmpEnv.SetSampleRate(sampleRate);
    mFilterEnv.SetSampleRate(sampleRate);
    mPitchEnv.SetSampleRate(sampleRate);
    mFilter.SetSampleRate(sampleRate);
    mFilter2.SetSampleRate(sampleRate);
    mFilter.Reset();
    mFilter2.Reset();
    memset(mPhases, 0, sizeof(mPhases));
    mNote   = -1;
    mActive = false;
    mAge    = 0;
  }

  void UpdateEnvTimes(const HALSynthParams& p)
  {
    using Env = iplug::ADSREnvelope<double>;
    mAmpEnv.SetStageTime(Env::kAttack,  p.ampAttack);
    mAmpEnv.SetStageTime(Env::kDecay,   p.ampDecay);
    mAmpEnv.SetStageTime(Env::kRelease, p.ampRelease);
    mFilterEnv.SetStageTime(Env::kAttack,  p.filterAttack);
    mFilterEnv.SetStageTime(Env::kDecay,   p.filterDecay);
    mFilterEnv.SetStageTime(Env::kRelease, p.filterRelease);
    mPitchEnv.SetStageTime(Env::kAttack,  p.pitchEnvAttack);
    mPitchEnv.SetStageTime(Env::kDecay,   p.pitchEnvDecay);
    mPitchEnv.SetStageTime(Env::kRelease, 1.0); // immediate on release
  }

  void NoteOn(int note, double velocity, const HALSynthParams& p, int age)
  {
    mNote     = note;
    mVelocity = velocity;
    mActive   = true;
    mAge      = age;

    UpdateEnvTimes(p);

    if (mAmpEnv.GetBusy())    mAmpEnv.Retrigger(1.0);
    else                      mAmpEnv.Start(1.0);
    if (mFilterEnv.GetBusy()) mFilterEnv.Retrigger(1.0);
    else                      mFilterEnv.Start(1.0);
    if (mPitchEnv.GetBusy()) mPitchEnv.Retrigger(1.0);
    else                     mPitchEnv.Start(1.0);

    // Stagger unison starting phases to avoid cancellation at t=0
    for (int o = 0; o < 3; ++o)
      for (int u = 0; u < kHALMaxUnison; ++u)
        mPhases[o][u] = (double)(o * kHALMaxUnison + u) / (3.0 * kHALMaxUnison);
  }

  void NoteOff()
  {
    mAmpEnv.Release();
    mFilterEnv.Release();
  }

  bool IsIdle()   const { return !mAmpEnv.GetBusy(); }
  bool IsActive() const { return mActive; }
  int  GetNote()  const { return mNote; }
  int  GetAge()   const { return mAge; }

  // lfoMod[2]: bipolar -1..+1 LFO values (computed once per sample at plugin level)
  double Process(const HALSynthParams& p, const double lfoMod[2])
  {
    if (!mAmpEnv.GetBusy()) { mActive = false; return 0.0; }

    const double baseFreq = 440.0 * std::pow(2.0, (mNote - 69.0) / 12.0);

    // Pitch envelope (decays from pitchEnvAmount semitones → 0)
    const double pEnv = mPitchEnv.Process(0.0); // sustain=0 → pure decay shape
    double pitchMod = (p.pitchEnvAmount > 0.001)
      ? std::pow(2.0, pEnv * p.pitchEnvAmount / 12.0)
      : 1.0;

    // LFO: osc pitch (target=0)
    for (int li = 0; li < 2; ++li)
      if (p.lfo[li].target == 0)
        pitchMod *= std::pow(2.0, lfoMod[li] * p.lfo[li].depth * 2.0); // ±2 oct

    // Filter envelope
    const double fEnv = mFilterEnv.Process(p.filterSustain);

    // Effective filter cutoff with envelope + LFO modulation
    double cutoff = p.filterCutoff;
    if (std::abs(p.filterEnvAmount) > 0.001)
      cutoff *= std::pow(2.0, p.filterEnvAmount * fEnv * 5.0); // ±5 oct
    for (int li = 0; li < 2; ++li)
      if (p.lfo[li].target == 1) // filter_cutoff
        cutoff *= std::pow(2.0, lfoMod[li] * p.lfo[li].depth * 3.0);
    cutoff = std::max(20.0, std::min(cutoff, 19000.0));

    // LFO: osc volume (target=2)
    double volMod = 1.0;
    for (int li = 0; li < 2; ++li)
      if (p.lfo[li].target == 2)
        volMod = std::max(0.0, 1.0 + lfoMod[li] * p.lfo[li].depth * 0.5);

    // Oscillator mix
    double mixedOsc = 0.0;
    double normSum  = 0.0;
    for (int oi = 0; oi < 3; ++oi)
    {
      const auto& osc = p.osc[oi];
      if (osc.volume < 0.001) continue;
      normSum += osc.volume;

      const double octFactor  = std::pow(2.0, (double)osc.octave);
      const double fineFactor = std::pow(2.0, osc.fineTune / 1200.0);
      const double oscFreq    = baseFreq * octFactor * fineFactor * pitchMod;
      const int    nU         = std::max(1, std::min(osc.unisonVoices, kHALMaxUnison));
      double       uniSum     = 0.0;

      for (int u = 0; u < nU; ++u)
      {
        const double detune = (nU > 1)
          ? ((double)u / (double)(nU - 1) - 0.5) * osc.unisonSpread * 0.1
          : 0.0;
        const double freq   = oscFreq * std::pow(2.0, detune);
        mPhases[oi][u]     += freq / mSampleRate;
        if (mPhases[oi][u] >= 1.0) mPhases[oi][u] -= 1.0;
        const double ph = mPhases[oi][u];

        double s;
        switch (osc.waveform) {
          case 0: s = std::sin(ph * kHALTwoPi);        break; // sine
          case 1: s = 2.0 * ph - 1.0;                  break; // saw
          case 2: s = (ph < 0.5) ? 1.0 : -1.0;         break; // square
          case 3: s = 1.0 - 4.0 * std::abs(ph - 0.5);  break; // triangle
          default: s = 2.0 * ph - 1.0;                 break;
        }
        uniSum += s;
      }
      mixedOsc += (uniSum / nU) * osc.volume;
    }
    if (normSum > 0.001) mixedOsc /= normSum;
    mixedOsc *= volMod;

    // Pre-filter drive
    if (p.filterDrive > 0.001) {
      const double d  = 1.0 + p.filterDrive * 4.0;
      const double td = std::tanh(d);
      if (td > 0.0) mixedOsc = std::tanh(mixedOsc * d) / td;
    }

    // Filter
    const double filtered = applyFilter(mixedOsc, p.filterType, cutoff, p.filterResonance);

    // Amp envelope
    return filtered * mAmpEnv.Process(p.ampSustain);
  }

private:
  double applyFilter(double x, int type, double cutHz, double res)
  {
    using Mode = iplug::SVF<double,1>::EMode;
    const double Q = 0.5 + res * 19.5;

    auto run = [&](iplug::SVF<double,1>& f, Mode mode) -> double {
      f.SetFreqCPS(cutHz);
      f.SetQ(Q);
      f.SetMode(mode);
      f.SetSampleRate(mSampleRate);
      double in  = x;
      double out = 0.0;
      double* pi = &in;
      double* po = &out;
      f.ProcessBlock(&pi, &po, 1, 1);
      return out;
    };

    switch (type) {
      case 0: return run(mFilter, Mode::kLowPass);
      case 1: {                                       // lp24: cascade
        x = run(mFilter, Mode::kLowPass);
        return run(mFilter2, Mode::kLowPass);
      }
      case 2: return run(mFilter, Mode::kHighPass);
      case 3: return run(mFilter, Mode::kBandPass);
      case 4: return run(mFilter, Mode::kNotch);
      default: return x;
    }
  }

  double mSampleRate = 44100.0;
  int    mNote       = -1;
  double mVelocity   = 0.0;
  bool   mActive     = false;
  int    mAge        = 0;

  double mPhases[3][kHALMaxUnison] = {};

  iplug::ADSREnvelope<double> mAmpEnv{"amp"};
  iplug::ADSREnvelope<double> mFilterEnv{"filter"};
  iplug::ADSREnvelope<double> mPitchEnv{"pitch"};
  iplug::SVF<double,1>        mFilter;
  iplug::SVF<double,1>        mFilter2; // second stage for lp24
};
