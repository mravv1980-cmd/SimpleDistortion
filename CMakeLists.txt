cmake_minimum_required(VERSION 3.22)
project(SimpleDistortion VERSION 1.0.0)

option(COPY_AFTER_BUILD "Copy plugin to system folder after build" ON)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SimpleDistortion
    COMPANY_NAME "MyCompany"
    PLUGIN_MANUFACTURER_CODE Myco
    PLUGIN_CODE Sdst
    FORMATS VST3 Standalone
    PRODUCT_NAME "Simple Distortion"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(SimpleDistortion PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp)

target_compile_definitions(SimpleDistortion PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SimpleDistortion
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
