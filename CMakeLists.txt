cmake_minimum_required(VERSION 3.21.0)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_TRY_COMPILE_CONFIGURATION "Release")

include(target_add_versioninfo)
include(generate_ipp_translation_units)
include(configure_msvc_wine_intellisense)

project(Bonus_Tiers VERSION "1.0.0.0" DESCRIPTION "Bonus Tiers - training cost from trained points only (Gothic I / II NotR, Union)")
enable_language(C CXX RC)

option(GOTHIC_API_G1  "Compile the plugin inside Gothic 1 namespace" ON)
option(GOTHIC_API_G1A "Compile the plugin inside Gothic 1 Addon (Gothic Sequel) namespace" ON)
option(GOTHIC_API_G2  "Compile the plugin inside Gothic 2 Classic namespace" ON)
option(GOTHIC_API_G2A "Compile the plugin inside Gothic 2 Addon (Night of the Raven) namespace" ON)

set(GENERATE_UNION_API_DLL OFF)
add_library(${CMAKE_PROJECT_NAME} SHARED)

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/src/generated_ipp")
set(SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/DllMain.cpp")
file(GLOB_RECURSE HDR_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE IPP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ipp")

generate_ipp_translation_units(INPUT ${IPP_FILES} OUTPUT IPP_TRANSLATION_UNITS)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SRC_FILES} ${HDR_FILES} ${IPP_FILES} ${IPP_TRANSLATION_UNITS})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/userapi")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
    PROJECT_NAME="${CMAKE_PROJECT_NAME}"
    PROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
)
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE /Zc:preprocessor /MP
    $<$<CONFIG:Debug>:/Od /Ob0 /RTC1>
    $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /GL /GF /Gy /DNDEBUG>
    $<$<CONFIG:RelWithDebInfo>:/O2 /Ob1 /Oi /Ot /GL /GF /Gy /DNDEBUG>
)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:/INCREMENTAL>
    $<$<CONFIG:Release>:/OPT:REF /OPT:ICF /LTCG /INCREMENTAL:NO /DEBUG:NONE>
    $<$<CONFIG:RelWithDebInfo>:/OPT:REF /OPT:ICF /LTCG /INCREMENTAL:NO /DEBUG>
)
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE <Union/Hook.h> <Union/Stream.h> <ZenGin/zGothicAPI.h>)
target_add_versioninfo(${CMAKE_PROJECT_NAME}
    VERSION ${CMAKE_PROJECT_VERSION}
    COMPANY_NAME "PEN996"
    PRODUCT_NAME "Bonus Tiers"
    FILE_DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}"
)

add_subdirectory(dependencies)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SRC_FILES} ${HDR_FILES} ${IPP_FILES})
source_group(TREE "${GENERATED_DIR}" PREFIX "Generated Files" FILES ${IPP_TRANSLATION_UNITS})
