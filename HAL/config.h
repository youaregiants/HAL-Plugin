#define PLUG_NAME "HAL"
#define PLUG_MFR "YourName"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'HALx'
#define PLUG_MFR_ID 'YrNm'
#define PLUG_URL_STR "https://github.com/youaregiants/HAL-Plugin"
#define PLUG_EMAIL_STR "your@email.com"
#define PLUG_COPYRIGHT_STR "Copyright 2026"
#define PLUG_CLASS_NAME HAL

#define BUNDLE_NAME "HAL"
#define BUNDLE_MFR "YourName"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "HAL"

#define PLUG_CHANNEL_IO "0-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE SYNTH
#define PLUG_DOES_MIDI_IN 1
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 600
#define PLUG_HEIGHT 400
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY HAL_Entry
#define AUV2_ENTRY_STR "HAL_Entry"
#define AUV2_FACTORY HAL_Factory
#define AUV2_VIEW_CLASS HAL_View
#define AUV2_VIEW_CLASS_STR "HAL_View"

#define AAX_TYPE_IDS 'HAL1', 'HAL2'
#define AAX_TYPE_IDS_AUDIOSUITE 'HLA1', 'HLA2'
#define AAX_PLUG_MFR_STR "YourName"
#define AAX_PLUG_NAME_STR "HAL\nHALx"
#define AAX_PLUG_CATEGORY_STR "SoundField"
#define AAX_DOES_AUDIOSUITE 0

#define VST3_SUBCATEGORY "Instrument|Synth"

#define CLAP_MANUAL_URL "https://iplug2.github.io/manuals/example_manual.pdf"
#define CLAP_SUPPORT_URL "https://github.com/iPlug2/iPlug2/wiki"
#define CLAP_DESCRIPTION "Natural language synthesizer — describe a sound, HAL generates the patch"
#define CLAP_FEATURES "instrument", "synthesizer"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
