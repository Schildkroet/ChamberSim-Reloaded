cmake_minimum_required(VERSION 3.16)

# The application version: the single place to change it (shown in Help > About, bundle info, ...).
set(GRT_VERSION "0.1")

project(GRTReloaded VERSION ${GRT_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    src/core/dbfields.h
    src/core/dbfields.cpp
    src/core/projectdata.h
    src/core/projectdata.cpp
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
    src/app/recorddialog.h
    src/app/recorddialog.cpp
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
    src/app/theme.cpp
)
target_include_directories(grtui PUBLIC src/app)
target_compile_definitions(grtui PUBLIC GRT_VERSION="${GRT_VERSION}")
target_link_libraries(grtui PUBLIC grtcore Qt6::Widgets Qt6::Charts Qt6::PrintSupport)

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
set(GRT_LANGUAGES en_US)
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

add_executable(grt_compare tools/grt_compare.cpp)
target_link_libraries(grt_compare PRIVATE grtcore)
target_compile_definitions(grt_compare PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# ---- tests ----
enable_testing()
add_subdirectory(tests)
