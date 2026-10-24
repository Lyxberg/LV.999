cmake_minimum_required(VERSION 3.22)

project(Lv999 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    message(FATAL_ERROR "JUCE not found. Put the JUCE source in ${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
endif()

juce_add_plugin(Lv999
    COMPANY_NAME            "BergAudio"
    BUNDLE_ID               "com.bergaudio.lv999"
    PLUGIN_MANUFACTURER_CODE Berg
    PLUGIN_CODE             Lv99
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "LV999"
    DESCRIPTION             "One cartridge. One sample. One random LFO."
    IS_SYNTH                TRUE
    IS_MIDI_EFFECT          FALSE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_IS_FREEWARE      TRUE
    VST3_CATEGORIES         Instrument)

juce_generate_juce_header(Lv999)

target_sources(Lv999 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(Lv999 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(Lv999
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags)

# ---- offline ui renderer ----------------------------------------------------
option(LV999_BUILD_TEST "Build the offline ui test" ON)

if(LV999_BUILD_TEST)
    juce_add_console_app(Lv999Test PRODUCT_NAME "Lv999Test")
    juce_generate_juce_header(Lv999Test)

    target_include_directories(Lv999Test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Source")

    target_sources(Lv999Test PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Test/UiTest.cpp)

    target_compile_definitions(Lv999Test PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JucePlugin_Name="Lv999")

    target_link_libraries(Lv999Test
        PRIVATE
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_gui_basics
        PUBLIC
            juce::juce_recommended_config_flags)
endif()
