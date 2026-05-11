cmake_minimum_required(VERSION 3.22)
project(SingingPracticeTool VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/permissive- /Zc:__cplusplus /utf-8)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Optional ASIO SDK detection (Windows only; Steinberg license forbids redistribution).
# Looks for third_party/asiosdk/common/iasiodrv.h or any subdir containing it.
set(SP_ASIO_FOUND OFF)
set(SP_ASIO_INCLUDE_DIR "")
if(WIN32)
    set(_asio_root "${CMAKE_SOURCE_DIR}/third_party/asiosdk")
    if(EXISTS "${_asio_root}/common/iasiodrv.h")
        set(SP_ASIO_INCLUDE_DIR "${_asio_root}/common")
    elseif(EXISTS "${_asio_root}")
        file(GLOB_RECURSE _asio_hdr "${_asio_root}/iasiodrv.h")
        if(_asio_hdr)
            list(GET _asio_hdr 0 _asio_h)
            get_filename_component(SP_ASIO_INCLUDE_DIR "${_asio_h}" DIRECTORY)
        endif()
    endif()

    if(SP_ASIO_INCLUDE_DIR)
        set(SP_ASIO_FOUND ON)
        message(STATUS "SingingPractice: ASIO SDK found at ${SP_ASIO_INCLUDE_DIR}")
    else()
        message(STATUS "SingingPractice: ASIO SDK NOT found at third_party/asiosdk/")
        message(STATUS "    ASIO disabled \xE2\x80\x94 falling back to WASAPI / DirectSound.")
        message(STATUS "    To enable, download from https://www.steinberg.net/asiosdk")
        message(STATUS "    and unzip so that third_party/asiosdk/common/iasiodrv.h exists.")
    endif()
endif()

juce_add_gui_app(App
    PRODUCT_NAME  "SingingPracticeTool"
    COMPANY_NAME  "SingingPracticeTool"
    BUNDLE_ID     "com.singingpracticetool.app"
    VERSION       ${PROJECT_VERSION}
    ICON_BIG      "${CMAKE_SOURCE_DIR}/resources/Icon.png"
    ICON_SMALL    "${CMAKE_SOURCE_DIR}/resources/Icon.png"
)

juce_generate_juce_header(App)

target_sources(App PRIVATE
    Source/Main.cpp
    Source/App/MainWindow.cpp
    Source/App/TabsComponent.cpp
    Source/Audio/Engine.cpp
    Source/Audio/DeviceSettings.cpp
    Source/Audio/Recorder.cpp
    Source/Pitch/PitchDetector.cpp
    Source/VST/PluginHost.cpp
    Source/VST/PluginScanCache.cpp
    Source/UI/PianoRoll.cpp
    Source/UI/PluginChainView.cpp
    Source/UI/PluginEditorWindow.cpp
    Source/UI/ModernLookAndFeel.cpp
    Source/UI/PracticeTab.cpp
    Source/UI/StemTab.cpp
    Source/UI/TransportScrubber.cpp
    Source/UI/VocalToMidiTab.cpp
    Source/UI/SettingsTab.cpp
    Source/Sidecar/SidecarProcess.cpp
    Source/Sidecar/JsonRpcClient.cpp
    Source/Sidecar/Backend.cpp
)

target_include_directories(App PRIVATE Source)

target_compile_definitions(App PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:App,JUCE_PROJECT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:App,JUCE_VERSION>"
    JUCE_PLUGINHOST_VST3=1
    $<$<PLATFORM_ID:Darwin>:JUCE_PLUGINHOST_AU=1>
    $<$<BOOL:${SP_ASIO_FOUND}>:JUCE_ASIO=1>
)

if(SP_ASIO_FOUND)
    target_include_directories(App PRIVATE "${SP_ASIO_INCLUDE_DIR}")
endif()

target_link_libraries(App PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
