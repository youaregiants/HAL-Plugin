#include "HAL.h"
#include "IPlug_include_in_plug_src.h"
#include "HALHTTPClient.h"
#include "HALPresets.h"
#include "HALExport.h"
#include "IControls.h"
#include "IControl.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>

// nlohmann/json — header-only, included via EXTRA_INC_PATHS → JSON_INC_PATH
#include "json.hpp"
using json = nlohmann::json;

using namespace iplug;
using namespace igraphics;

// ──────────────────────────────────────────────────────────────────────────────
// Colours / style
// ──────────────────────────────────────────────────────────────────────────────
// TE / Thermal-inspired palette: near-black bg, electric orange accent
static const IColor kColBG      {255,  10,  10,  12};   // #0A0A0C
static const IColor kColPanel   {255,  20,  20,  24};   // #141418
static const IColor kColPanel2  {255,  28,  28,  34};   // raised surfaces
static const IColor kColAccent  {255, 255,  85,   0};   // #FF5500 orange
static const IColor kColAccent2 {255, 255, 140,  40};   // lighter orange highlight
static const IColor kColText    {255, 235, 235, 240};
static const IColor kColMuted   {255, 110, 110, 128};
static const IColor kColDivider {255,  38,  38,  46};

// Section accent colours
static const IColor kColOscHue    {255,  70, 140, 255};  // blue
static const IColor kColFiltHue   {255, 255,  85,   0};  // orange (same as accent)
static const IColor kColEnvHue    {255,  80, 210, 130};  // green
static const IColor kColLFOHue    {255, 170,  80, 255};  // purple
static const IColor kColFXHue     {255, 255, 200,  40};  // yellow

static IVStyle HALStyle(IColor accent = IColor(255, 255, 85, 0))
{
  return DEFAULT_STYLE
    .WithColor(kFG,  accent)
    .WithColor(kBG,  kColPanel2)
    .WithColor(kFR,  accent)
    .WithColor(kHL,  IColor(255, 255, 120, 40))
    .WithColor(kSH,  IColor(100, 0, 0, 0))
    .WithColor(kX1,  kColDivider)
    .WithLabelText(IText(10.f, kColMuted, "Roboto-Regular"))
    .WithValueText(IText(9.f,  kColMuted, "Roboto-Regular"))
    .WithDrawShadows(false)
    .WithRoundness(0.08f);
}

// ──────────────────────────────────────────────────────────────────────────────
// Constructor — parameters + UI
// ──────────────────────────────────────────────────────────────────────────────
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

  // Pitch Envelope
  GetParam(kPitchEnvAmount)->InitDouble("Pitch Env Amount", 0.0, 0.0, 48.0, 0.1, "semi", 0, "Pitch Env");
  GetParam(kPitchEnvAttack)->InitDouble("Pitch Env Attack", 2.0, 0.0, 500.0, 0.1, "ms", 0, "Pitch Env");
  GetParam(kPitchEnvDecay)->InitDouble("Pitch Env Decay", 150.0, 1.0, 5000.0, 1.0, "ms", 0, "Pitch Env");

  // Arpeggiator
  GetParam(kArpEnabled)->InitInt("Arp Enabled", 0, 0, 1, "", 0, "Arp");
  GetParam(kArpRate)->InitDouble("Arp Rate", 4.0, 0.25, 32.0, 0.01, "Hz", 0, "Arp");
  GetParam(kArpPattern)->InitInt("Arp Pattern", 0, 0, 3, "", 0, "Arp");   // 0=up,1=dn,2=updn,3=rnd
  GetParam(kArpOctaves)->InitInt("Arp Octaves", 1, 1, 4, "", 0, "Arp");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                        GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(kColBG);
    pGraphics->EnableMouseOver(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT b = pGraphics->GetBounds();
    (void)b.W(); (void)b.H();

    // ── Header bar (36px) ────────────────────────────────────────────────────
    const IRECT header = b.GetFromTop(36.f);
    pGraphics->AttachControl(new IPanelControl(header, kColPanel));
    // HAL logo — left side
    pGraphics->AttachControl(new ITextControl(
      header.GetFromLeft(80.f).GetVPadded(-4.f),
      "HAL",
      IText(22.f, kColAccent, "Roboto-Regular", EAlign::Near, EVAlign::Middle)));
    // Tagline — right side
    pGraphics->AttachControl(new ITextControl(
      header.GetFromRight(300.f).GetVPadded(-4.f),
      "natural language synthesizer",
      IText(10.f, kColMuted, "Roboto-Regular", EAlign::Far, EVAlign::Middle)));

    // ── Category tab bar (28px below header) ─────────────────────────────────
    const IRECT tabBar = b.GetReducedFromTop(36.f).GetFromTop(28.f);
    pGraphics->AttachControl(new IPanelControl(tabBar, kColPanel2));

    const char* cats[] = {"PAD","ARP","BASS","LEAD","STAB","808","FX"};
    const int nCats = 7;
    const float tabW = tabBar.W() / (float)nCats;
    for (int ti = 0; ti < nCats; ++ti)
    {
      const IRECT tabR = IRECT(tabBar.L + ti*tabW, tabBar.T, tabBar.L + (ti+1)*tabW, tabBar.B);
      const std::string catStr = cats[ti];
      pGraphics->AttachControl(new IVButtonControl(
        tabR,
        [&, catStr](IControl* pCaller) {
          mActiveCategory = catStr;
          // Update prompt field placeholder to suggest category
          if (auto* pUI = pCaller->GetUI()) {
            if (auto* p = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagPromptInput)))
              if (std::string(p->GetStr()).empty() || p->GetStr()[0] == '\0')
                p->SetStr(("e.g. " + catStr + " sound").c_str());
          }
        },
        cats[ti],
        HALStyle(kColMuted)
          .WithColor(kBG, kColPanel2)
          .WithLabelText(IText(10.f, kColMuted, "Roboto-Regular", EAlign::Center, EVAlign::Middle)),
        true, false));
    }

    // ── Main area: left panel (AI) | right panel (synth) ─────────────────────
    const IRECT main = b.GetReducedFromTop(64.f).GetPadded(-6.f);
    const float leftW = 200.f;
    const IRECT leftCol  = main.GetFromLeft(leftW);
    const IRECT rightCol = main.GetReducedFromLeft(leftW + 6.f);

    // ── Left panel background ─────────────────────────────────────────────────
    pGraphics->AttachControl(new IPanelControl(leftCol, kColPanel));

    const IRECT lp = leftCol.GetPadded(-8.f);

    // Prompt label
    pGraphics->AttachControl(new ITextControl(
      lp.GetFromTop(13.f),
      "DESCRIBE SOUND",
      IText(9.f, kColAccent, "Roboto-Regular", EAlign::Near, EVAlign::Top)));

    // Prompt field
    pGraphics->AttachControl(new IEditableTextControl(
      lp.GetReducedFromTop(15.f).GetFromTop(26.f),
      "warm pad with slow attack",
      IText(11.f, kColText, "Roboto-Regular", EAlign::Near, EVAlign::Middle),
      IColor(255, 30, 30, 36)),
      kCtrlTagPromptInput);

    // Style label
    pGraphics->AttachControl(new ITextControl(
      lp.GetReducedFromTop(47.f).GetFromTop(13.f),
      "STYLE HINT",
      IText(9.f, kColMuted, "Roboto-Regular", EAlign::Near, EVAlign::Top)));

    // Style field
    pGraphics->AttachControl(new IEditableTextControl(
      lp.GetReducedFromTop(62.f).GetFromTop(24.f),
      "",
      IText(11.f, kColText, "Roboto-Regular", EAlign::Near, EVAlign::Middle),
      IColor(255, 30, 30, 36)),
      kCtrlTagStyleHint);

    // Generate button
    pGraphics->AttachControl(
      new IVButtonControl(
        lp.GetReducedFromTop(93.f).GetFromTop(32.f),
        [&](IControl* pCaller) {
          if (mGenerating) return;
          auto* pUI     = pCaller->GetUI();
          auto* pPrompt = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagPromptInput));
          auto* pStyle  = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagStyleHint));
          auto* pNudge  = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagNudgeInput));
          std::string prompt = pPrompt ? pPrompt->GetStr() : "";
          std::string style  = pStyle  ? pStyle->GetStr()  : "";
          std::string nudge  = pNudge  ? pNudge->GetStr()  : "";
          // Prepend active category to prompt for better server guidance
          if (!mActiveCategory.empty() && !prompt.empty() &&
              prompt.find(mActiveCategory) == std::string::npos)
            style = mActiveCategory + (style.empty() ? "" : " " + style);
          if (!nudge.empty() && !mLastPrompt.empty())
            GeneratePatch(mLastPrompt, style, nudge);
          else if (!prompt.empty())
            GeneratePatch(prompt, style);
        },
        "GENERATE",
        HALStyle(kColAccent)
          .WithColor(kBG, IColor(255, 50, 22, 0))
          .WithLabelText(IText(12.f, kColText, "Roboto-Regular", EAlign::Center, EVAlign::Middle)),
        true, false),
      kCtrlTagGenerateButton);

    // Status
    pGraphics->AttachControl(
      new ITextControl(
        lp.GetReducedFromTop(131.f).GetFromTop(18.f),
        "Ready",
        IText(10.f, kColMuted, "Roboto-Regular", EAlign::Near, EVAlign::Middle)),
      kCtrlTagStatus);

    // Divider
    pGraphics->AttachControl(new IPanelControl(
      lp.GetReducedFromTop(154.f).GetFromTop(1.f), kColDivider));

    // Nudge label
    pGraphics->AttachControl(new ITextControl(
      lp.GetReducedFromTop(159.f).GetFromTop(13.f),
      "NUDGE / REFINE",
      IText(9.f, kColMuted, "Roboto-Regular", EAlign::Near, EVAlign::Top)));

    // Nudge field
    pGraphics->AttachControl(new IEditableTextControl(
      lp.GetReducedFromTop(174.f).GetFromTop(24.f),
      "",
      IText(11.f, kColText, "Roboto-Regular", EAlign::Near, EVAlign::Middle),
      IColor(255, 30, 30, 36)),
      kCtrlTagNudgeInput);

    // Divider
    pGraphics->AttachControl(new IPanelControl(
      lp.GetReducedFromTop(202.f).GetFromTop(1.f), kColDivider));

    // Preset browser label
    pGraphics->AttachControl(new ITextControl(
      lp.GetReducedFromTop(207.f).GetFromTop(13.f),
      "NOT PRESETS",
      IText(9.f, kColMuted, "Roboto-Regular", EAlign::Near, EVAlign::Top)));

    // Preset button — opens popup menu
    pGraphics->AttachControl(
      new IVButtonControl(
        lp.GetReducedFromTop(222.f).GetFromTop(24.f),
        [&](IControl* pCaller) {
          IPopupMenu menu{"Not Presets"};
          for (int i = 0; i < kNumHALPresets; ++i)
            menu.AddItem(kHALPresets[i].name);
          pCaller->GetUI()->CreatePopupMenu(*pCaller, menu,
            pCaller->GetRECT());
          // Result handled in IControl::OnPopupMenuSelection — use lambda capture approach
          // iPlug2 will call OnPopupMenuSelection on the control; we sub-class below
        },
        "Choose preset...",
        HALStyle(kColMuted)
          .WithColor(kBG, kColPanel2)
          .WithLabelText(IText(10.f, kColMuted, "Roboto-Regular", EAlign::Near, EVAlign::Middle)),
        true, false));

    // Export buttons row
    const IRECT exportRow = lp.GetReducedFromTop(252.f).GetFromTop(24.f);
    pGraphics->AttachControl(
      new IVButtonControl(
        exportRow.GetGridCell(0, 0, 2, 1).GetHPadded(-2.f),
        [&](IControl*) { ExportWAV(); },
        "WAV",
        HALStyle(kColFXHue)
          .WithColor(kBG, kColPanel2)
          .WithLabelText(IText(10.f, kColFXHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)),
        true, false),
      kCtrlTagExportButton);

    pGraphics->AttachControl(
      new IVButtonControl(
        exportRow.GetGridCell(0, 1, 2, 1).GetHPadded(-2.f),
        [&](IControl*) { ExportVital(); },
        "VITAL",
        HALStyle(kColLFOHue)
          .WithColor(kBG, kColPanel2)
          .WithLabelText(IText(10.f, kColLFOHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)),
        true, false));

    // ── Right panel: synth parameters ────────────────────────────────────────
    const float knobSz = 34.f;
    const float rowH   = 72.f;
    float ry = rightCol.T;

    auto sectionLabel = [&](IRECT area, const char* lbl, IColor col) {
      pGraphics->AttachControl(new ITextControl(
        area.GetFromTop(13.f),
        lbl, IText(9.f, col, "Roboto-Regular", EAlign::Near, EVAlign::Top)));
    };

    auto knob = [&](const IRECT& cell, int param, const char* lbl, IColor col) {
      pGraphics->AttachControl(new IVKnobControl(
        cell.GetCentredInside(knobSz), param, lbl,
        HALStyle(col)
          .WithLabelText(IText(8.f, kColMuted, "Roboto-Regular"))
          .WithValueText(IText(7.f, kColMuted, "Roboto-Regular"))));
    };

    // ── OSC section ──────────────────────────────────────────────────────────
    const IRECT oscSection = IRECT(rightCol.L, ry, rightCol.R, ry + 13.f + rowH * 3.f);
    pGraphics->AttachControl(new IPanelControl(oscSection.GetPadded(2.f), kColPanel));
    sectionLabel(oscSection, "OSCILLATORS", kColOscHue);

    const char* oscKnobLabels[] = {"Wave","Oct","Fine","Uni","Sprd","Vol"};
    int oscKnobParams[][6] = {
      {kOsc1Waveform,kOsc1Octave,kOsc1FineTune,kOsc1UnisonVoices,kOsc1UnisonSpread,kOsc1Volume},
      {kOsc2Waveform,kOsc2Octave,kOsc2FineTune,kOsc2UnisonVoices,kOsc2UnisonSpread,kOsc2Volume},
      {kOsc3Waveform,kOsc3Octave,kOsc3FineTune,kOsc3UnisonVoices,kOsc3UnisonSpread,kOsc3Volume},
    };
    for (int oi = 0; oi < 3; ++oi)
    {
      const IRECT row = IRECT(oscSection.L, oscSection.T + 14.f + oi*rowH,
                              oscSection.R, oscSection.T + 14.f + (oi+1)*rowH);
      const char* tag = (oi==0?"O1":oi==1?"O2":"O3");
      pGraphics->AttachControl(new ITextControl(
        row.GetFromLeft(18.f),
        tag, IText(8.f, kColOscHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
      for (int k = 0; k < 6; ++k)
        knob(row.GetReducedFromLeft(20.f).GetGridCell(0, k, 6, 1),
             oscKnobParams[oi][k], oscKnobLabels[k], kColOscHue);
    }
    ry += 14.f + rowH * 3.f + 6.f;

    // ── Filter + Pitch Env section ───────────────────────────────────────────
    const IRECT filtSection = IRECT(rightCol.L, ry, rightCol.R, ry + 13.f + rowH * 2.f);
    pGraphics->AttachControl(new IPanelControl(filtSection.GetPadded(2.f), kColPanel));
    sectionLabel(filtSection, "FILTER", kColFiltHue);

    const char* filtLabels[] = {"Type","Cut","Res","Env","Drv"};
    int   filtParams[]       = {kFilterType,kFilterCutoff,kFilterResonance,kFilterEnvAmount,kFilterDrive};
    const IRECT filtRow = IRECT(filtSection.L, filtSection.T+14.f, filtSection.R, filtSection.T+14.f+rowH);
    pGraphics->AttachControl(new ITextControl(
      filtRow.GetFromLeft(18.f), "FLT",
      IText(8.f, kColFiltHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    for (int k = 0; k < 5; ++k)
      knob(filtRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 8, 1), filtParams[k], filtLabels[k], kColFiltHue);

    // Pitch env knobs in same row after filter
    const char* pitchLabels[] = {"PEnv","PAtk","PDec"};
    int pitchParams[]         = {kPitchEnvAmount, kPitchEnvAttack, kPitchEnvDecay};
    for (int k = 0; k < 3; ++k)
      knob(filtRow.GetReducedFromLeft(20.f).GetGridCell(0, k+5, 8, 1), pitchParams[k], pitchLabels[k], kColEnvHue);

    // Filter envelope row
    const IRECT fEnvRow = IRECT(filtSection.L, filtSection.T+14.f+rowH, filtSection.R, filtSection.B);
    pGraphics->AttachControl(new ITextControl(
      fEnvRow.GetFromLeft(18.f), "FEV",
      IText(8.f, kColEnvHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    const char* fenvLabels[] = {"Atk","Dec","Sus","Rel"};
    int fenvParams[]         = {kFilterAttack,kFilterDecay,kFilterSustain,kFilterRelease};
    for (int k = 0; k < 4; ++k)
      knob(fEnvRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 8, 1), fenvParams[k], fenvLabels[k], kColEnvHue);

    ry += 14.f + rowH * 2.f + 6.f;

    // ── Amp Env + LFO section ────────────────────────────────────────────────
    const IRECT envSection = IRECT(rightCol.L, ry, rightCol.R, ry + 13.f + rowH * 2.f);
    pGraphics->AttachControl(new IPanelControl(envSection.GetPadded(2.f), kColPanel));
    sectionLabel(envSection, "AMP ENV  /  LFO", kColEnvHue);

    // Amp env
    const IRECT ampRow = IRECT(envSection.L, envSection.T+14.f, envSection.R, envSection.T+14.f+rowH);
    pGraphics->AttachControl(new ITextControl(
      ampRow.GetFromLeft(18.f), "AMP",
      IText(8.f, kColEnvHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    const char* ampLabels[] = {"Atk","Dec","Sus","Rel"};
    int ampParams[]         = {kAmpAttack,kAmpDecay,kAmpSustain,kAmpRelease};
    for (int k = 0; k < 4; ++k)
      knob(ampRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 8, 1), ampParams[k], ampLabels[k], kColEnvHue);

    // LFO 1+2 in same row, right half
    const char* lfoLabels[] = {"Shp","Rate","Dep","Tgt","Shp","Rate","Dep","Tgt"};
    int lfoParams[] = {kLFO1Shape,kLFO1Rate,kLFO1Depth,kLFO1Target,
                       kLFO2Shape,kLFO2Rate,kLFO2Depth,kLFO2Target};
    const IRECT lfoRow = IRECT(envSection.L, envSection.T+14.f+rowH, envSection.R, envSection.B);
    pGraphics->AttachControl(new ITextControl(
      lfoRow.GetFromLeft(18.f), "LFO",
      IText(8.f, kColLFOHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    for (int k = 0; k < 8; ++k)
      knob(lfoRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 8, 1), lfoParams[k], lfoLabels[k], kColLFOHue);

    ry += 14.f + rowH * 2.f + 6.f;

    // ── FX + Arp section ─────────────────────────────────────────────────────
    const IRECT fxSection = IRECT(rightCol.L, ry, rightCol.R, ry + 13.f + rowH * 2.f);
    pGraphics->AttachControl(new IPanelControl(fxSection.GetPadded(2.f), kColPanel));
    sectionLabel(fxSection, "EFFECTS  /  ARP", kColFXHue);

    const IRECT fxRow = IRECT(fxSection.L, fxSection.T+14.f, fxSection.R, fxSection.T+14.f+rowH);
    pGraphics->AttachControl(new ITextControl(
      fxRow.GetFromLeft(18.f), "FX",
      IText(8.f, kColFXHue, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    const char* fxLabels[] = {"ChD","ChM","RvS","RvM","DlT","DlF","DlM","Sat","EQL","EQH"};
    int fxParams[]         = {kChorusDepth,kChorusMix,kReverbSize,kReverbMix,
                               kDelayTime,kDelayFeedback,kDelayMix,
                               kSaturationDrive,kEQLowShelf,kEQHighShelf};
    for (int k = 0; k < 10; ++k)
      knob(fxRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 10, 1), fxParams[k], fxLabels[k], kColFXHue);

    const IRECT arpRow = IRECT(fxSection.L, fxSection.T+14.f+rowH, fxSection.R, fxSection.B);
    pGraphics->AttachControl(new ITextControl(
      arpRow.GetFromLeft(18.f), "ARP",
      IText(8.f, kColAccent, "Roboto-Regular", EAlign::Center, EVAlign::Middle)));
    const char* arpLabels[] = {"On","Rate","Pat","Oct"};
    int arpParams[]         = {kArpEnabled,kArpRate,kArpPattern,kArpOctaves};
    for (int k = 0; k < 4; ++k)
      knob(arpRow.GetReducedFromLeft(20.f).GetGridCell(0, k, 8, 1), arpParams[k], arpLabels[k], kColAccent);
  };
#endif
}

// ──────────────────────────────────────────────────────────────────────────────
// DSP initialisation
// ──────────────────────────────────────────────────────────────────────────────
void HAL::InitDSP()
{
  for (int v = 0; v < kNumVoices; ++v)
    mVoices[v].Init(mSampleRate);

  for (int li = 0; li < 2; ++li) {
    mLFO[li].SetSampleRate(mSampleRate);
    mLFO[li].SetPolarity(true); // bipolar -1..+1
    mLFO[li].Reset();
  }

  mEQLow.SetSampleRate(mSampleRate);
  mEQLow.SetMode(SVF<double,2>::kLowPassShelf);
  mEQLow.SetFreqCPS(200.0);
  mEQLow.SetQ(0.707);
  mEQLow.Reset();

  mEQHigh.SetSampleRate(mSampleRate);
  mEQHigh.SetMode(SVF<double,2>::kHighPassShelf);
  mEQHigh.SetFreqCPS(8000.0);
  mEQHigh.SetQ(0.707);
  mEQHigh.Reset();

  mChorus.Init(mSampleRate);
  mReverb.Init(mSampleRate);
  mDelay.Init(mSampleRate);

  UpdateSynthParams();
}

void HAL::UpdateSynthParams()
{
  auto& p = mSynthParams;
  auto V  = [&](int idx) { return GetParam(idx)->Value(); };

  for (int i = 0; i < 3; ++i) {
    const int base       = kOsc1Waveform + i * 6;
    p.osc[i].waveform     = (int)V(base + 0);
    p.osc[i].octave       = (int)V(base + 1);
    p.osc[i].fineTune     = V(base + 2);
    p.osc[i].unisonVoices = (int)V(base + 3);
    p.osc[i].unisonSpread = V(base + 4);
    p.osc[i].volume       = V(base + 5);
  }

  p.filterType      = (int)V(kFilterType);
  p.filterCutoff    = V(kFilterCutoff);
  p.filterResonance = V(kFilterResonance);
  p.filterEnvAmount = V(kFilterEnvAmount);
  p.filterDrive     = V(kFilterDrive);

  p.filterAttack  = V(kFilterAttack);
  p.filterDecay   = V(kFilterDecay);
  p.filterSustain = V(kFilterSustain);
  p.filterRelease = V(kFilterRelease);

  p.ampAttack  = V(kAmpAttack);
  p.ampDecay   = V(kAmpDecay);
  p.ampSustain = V(kAmpSustain);
  p.ampRelease = V(kAmpRelease);

  for (int li = 0; li < 2; ++li) {
    const int base    = kLFO1Shape + li * 4;
    p.lfo[li].shape   = (int)V(base + 0);
    p.lfo[li].rate    = V(base + 1);
    p.lfo[li].depth   = V(base + 2);
    p.lfo[li].target  = (int)V(base + 3);
  }

  p.chorusDepth   = V(kChorusDepth);
  p.chorusMix     = V(kChorusMix);
  p.reverbSize    = V(kReverbSize);
  p.reverbMix     = V(kReverbMix);
  p.delayTime     = V(kDelayTime);
  p.delayFeedback = V(kDelayFeedback);
  p.delayMix      = V(kDelayMix);
  p.satDrive      = V(kSaturationDrive);
  p.eqLow         = V(kEQLowShelf);
  p.eqHigh        = V(kEQHighShelf);

  p.pitchEnvAmount = V(kPitchEnvAmount);
  p.pitchEnvAttack = V(kPitchEnvAttack);
  p.pitchEnvDecay  = V(kPitchEnvDecay);

  // Propagate new envelope times to any currently active voices
  for (int v = 0; v < kNumVoices; ++v)
    mVoices[v].UpdateEnvTimes(p);
}

// ──────────────────────────────────────────────────────────────────────────────
// Audio
// ──────────────────────────────────────────────────────────────────────────────
void HAL::OnReset()
{
  mSampleRate = GetSampleRate();
  InitDSP();
}

void HAL::OnParamChange(int /*paramIdx*/)
{
  UpdateSynthParams();
}

void HAL::ProcessMidiMsg(const iplug::IMidiMsg& msg)
{
  const bool arpOn = (GetParam(kArpEnabled)->Int() != 0);

  switch (msg.StatusMsg())
  {
    case IMidiMsg::kNoteOn:
      if (msg.Velocity() > 0) {
        if (arpOn) {
          // Track held notes for arp; silence current arp step first
          if (std::find(mHeldNotes.begin(), mHeldNotes.end(), msg.NoteNumber()) == mHeldNotes.end())
            mHeldNotes.push_back(msg.NoteNumber());
        } else {
          const int v = FindFreeVoice();
          mVoices[v].NoteOn(msg.NoteNumber(), msg.Velocity() / 127.0, mSynthParams, ++mVoiceAge);
        }
      } else {
        if (arpOn) {
          mHeldNotes.erase(std::remove(mHeldNotes.begin(), mHeldNotes.end(), msg.NoteNumber()), mHeldNotes.end());
        } else {
          for (int v = 0; v < kNumVoices; ++v)
            if (mVoices[v].GetNote() == msg.NoteNumber()) mVoices[v].NoteOff();
        }
      }
      break;
    case IMidiMsg::kNoteOff:
      if (arpOn) {
        mHeldNotes.erase(std::remove(mHeldNotes.begin(), mHeldNotes.end(), msg.NoteNumber()), mHeldNotes.end());
      } else {
        for (int v = 0; v < kNumVoices; ++v)
          if (mVoices[v].GetNote() == msg.NoteNumber()) mVoices[v].NoteOff();
      }
      break;
    default:
      break;
  }
}

int HAL::FindFreeVoice()
{
  for (int v = 0; v < kNumVoices; ++v)
    if (mVoices[v].IsIdle()) return v;
  // All busy — steal oldest
  int oldest = 0;
  for (int v = 1; v < kNumVoices; ++v)
    if (mVoices[v].GetAge() < mVoices[oldest].GetAge()) oldest = v;
  return oldest;
}

void HAL::ProcessBlock(iplug::sample** /*inputs*/, iplug::sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  for (int c = 0; c < nChans; c++)
    for (int s = 0; s < nFrames; s++)
      outputs[c][s] = 0.0;

  if (nChans < 1) return;

  const HALSynthParams p = mSynthParams; // snapshot for this block

  // ── Arpeggiator ──────────────────────────────────────────────────────────
  const bool arpOn    = (GetParam(kArpEnabled)->Int() != 0);
  const double arpHz  = GetParam(kArpRate)->Value();
  const int arpPat    = GetParam(kArpPattern)->Int();
  const int arpOcts   = GetParam(kArpOctaves)->Int();

  if (arpOn && !mHeldNotes.empty())
  {
    // Build note set with octave expansion
    static int arpNotes[kNumVoices * 4];
    int numArpNotes = 0;
    for (int oct = 0; oct < arpOcts; ++oct)
      for (int n : mHeldNotes)
        if (numArpNotes < (int)(sizeof(arpNotes)/sizeof(arpNotes[0])))
          arpNotes[numArpNotes++] = n + oct * 12;

    if (arpPat == 1) { // down — reverse
      std::reverse(arpNotes, arpNotes + numArpNotes);
    }

    for (int s = 0; s < nFrames; ++s)
    {
      mArpPhase += arpHz / mSampleRate;
      if (mArpPhase >= 1.0) {
        mArpPhase -= 1.0;
        // Kill all active arp voices
        for (int v = 0; v < kNumVoices; ++v)
          if (mVoices[v].IsActive()) mVoices[v].NoteOff();

        // Pattern step
        if (arpPat == 2) { // up/down
          if (mArpUp) {
            mArpIdx++;
            if (mArpIdx >= numArpNotes) { mArpIdx = std::max(0, numArpNotes-2); mArpUp = false; }
          } else {
            mArpIdx--;
            if (mArpIdx < 0)            { mArpIdx = std::min(1, numArpNotes-1); mArpUp = true; }
          }
        } else if (arpPat == 3) { // random
          mArpIdx = (numArpNotes > 1) ? (rand() % numArpNotes) : 0;
        } else {
          mArpIdx = (mArpIdx + 1) % numArpNotes;
        }

        const int note = arpNotes[mArpIdx % numArpNotes];
        const int v    = FindFreeVoice();
        mVoices[v].NoteOn(note, 0.85, p, ++mVoiceAge);
      }
    }
  }

  // LFO shape mapping: plugin (0=sine,1=tri,2=saw,3=sq,4=random) → LFO<double>::EShape
  using LFOShape = LFO<double>::EShape;
  static const LFOShape shapeMap[5] = {
    LFOShape::kSine, LFOShape::kTriangle, LFOShape::kRampUp,
    LFOShape::kSquare, LFOShape::kRampUp   // 4=random: fall back to ramp
  };
  for (int li = 0; li < 2; ++li)
    mLFO[li].SetShape((int)shapeMap[std::max(0, std::min(p.lfo[li].shape, 4))]);

  double lfoMod[2] = {0.0, 0.0};

  for (int s = 0; s < nFrames; ++s)
  {
    // Advance both LFOs once per sample
    for (int li = 0; li < 2; ++li)
      lfoMod[li] = mLFO[li].Process(p.lfo[li].rate);

    // Sum all voices
    double mono = 0.0;
    for (int v = 0; v < kNumVoices; ++v)
      mono += mVoices[v].Process(p, lfoMod);
    mono *= 0.2; // headroom for 8 voices

    if (nChans >= 2) {
      outputs[0][s] = mono;
      outputs[1][s] = mono;
    } else {
      outputs[0][s] = mono;
    }
  }

  if (nChans >= 2)
  {
    // Saturation
    if (p.satDrive > 0.001)
      for (int s = 0; s < nFrames; ++s) {
        outputs[0][s] = halSoftSat(outputs[0][s], p.satDrive);
        outputs[1][s] = halSoftSat(outputs[1][s], p.satDrive);
      }

    // EQ shelves
    if (std::abs(p.eqLow) > 0.1) {
      mEQLow.SetGain(p.eqLow);
      mEQLow.SetSampleRate(mSampleRate);
      mEQLow.ProcessBlock(outputs, outputs, 2, nFrames);
    }
    if (std::abs(p.eqHigh) > 0.1) {
      mEQHigh.SetGain(p.eqHigh);
      mEQHigh.SetSampleRate(mSampleRate);
      mEQHigh.ProcessBlock(outputs, outputs, 2, nFrames);
    }

    // Time-based effects
    mChorus.Process(outputs[0], outputs[1], nFrames, p.chorusDepth, p.chorusMix);
    mReverb.Process(outputs[0], outputs[1], nFrames, p.reverbSize,  p.reverbMix);
    mDelay.Process (outputs[0], outputs[1], nFrames, p.delayTime,   p.delayFeedback, p.delayMix);
  }
}

// ──────────────────────────────────────────────────────────────────────────────
// OnIdle — runs on main thread, picks up patch + status from HTTP callback
// ──────────────────────────────────────────────────────────────────────────────
void HAL::OnIdle()
{
  // Animated loading dots while waiting for the HTTP response
  if (mGenerating && !mPatchReady && !mStatusDirty)
  {
    if ((++mAnimTick % 18) == 0) // ~3 changes/sec at 60Hz
    {
      mAnimFrame = (mAnimFrame + 1) % 4;
      static const char* frames[4] = {"Generating .", "Generating . .", "Generating . . .", "Generating . ."};
      if (auto* pUI = GetUI())
        if (auto* ctrl = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagStatus)))
        {
          ctrl->SetStr(frames[mAnimFrame]);
          ctrl->SetDirty(false);
        }
    }
  }

  if (mStatusDirty.exchange(false))
  {
    std::string msg;
    {
      std::lock_guard<std::mutex> lk(mPatchMutex);
      msg = mPendingStatus;
    }
    if (auto* pUI = GetUI())
    {
      if (auto* ctrl = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagStatus)))
      {
        ctrl->SetStr(msg.c_str());
        ctrl->SetDirty(false);
      }
      if (auto* btn = pUI->GetControlWithTag(kCtrlTagGenerateButton))
        btn->SetDisabled(false);
    }
  }

  if (mPatchReady.exchange(false))
  {
    std::string patch;
    {
      std::lock_guard<std::mutex> lk(mPatchMutex);
      patch = mPendingPatch;
    }
    ApplyPatchJSON(patch);
    mGenerating = false;
  }
}

// ──────────────────────────────────────────────────────────────────────────────
// GeneratePatch — fires async HTTP request
// ──────────────────────────────────────────────────────────────────────────────
void HAL::GeneratePatch(const std::string& prompt, const std::string& style,
                        const std::string& nudge)
{
  if (mGenerating.exchange(true)) return;

  mLastPrompt  = prompt;
  mAnimTick    = 0;
  mAnimFrame   = 0;

  if (auto* pUI = GetUI())
  {
    if (auto* btn = pUI->GetControlWithTag(kCtrlTagGenerateButton))
      btn->SetDisabled(true);
    if (auto* statusCtrl = static_cast<ITextControl*>(pUI->GetControlWithTag(kCtrlTagStatus)))
    {
      statusCtrl->SetStr("Generating .");
      statusCtrl->SetDirty(false);
    }
  }

  // Build request body — include current_patch + nudge when in refine mode
  std::string currentPatchJSON;
  if (!nudge.empty())
    currentPatchJSON = BuildCurrentPatchJSON();

  HALHTTPClient::generate(prompt, style, nudge, currentPatchJSON,
                          [this, prompt](bool ok, std::string body) {
    std::lock_guard<std::mutex> lk(mPatchMutex);
    if (ok) {
      mPendingPatch = std::move(body);
      mPatchReady   = true;
      // Truncate prompt to 36 chars for display
      std::string label = prompt.size() > 36 ? prompt.substr(0, 33) + "..." : prompt;
      mPendingStatus = "\xe2\x9c\x93 " + label; // ✓ (UTF-8)
    } else {
      mPendingStatus = "Error: " + body;
      mGenerating    = false;
    }
    mStatusDirty = true;
  });
}

// ──────────────────────────────────────────────────────────────────────────────
// Export helpers
// ──────────────────────────────────────────────────────────────────────────────
void HAL::ExportWAV()
{
  SetStatus("Rendering...");

  const HALSynthParams p = mSynthParams;
  const double sr = mSampleRate;

  std::vector<float> L, R;
  uint32_t loopStart = 0, loopEnd = 0;
  renderPatch(p, 60, 3000.0, 2000.0, sr, L, R, loopStart, loopEnd);

  // Build path: ~/Desktop/HAL_export_<timestamp>.wav
  const std::string ts = std::to_string((long long)std::time(nullptr));
  const std::string path = std::string(getenv("HOME")) + "/Desktop/HAL_export_" + ts + ".wav";

  if (writeWAV(path, L, R, sr, loopStart, loopEnd))
  {
    SetStatus("WAV saved to Desktop");
    // Reveal in Finder
    const std::string cmd = "open -R \"" + path + "\"";
    system(cmd.c_str());
  }
  else
  {
    SetStatus("WAV export failed");
  }
}

void HAL::ExportVital()
{
  const HALSynthParams p = mSynthParams;
  const std::string name = mLastPrompt.empty() ? "HAL Patch" : mLastPrompt;
  const std::string preset = buildVitalPreset(name, p);

  const std::string ts   = std::to_string((long long)std::time(nullptr));
  const std::string path = std::string(getenv("HOME")) + "/Desktop/HAL_" + ts + ".vital";

  std::ofstream f(path);
  if (f.is_open()) {
    f << preset;
    f.close();
    SetStatus("Vital preset saved to Desktop");
    const std::string cmd = "open -R \"" + path + "\"";
    system(cmd.c_str());
  } else {
    SetStatus("Vital export failed");
  }
}

// ──────────────────────────────────────────────────────────────────────────────
// BuildCurrentPatchJSON — serialize current knob state for nudge requests
// ──────────────────────────────────────────────────────────────────────────────
std::string HAL::BuildCurrentPatchJSON() const
{
  static const char* waveNames[]   = {"sine","saw","square","triangle"};
  static const char* filterNames[] = {"lp12","lp24","hp12","bp","notch"};
  static const char* lfoShapes[]   = {"sine","triangle","saw","square","random"};
  static const char* lfoTargets[]  = {"osc_pitch","filter_cutoff","osc_volume","pan"};

  auto clamp01 = [](double v) { return v < 0.0 ? 0.0 : v > 1.0 ? 1.0 : v; };
  auto V = [&](int idx) { return GetParam(idx)->Value(); };

  auto oscJSON = [&](int base) -> json {
    int wf = (int)V(base+0); if (wf < 0) wf = 0; if (wf > 3) wf = 3;
    return json{
      {"waveform",      waveNames[wf]},
      {"octave",        (int)V(base+1)},
      {"fine_tune",     V(base+2)},
      {"unison_voices", (int)V(base+3)},
      {"unison_spread", V(base+4)},
      {"volume",        V(base+5)},
      {"pan",           0.0}
    };
  };

  int ft = (int)V(kFilterType); if (ft < 0) ft = 0; if (ft > 4) ft = 4;
  int l1s = (int)V(kLFO1Shape);  if (l1s<0)l1s=0; if(l1s>4)l1s=4;
  int l2s = (int)V(kLFO2Shape);  if (l2s<0)l2s=0; if(l2s>4)l2s=4;
  int l1t = (int)V(kLFO1Target); if (l1t<0)l1t=0; if(l1t>3)l1t=3;
  int l2t = (int)V(kLFO2Target); if (l2t<0)l2t=0; if(l2t>3)l2t=3;

  json patch = {
    {"osc1",       oscJSON(kOsc1Waveform)},
    {"osc2",       oscJSON(kOsc2Waveform)},
    {"osc3",       oscJSON(kOsc3Waveform)},
    {"filter", {
      {"type",       filterNames[ft]},
      {"cutoff_hz",  V(kFilterCutoff)},
      {"resonance",  V(kFilterResonance)},
      {"env_amount", V(kFilterEnvAmount)},
      {"drive",      V(kFilterDrive)},
      {"key_tracking", 0.0}
    }},
    {"filter_env", {
      {"attack_ms",  V(kFilterAttack)},
      {"decay_ms",   V(kFilterDecay)},
      {"sustain",    clamp01(V(kFilterSustain))},
      {"release_ms", V(kFilterRelease)}
    }},
    {"amp_env", {
      {"attack_ms",  V(kAmpAttack)},
      {"decay_ms",   V(kAmpDecay)},
      {"sustain",    clamp01(V(kAmpSustain))},
      {"release_ms", V(kAmpRelease)}
    }},
    {"lfo1", {
      {"shape",         lfoShapes[l1s]},
      {"rate_hz",       V(kLFO1Rate)},
      {"depth",         clamp01(V(kLFO1Depth))},
      {"target",        lfoTargets[l1t]},
      {"sync_to_tempo", false}
    }},
    {"lfo2", {
      {"shape",         lfoShapes[l2s]},
      {"rate_hz",       V(kLFO2Rate)},
      {"depth",         clamp01(V(kLFO2Depth))},
      {"target",        lfoTargets[l2t]},
      {"sync_to_tempo", false}
    }},
    {"pitch_env", {
      {"amount_semitones", V(kPitchEnvAmount)},
      {"attack_ms",        V(kPitchEnvAttack)},
      {"decay_ms",         V(kPitchEnvDecay)}
    }},
    {"effects", {
      {"chorus_depth",    V(kChorusDepth)},
      {"chorus_mix",      V(kChorusMix)},
      {"reverb_size",     V(kReverbSize)},
      {"reverb_mix",      V(kReverbMix)},
      {"delay_time_ms",   V(kDelayTime)},
      {"delay_feedback",  V(kDelayFeedback)},
      {"delay_mix",       V(kDelayMix)},
      {"saturation_drive",V(kSaturationDrive)},
      {"saturation_type", "soft"},
      {"eq_low_shelf_db", V(kEQLowShelf)},
      {"eq_high_shelf_db",V(kEQHighShelf)},
      {"reverb_damping",  0.5},
      {"chorus_rate",     0.5},
      {"eq_mid_freq",     1000.0},
      {"eq_mid_gain_db",  0.0}
    }}
  };

  return patch.dump();
}

// ──────────────────────────────────────────────────────────────────────────────
// ApplyPatchJSON — parse server response and push all params to host
// ──────────────────────────────────────────────────────────────────────────────

// Response shape: {"patch": PatchParameters, "cached": bool}
// PatchParameters: {osc1, osc2, osc3, filter, filter_env, amp_env, lfo1, lfo2, effects}

static int waveformIndex(const std::string& s)
{
  if (s == "sine")     return 0;
  if (s == "saw")      return 1;
  if (s == "square")   return 2;
  if (s == "triangle") return 3;
  return 1;
}

// Server uses: lp12, lp24, hp12, bp, notch → maps to plugin's 0-4
static int filterTypeIndex(const std::string& s)
{
  if (s == "lp12")  return 0;
  if (s == "lp24")  return 1;
  if (s == "hp12")  return 2;
  if (s == "bp")    return 3;
  if (s == "notch") return 4;
  return 1;
}

static int lfoShapeIndex(const std::string& s)
{
  if (s == "sine")     return 0;
  if (s == "triangle") return 1;
  if (s == "saw")      return 2;
  if (s == "square")   return 3;
  if (s == "random")   return 4;
  return 0;
}

// Server uses: filter_cutoff, osc_pitch, osc_volume, pan
static int lfoTargetIndex(const std::string& s)
{
  if (s == "osc_pitch")    return 0;
  if (s == "filter_cutoff")return 1;
  if (s == "osc_volume")   return 2;
  if (s == "pan")          return 3;
  return 1;
}

void HAL::SetStatus(const std::string& msg)
{
  std::lock_guard<std::mutex> lk(mPatchMutex);
  mPendingStatus = msg;
  mStatusDirty = true;
}

void HAL::ApplyPatchJSON(const std::string& jsonStr)
{
  try
  {
    const json root = json::parse(jsonStr);

    // Unwrap the envelope: server returns {"patch": {...}, "cached": bool}
    if (!root.contains("patch") || !root["patch"].is_object()) {
      SetStatus("Error: unexpected response format");
      return;
    }
    const json& j = root["patch"];

    // ── Oscillators ──────────────────────────────────────────────────────────
    const char* oscKeys[] = {"osc1", "osc2", "osc3"};
    const int oscParams[][6] = {
      {kOsc1Waveform, kOsc1Octave, kOsc1FineTune, kOsc1UnisonVoices, kOsc1UnisonSpread, kOsc1Volume},
      {kOsc2Waveform, kOsc2Octave, kOsc2FineTune, kOsc2UnisonVoices, kOsc2UnisonSpread, kOsc2Volume},
      {kOsc3Waveform, kOsc3Octave, kOsc3FineTune, kOsc3UnisonVoices, kOsc3UnisonSpread, kOsc3Volume},
    };
    for (int i = 0; i < 3; ++i)
    {
      if (!j.contains(oscKeys[i])) continue;
      const auto& o = j[oscKeys[i]];
      if (o.contains("waveform"))      GetParam(oscParams[i][0])->Set(waveformIndex(o["waveform"].get<std::string>()));
      if (o.contains("octave"))        GetParam(oscParams[i][1])->Set(o["octave"].get<int>());
      if (o.contains("fine_tune"))     GetParam(oscParams[i][2])->Set(o["fine_tune"].get<double>());
      if (o.contains("unison_voices")) GetParam(oscParams[i][3])->Set(o["unison_voices"].get<int>());
      if (o.contains("unison_spread")) GetParam(oscParams[i][4])->Set(o["unison_spread"].get<double>());
      if (o.contains("volume"))        GetParam(oscParams[i][5])->Set(o["volume"].get<double>());
    }

    // ── Filter ───────────────────────────────────────────────────────────────
    if (j.contains("filter") && j["filter"].is_object())
    {
      const auto& f = j["filter"];
      if (f.contains("type"))       GetParam(kFilterType)->Set(filterTypeIndex(f["type"].get<std::string>()));
      if (f.contains("cutoff_hz"))  GetParam(kFilterCutoff)->Set(f["cutoff_hz"].get<double>());
      if (f.contains("resonance"))  GetParam(kFilterResonance)->Set(f["resonance"].get<double>());
      if (f.contains("env_amount")) GetParam(kFilterEnvAmount)->Set(f["env_amount"].get<double>());
      if (f.contains("drive"))      GetParam(kFilterDrive)->Set(f["drive"].get<double>());
    }

    // ── Envelopes ─────────────────────────────────────────────────────────────
    // Server uses attack_ms / decay_ms / release_ms (ms), sustain (0-1)
    auto applyEnv = [&](const json& env, int a, int d, int s, int r) {
      if (env.contains("attack_ms"))  GetParam(a)->Set(env["attack_ms"].get<double>());
      if (env.contains("decay_ms"))   GetParam(d)->Set(env["decay_ms"].get<double>());
      if (env.contains("sustain"))    GetParam(s)->Set(env["sustain"].get<double>());
      if (env.contains("release_ms")) GetParam(r)->Set(env["release_ms"].get<double>());
    };
    if (j.contains("filter_env") && j["filter_env"].is_object())
      applyEnv(j["filter_env"], kFilterAttack, kFilterDecay, kFilterSustain, kFilterRelease);
    if (j.contains("amp_env") && j["amp_env"].is_object())
      applyEnv(j["amp_env"], kAmpAttack, kAmpDecay, kAmpSustain, kAmpRelease);

    // ── Pitch envelope (optional) ─────────────────────────────────────────────
    if (j.contains("pitch_env") && j["pitch_env"].is_object()) {
      const auto& pe = j["pitch_env"];
      if (pe.contains("amount_semitones")) GetParam(kPitchEnvAmount)->Set(pe["amount_semitones"].get<double>());
      if (pe.contains("attack_ms"))        GetParam(kPitchEnvAttack)->Set(pe["attack_ms"].get<double>());
      if (pe.contains("decay_ms"))         GetParam(kPitchEnvDecay)->Set(pe["decay_ms"].get<double>());
    }

    // ── LFOs ─────────────────────────────────────────────────────────────────
    const char* lfoKeys[] = {"lfo1", "lfo2"};
    const int lfoParams[][4] = {
      {kLFO1Shape, kLFO1Rate, kLFO1Depth, kLFO1Target},
      {kLFO2Shape, kLFO2Rate, kLFO2Depth, kLFO2Target},
    };
    for (int i = 0; i < 2; ++i)
    {
      if (!j.contains(lfoKeys[i])) continue;
      const auto& l = j[lfoKeys[i]];
      if (l.contains("shape"))   GetParam(lfoParams[i][0])->Set(lfoShapeIndex(l["shape"].get<std::string>()));
      if (l.contains("rate_hz")) GetParam(lfoParams[i][1])->Set(l["rate_hz"].get<double>());
      if (l.contains("depth"))   GetParam(lfoParams[i][2])->Set(l["depth"].get<double>());
      if (l.contains("target"))  GetParam(lfoParams[i][3])->Set(lfoTargetIndex(l["target"].get<std::string>()));
    }

    // ── Effects ───────────────────────────────────────────────────────────────
    if (j.contains("effects") && j["effects"].is_object())
    {
      const auto& fx = j["effects"];
      auto setFX = [&](const char* key, int param) {
        if (fx.contains(key)) GetParam(param)->Set(fx[key].get<double>());
      };
      setFX("chorus_depth",     kChorusDepth);
      setFX("chorus_mix",       kChorusMix);
      setFX("reverb_size",      kReverbSize);
      setFX("reverb_mix",       kReverbMix);
      setFX("delay_time_ms",    kDelayTime);
      setFX("delay_feedback",   kDelayFeedback);
      setFX("delay_mix",        kDelayMix);
      setFX("saturation_drive", kSaturationDrive);
      setFX("eq_low_shelf_db",  kEQLowShelf);
      setFX("eq_high_shelf_db", kEQHighShelf);
    }

    // Inform host and refresh UI
    for (int i = 0; i < kNumParams; ++i)
    {
      BeginInformHostOfParamChange(i);
      InformHostOfParamChange(i, GetParam(i)->GetNormalized());
      EndInformHostOfParamChange(i);
    }
    SendCurrentParamValuesFromDelegate();
  }
  catch (const json::exception& e)
  {
    SetStatus(std::string("Parse error: ") + e.what());
  }
}
