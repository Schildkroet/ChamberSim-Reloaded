cmake_minimum_required(VERSION 3.16)

# The application version: the single place to change it (shown in Help > About, bundle info, ...).
set(GRT_VERSION "0.2")

project(GRTReloaded VERSION ${GRT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Release builds use -O2 (CMake's default would be -O3).
if(NOT MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "Flags used by the CXX compiler during Release builds." FORCE)
endif()

# ---- build speed: use all cores and cache compiler output ----
# Ninja (see CMakePresets.json) builds in parallel on all cores by default. With "Unix Makefiles" pass
# -j<N> or set CMAKE_BUILD_PARALLEL_LEVEL (cmake --build . --parallel).
if(MSVC)
    add_compile_options(/MP)  # compile the files of one target in parallel
endif()
find_program(GRT_CCACHE ccache)
if(GRT_CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${GRT_CCACHE})
endif()
option(GRT_UNITY_BUILD "Combine source files for faster full builds" OFF)
if(GRT_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
endif()
if(CMAKE_GENERATOR MATCHES "Makefiles" AND NOT DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL} AND NOT DEFINED ENV{MAKEFLAGS})
    message(STATUS "Tip: Makefile generators build serially unless you pass -j<N>; "
                   "use 'cmake --preset debug' (Ninja) or 'cmake --build . --parallel'.")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Xml Charts PrintSupport Test LinguistTools)

# ---- core library (no GUI): file formats, database access, ballistics solver ----
add_library(grtcore STATIC
    src/core/loadfile.h
    src/core/numeric.h
    src/core/loadfile.cpp
    src/core/solver.h
    src/core/solver.cpp
    src/core/simconfig.h
    src/core/simconfig.cpp
    src/core/loadspec.h
    src/core/loadbuilder.h
    src/core/loadbuilder.cpp
    src/core/report.h
    src/core/report.cpp
    src/core/wiki.h
    src/core/wiki.cpp
    src/core/reportformat.h
    src/core/reportformat.cpp
    src/core/units.h
    src/core/units.cpp
    src/core/language.h
    src/core/language.cpp
    src/core/reference.h
    src/core/reference.cpp
    src/core/database.h
    src/core/database.cpp
    src/core/measurement.h
    src/core/measurement.cpp
    src/core/dates.h
    src/core/dates.cpp
    src/core/chronograph.h
    src/core/chronograph.cpp
    src/core/log.h
    src/core/log.cpp
    src/core/dbfields.h
    src/core/dbfields.cpp
    src/core/projectdata.h
    src/core/projectdata.cpp
    src/core/burnfactors.h
    src/core/burnfactors.cpp
    src/core/powderfit.h
    src/core/powderfit.cpp
    src/core/fieldnames.h
    src/core/fieldnames.cpp
    src/core/physicalpowders.h
    src/core/physicalpowders.cpp
)
target_include_directories(grtcore PUBLIC src/core)
target_link_libraries(grtcore PUBLIC Qt6::Core Qt6::Xml Qt6::Sql)
target_compile_options(grtcore PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

# ---- application: GUI library (testable) + thin executable ----
add_library(grtui STATIC
    src/app/mainwindow.cpp
    src/app/mainwindow.h
    src/app/mainwindow.ui
    src/app/curvechart.h
    src/app/curvechart.cpp
    src/app/tablebrowser.h
    src/app/tablebrowser.cpp
    src/app/loadview.h
    src/app/loadview.cpp
    src/app/reportview.h
    src/app/reportview.cpp
    src/app/reporttemplatedialog.h
    src/app/reporttemplatedialog.cpp
    src/app/chamberview.h
    src/app/chamberview.cpp
    src/app/cartridgeview.h
    src/app/cartridgeview.cpp
    src/app/propertypanel.h
    src/app/propertypanel.cpp
    src/app/vivacitychart.h
    src/app/vivacitychart.cpp
    src/app/measurementchart.h
    src/app/measurementchart.cpp
    src/app/measurementview.h
    src/app/measurementview.cpp
    src/app/photoview.h
    src/app/photoview.cpp
    src/app/chronographimportdialog.h
    src/app/chronographimportdialog.cpp
    src/app/recorddialog.h
    src/app/recorddialog.cpp
    src/app/powderfitdialog.h
    src/app/powderfitdialog.cpp
    src/app/solverprefs.h
    src/app/solverprefs.cpp
    src/app/burnratedialog.h
    src/app/burnratedialog.cpp
    src/app/selectrecorddialog.h
    src/app/selectrecorddialog.cpp
    src/app/newloaddialog.h
    src/app/newloaddialog.cpp
    src/app/translation.h
    src/app/translation.cpp
    src/app/unitprefs.h
    src/app/unitprefs.cpp
    src/app/about.h
    src/app/about.cpp
    src/app/appfont.h
    src/app/appfont.cpp
    src/app/settingsdialog.h
    src/app/settingsdialog.cpp
    src/app/theme.h
    src/app/titlebar.h
    src/app/titlebar.cpp
    src/app/flags.h
    src/app/flags.cpp
    src/app/theme.cpp
)
target_include_directories(grtui PUBLIC src/app)
target_compile_definitions(grtui PUBLIC GRT_VERSION="${GRT_VERSION}")
target_link_libraries(grtui PUBLIC grtcore Qt6::Widgets Qt6::Charts Qt6::PrintSupport)

# flags for the language menu (PNG renderings of resources/flags/*.svg)
file(GLOB GRT_FLAG_ICONS RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/flags/*.png")
qt_add_resources(grtui "flags" PREFIX "/" FILES ${GRT_FLAG_ICONS})

qt_add_executable(GRTReloaded
    MANUAL_FINALIZATION
    src/app/main.cpp
)

target_link_libraries(GRTReloaded PRIVATE grtui)

# the program icon (PNG renderings of packaging/linux/grtreloaded.svg: no SVG plugin needed)
qt_add_resources(GRTReloaded "icons" PREFIX "/"
    FILES resources/icons/grtreloaded-32.png resources/icons/grtreloaded-64.png resources/icons/grtreloaded-256.png)

# the project's records (data/<calibers|projectiles|propellants>/*.json), read at startup as ":/data/..."
file(GLOB_RECURSE GRT_PROJECT_DATA CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/data/*.json")
if(GRT_PROJECT_DATA)
    qt_add_resources(GRTReloaded "projectdata" PREFIX "/" FILES ${GRT_PROJECT_DATA})
endif()

set_target_properties(GRTReloaded PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS GRTReloaded
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---- translations ----
# American English (en_US) is the source language and the default. To add a language, append its
# code here (e.g. de_DE), run the `update_translations` target to create/refresh
# translations/GRTReloaded_<lang>.ts, translate it in Qt Linguist, and rebuild: the .qm files are
# generated and embedded automatically (resource prefix :/i18n) and show up in View > Language.
set(GRT_LANGUAGES en_US de_DE fr_FR it_IT)
set(GRT_TS_FILES "")
foreach(lang IN LISTS GRT_LANGUAGES)
    list(APPEND GRT_TS_FILES ${CMAKE_SOURCE_DIR}/translations/GRTReloaded_${lang}.ts)
endforeach()
get_target_property(GRT_APP_SOURCES grtui SOURCES)
list(APPEND GRT_APP_SOURCES src/app/main.cpp)
get_target_property(GRT_CORE_SOURCES grtcore SOURCES)
qt_add_translations(GRTReloaded
    TS_FILES ${GRT_TS_FILES}
    SOURCES ${GRT_APP_SOURCES} ${GRT_CORE_SOURCES}
    RESOURCE_PREFIX /i18n
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/app
    LUPDATE_OPTIONS -source-language en_US -locations relative -no-obsolete
)

qt_finalize_executable(GRTReloaded)

# ---- developer tool: compare the solver with the reference runs of the original program ----
# checks the project's record files (used by CI): grt_datacheck [data directory]
add_executable(grt_datacheck tools/grt_datacheck.cpp)
target_link_libraries(grt_datacheck PRIVATE grtcore)
target_compile_definitions(grt_datacheck PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# fits a propellant model to the measurements in *.grtload files: grt_fitpowder --help
add_executable(grt_fitpowder tools/grt_fitpowder.cpp)
target_link_libraries(grt_fitpowder PRIVATE grtcore)
install(TARGETS grt_fitpowder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# fits a propellant model to published load data (CSV from tools/loaddata): grt_fitdata --help
add_executable(grt_fitdata tools/grt_fitdata.cpp)
target_link_libraries(grt_fitdata PRIVATE grtcore)
target_compile_definitions(grt_fitdata PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# simulates published load data with an adjustable solver calibration (analysis/calibrate_published.py)
add_executable(grt_eval tools/grt_eval.cpp)
target_link_libraries(grt_eval PRIVATE grtcore)
target_compile_definitions(grt_eval PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

add_executable(grt_compare tools/grt_compare.cpp)
target_link_libraries(grt_compare PRIVATE grtcore)
target_compile_definitions(grt_compare PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# ---- formatting: clang-format (see .clang-format) runs on the sources before every build ----
# It only rewrites files that change. Turn it off with -DGRT_FORMAT_ON_BUILD=OFF (CI builds that must not touch the tree).
option(GRT_FORMAT_ON_BUILD "Run clang-format -i on the sources before every build" ON)
find_program(GRT_CLANG_FORMAT clang-format)
if(GRT_FORMAT_ON_BUILD AND GRT_CLANG_FORMAT)
    file(GLOB_RECURSE GRT_FORMAT_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/src/*.cpp"
         "${CMAKE_SOURCE_DIR}/tests/*.cpp" "${CMAKE_SOURCE_DIR}/tools/*.cpp")
    add_custom_target(clang_format
        COMMAND ${GRT_CLANG_FORMAT} -i --style=file ${GRT_FORMAT_FILES}
        COMMENT "clang-format"
        VERBATIM)
    foreach(t grtcore grtui GRTReloaded grt_datacheck grt_fitpowder grt_fitdata grt_eval grt_compare)
        add_dependencies(${t} clang_format)
    endforeach()
elseif(GRT_FORMAT_ON_BUILD)
    message(STATUS "clang-format not found: the sources are not formatted by the build")
endif()

# ---- tests ----
enable_testing()
add_subdirectory(tests)
