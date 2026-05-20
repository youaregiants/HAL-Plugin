#pragma once
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// Soft saturation — tanh-based, drive 0..1
// ──────────────────────────────────────────────────────────────────────────────
inline double halSoftSat(double x, double drive)
{
  if (drive < 0.001) return x;
  const double d  = 1.0 + drive * 5.0;
  const double td = std::tanh(d);
  return td > 0.0 ? std::tanh(x * d) / td : x;
}

// ──────────────────────────────────────────────────────────────────────────────
// HALChorus — stereo chorus with 90°-apart sine LFOs
// ──────────────────────────────────────────────────────────────────────────────
class HALChorus
{
public:
  void Init(double sr)
  {
    mSampleRate = sr;
    const int sz = (int)(sr * 0.06) + 4; // 60ms max
    mBufL.assign(sz, 0.0);
    mBufR.assign(sz, 0.0);
    mWritePos = 0;
    mLFOPhase = 0.0;
  }

  void Process(double* L, double* R, int nFrames, double depth, double mix)
  {
    if (mix < 0.001) return;
    const int    sz        = (int)mBufL.size();
    const double lfoIncr   = 0.6 / mSampleRate; // 0.6Hz internal rate
    const double baseDelay = 0.015 * mSampleRate;
    const double modAmt    = depth * 0.008 * mSampleRate;

    for (int s = 0; s < nFrames; ++s)
    {
      mBufL[mWritePos] = L[s];
      mBufR[mWritePos] = R[s];

      const double lfoL = std::sin(mLFOPhase * 6.283185307179586);
      const double lfoR = std::cos(mLFOPhase * 6.283185307179586); // 90° shift

      L[s] = L[s]*(1.0-mix) + readLinear(mBufL, sz, mWritePos, baseDelay+lfoL*modAmt)*mix;
      R[s] = R[s]*(1.0-mix) + readLinear(mBufR, sz, mWritePos, baseDelay+lfoR*modAmt)*mix;

      if (++mWritePos >= sz) mWritePos = 0;
      mLFOPhase += lfoIncr;
      if (mLFOPhase >= 1.0) mLFOPhase -= 1.0;
    }
  }

private:
  double readLinear(const std::vector<double>& buf, int sz, int wpos, double delay)
  {
    delay = std::max(1.0, std::min(delay, (double)(sz - 2)));
    const int    id   = (int)delay;
    const double frac = delay - id;
    const int    r0   = (wpos - id + sz) % sz;
    const int    r1   = (r0 - 1 + sz) % sz;
    return buf[r0] * (1.0 - frac) + buf[r1] * frac;
  }

  double              mSampleRate = 44100.0;
  std::vector<double> mBufL, mBufR;
  int                 mWritePos = 0;
  double              mLFOPhase = 0.0;
};

// ──────────────────────────────────────────────────────────────────────────────
// HALReverb — Freeverb-style (8 parallel combs + 4 series allpass, stereo)
// ──────────────────────────────────────────────────────────────────────────────
class HALReverb
{
  static constexpr int kNC = 8;
  static constexpr int kNA = 4;

  struct Comb {
    std::vector<double> buf;
    int    pos   = 0;
    double store = 0.0;
    void init(double sr, int refSamples) {
      buf.assign(std::max(1, (int)(refSamples * sr / 44100.0)), 0.0);
      pos = 0; store = 0.0;
    }
    double tick(double in, double fb, double damp) {
      const double out = buf[pos];
      store = out*(1.0-damp) + store*damp;
      buf[pos] = in + store*fb;
      if (++pos >= (int)buf.size()) pos = 0;
      return out;
    }
  };

  struct Allpass {
    std::vector<double> buf;
    int pos = 0;
    void init(double sr, int refSamples) {
      buf.assign(std::max(1, (int)(refSamples * sr / 44100.0)), 0.0);
      pos = 0;
    }
    double tick(double in) {
      const double g   = 0.5;
      const double out = buf[pos];
      buf[pos] = in + out*g;
      if (++pos >= (int)buf.size()) pos = 0;
      return out - in*g;
    }
  };

public:
  void Init(double sr)
  {
    // Freeverb reference delay times (samples @ 44100Hz)
    static constexpr int combL[8]  = {1116,1188,1277,1356,1422,1491,1557,1617};
    static constexpr int combR[8]  = {1139,1211,1300,1379,1445,1514,1580,1640};
    static constexpr int apL[4]    = {556, 441, 341, 225};
    static constexpr int apR[4]    = {579, 464, 364, 248};
    for (int i = 0; i < kNC; ++i) { mCL[i].init(sr,combL[i]); mCR[i].init(sr,combR[i]); }
    for (int i = 0; i < kNA; ++i) { mAL[i].init(sr,apL[i]);   mAR[i].init(sr,apR[i]);   }
  }

  void Process(double* L, double* R, int nFrames, double size, double mix)
  {
    if (mix < 0.001) return;
    const double fb   = 0.84 + size * 0.12; // 0.84..0.96
    const double damp = 0.5 * (1.0 - size);

    for (int s = 0; s < nFrames; ++s)
    {
      const double in = (L[s] + R[s]) * 0.015; // attenuate into reverb
      double wL = 0.0, wR = 0.0;
      for (int i = 0; i < kNC; ++i) { wL += mCL[i].tick(in,fb,damp); wR += mCR[i].tick(in,fb,damp); }
      for (int i = 0; i < kNA; ++i) { wL  = mAL[i].tick(wL);         wR  = mAR[i].tick(wR);         }
      L[s] = L[s]*(1.0-mix) + wL*mix;
      R[s] = R[s]*(1.0-mix) + wR*mix;
    }
  }

private:
  Comb    mCL[kNC], mCR[kNC];
  Allpass mAL[kNA], mAR[kNA];
};

// ──────────────────────────────────────────────────────────────────────────────
// HALDelay — stereo feedback delay, up to 2s
// ──────────────────────────────────────────────────────────────────────────────
class HALDelay
{
public:
  void Init(double sr)
  {
    mSampleRate = sr;
    const int sz = (int)(sr * 2.0) + 4;
    mBufL.assign(sz, 0.0);
    mBufR.assign(sz, 0.0);
    mWritePos = 0;
  }

  void Process(double* L, double* R, int nFrames, double timeMs, double feedback, double mix)
  {
    if (mix < 0.001) return;
    const int sz    = (int)mBufL.size();
    const int delay = std::max(1, std::min((int)(timeMs * 0.001 * mSampleRate), sz - 1));

    for (int s = 0; s < nFrames; ++s)
    {
      const int rp = (mWritePos - delay + sz) % sz;
      const double wL = mBufL[rp], wR = mBufR[rp];
      mBufL[mWritePos] = L[s] + wL * feedback;
      mBufR[mWritePos] = R[s] + wR * feedback;
      L[s] = L[s]*(1.0-mix) + wL*mix;
      R[s] = R[s]*(1.0-mix) + wR*mix;
      if (++mWritePos >= sz) mWritePos = 0;
    }
  }

private:
  double              mSampleRate = 44100.0;
  std::vector<double> mBufL, mBufR;
  int                 mWritePos = 0;
};
