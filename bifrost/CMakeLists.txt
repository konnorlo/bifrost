cmake_minimum_required(VERSION 3.27)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version supported by Bifrost")
endif()

project(Bifrost VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BIFROST_ENABLE_ONNX "Enable optional ONNX Runtime import-time timbre encoder" OFF)
option(BIFROST_ENABLE_MINIAUDIO "Enable optional miniaudio file decode backend" OFF)
option(BIFROST_BUILD_STANDALONE "Build standalone target for debug" OFF)
option(BIFROST_BUILD_TESTS "Build Bifrost DSP/model test executable" ON)
option(BIFROST_BUILD_PLUGIN_SMOKE "Build local host-style VST3 smoke loader" ON)
set(BIFROST_ONNX_MODEL_PATH "" CACHE FILEPATH "Optional TinyTimbreEncoder ONNX model path")

set(BIFROST_GIT_SHA "unknown")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short=12 HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BIFROST_GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()

if(BIFROST_GIT_SHA STREQUAL "")
    set(BIFROST_GIT_SHA "unknown")
endif()

string(TIMESTAMP BIFROST_BUILD_DATE_UTC "%Y-%m-%dT%H:%M:%SZ" UTC)
configure_file(
    Source/BuildInfo.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/BuildInfo.h
    @ONLY
)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
)
FetchContent_MakeAvailable(JUCE)

set(BIFROST_FORMATS VST3)
if(BIFROST_BUILD_STANDALONE)
    list(APPEND BIFROST_FORMATS Standalone)
endif()

juce_add_plugin(Bifrost
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "konnorlo"
    BUNDLE_ID "com.konnorlo.bifrost"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE KLoA
    PLUGIN_CODE BfSt
    FORMATS ${BIFROST_FORMATS}
    PRODUCT_NAME "Bifrost"
    VST3_CATEGORIES "Instrument" "Synth"
)

if(APPLE)
    set_target_properties(Bifrost_VST3 PROPERTIES
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.konnorlo.bifrost"
    )
endif()

juce_generate_juce_header(Bifrost)

juce_add_binary_data(BifrostFonts
    SOURCES
        assets/fonts/Rajdhani-Regular.ttf
        assets/fonts/Rajdhani-Medium.ttf
        assets/fonts/Rajdhani-SemiBold.ttf
        assets/fonts/Orbitron-Medium.ttf
        assets/background.png
)

target_sources(Bifrost PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/audio/AudioFileLoader.cpp
    Source/analysis/AnalysisController.cpp
    Source/analysis/AnalysisJob.cpp
    Source/analysis/STFT.cpp
    Source/analysis/TempoAnalyzer.cpp
    Source/analysis/TransientDetector.cpp
    Source/analysis/PitchTracker.cpp
    Source/analysis/HarmonicExtractor.cpp
    Source/analysis/NoiseExtractor.cpp
    Source/analysis/ResonatorExtractor.cpp
    Source/analysis/TimbreStateModel.cpp
    Source/model/TimbreModel.cpp
    Source/model/PresetSerializer.cpp
    Source/synth/Voice.cpp
    Source/synth/VoiceManager.cpp
    Source/synth/AdditiveOscBank.cpp
    Source/synth/NoiseBank.cpp
    Source/synth/ResonatorBank.cpp
    Source/synth/InertiaSmoother.cpp
    Source/ui/BifrostLookAndFeel.cpp
    Source/ui/AdsrEnvelopeView.cpp
    Source/ui/SampleDropTarget.cpp
    Source/ui/StateMapView.cpp
    Source/ui/WaveformView.cpp
    Source/ml/TinyTimbreEncoder.cpp
)

target_include_directories(Bifrost PRIVATE
    Source
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(Bifrost PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    BIFROST_ENABLE_ONNX=$<BOOL:${BIFROST_ENABLE_ONNX}>
    BIFROST_ONNX_MODEL_PATH="${BIFROST_ONNX_MODEL_PATH}"
    BIFROST_ENABLE_MINIAUDIO=$<BOOL:${BIFROST_ENABLE_MINIAUDIO}>
)

target_link_libraries(Bifrost PRIVATE
    BifrostFonts
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(APPLE)
    add_custom_command(TARGET Bifrost_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DVST3_BUNDLE=$<TARGET_BUNDLE_DIR:Bifrost_VST3> -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SanitizeVst3ModuleInfo.cmake"
        COMMAND /usr/bin/codesign --force --sign - --deep $<TARGET_BUNDLE_DIR:Bifrost_VST3>
        COMMAND ${CMAKE_COMMAND} -DVST3_BUNDLE=$ENV{HOME}/Library/Audio/Plug-Ins/VST3/Bifrost.vst3 -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SanitizeVst3ModuleInfo.cmake"
        COMMAND /usr/bin/codesign --force --sign - --deep $ENV{HOME}/Library/Audio/Plug-Ins/VST3/Bifrost.vst3
        VERBATIM
    )
endif()

if(BIFROST_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(BifrostTests
        PRODUCT_NAME "Bifrost Tests"
    )

    juce_generate_juce_header(BifrostTests)

    target_sources(BifrostTests PRIVATE
        tests/analysis_tests.cpp
        Source/analysis/AnalysisJob.cpp
        Source/analysis/STFT.cpp
        Source/analysis/TempoAnalyzer.cpp
        Source/analysis/PitchTracker.cpp
        Source/analysis/HarmonicExtractor.cpp
        Source/analysis/NoiseExtractor.cpp
        Source/analysis/ResonatorExtractor.cpp
        Source/analysis/TimbreStateModel.cpp
        Source/ml/TinyTimbreEncoder.cpp
        Source/model/PresetSerializer.cpp
        Source/synth/VoiceManager.cpp
        Source/synth/Voice.cpp
        Source/synth/AdditiveOscBank.cpp
        Source/synth/NoiseBank.cpp
        Source/synth/ResonatorBank.cpp
        Source/synth/InertiaSmoother.cpp
    )

    target_include_directories(BifrostTests PRIVATE
        Source
        ${CMAKE_CURRENT_BINARY_DIR}/generated
    )

    target_compile_definitions(BifrostTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        BIFROST_ENABLE_ONNX=0
        BIFROST_ONNX_MODEL_PATH=""
    )

    target_link_libraries(BifrostTests PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(NAME BifrostTests COMMAND $<TARGET_FILE:BifrostTests>)
endif()

if(BIFROST_BUILD_PLUGIN_SMOKE)
    juce_add_console_app(BifrostPluginSmoke
        PRODUCT_NAME "Bifrost Plugin Smoke"
    )

    juce_generate_juce_header(BifrostPluginSmoke)

    target_sources(BifrostPluginSmoke PRIVATE
        tools/PluginLoadSmoke.cpp
    )

    target_compile_definitions(BifrostPluginSmoke PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
    )

    target_link_libraries(BifrostPluginSmoke PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )
endif()
