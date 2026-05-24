cmake_minimum_required(VERSION 3.21)
project(Voxanova VERSION 0.1.109)

if(NOT DEFINED JUCE_DIR)
  message(FATAL_ERROR "Set JUCE_DIR to your local JUCE checkout before configuring this target.")
endif()

add_subdirectory(${JUCE_DIR} JUCE)

set(VOXANOVA_PLUGIN_FORMATS VST3 Standalone)

if(APPLE)
  list(PREPEND VOXANOVA_PLUGIN_FORMATS AU)
endif()

juce_add_plugin(Voxanova
  COMPANY_NAME "Voxanova"
  BUNDLE_ID com.voxanova.vocalchain
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
  NEEDS_WEB_BROWSER TRUE
  NEEDS_WEBVIEW2 TRUE
  PLUGIN_MANUFACTURER_CODE VxNv
  PLUGIN_CODE VxCh
  # VST3 and AU are the production targets. Legacy VST2 requires Steinberg's
  # discontinued VST2 SDK/license and is intentionally not enabled by default.
  FORMATS ${VOXANOVA_PLUGIN_FORMATS}
  PRODUCT_NAME "Voxanova"
)

file(GLOB_RECURSE VOXANOVA_WEB_ASSETS
  CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../dist/*"
)

juce_add_binary_data(VoxanovaWebAssets
  SOURCES
    ${VOXANOVA_WEB_ASSETS}
)

target_sources(Voxanova
  PRIVATE
    source/FatTuneEngine.cpp
    source/FatTuneEngine.h
    source/PluginEditor.cpp
    source/PluginEditor.h
    source/PluginProcessor.cpp
    source/PluginProcessor.h
)

target_compile_features(Voxanova PRIVATE cxx_std_20)

target_compile_definitions(Voxanova
  PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    $<$<PLATFORM_ID:Windows>:JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1>
)

target_link_libraries(Voxanova
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    VoxanovaWebAssets
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
