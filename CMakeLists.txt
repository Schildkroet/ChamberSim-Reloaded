cmake_minimum_required(VERSION 3.16)

# The application version: the single place to change it (shown in Help > About, bundle info, ...).
set(CSR_VERSION "0.13")

project(ChamberSimReloaded VERSION ${CSR_VERSION} LANGUAGES CXX)

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
    # Since Visual Studio 2022 17.10 std::mutex has a constexpr constructor that crashes at the first lock() when the program runs with an
    # older msvcp140.dll (the Visual C++ runtime of the user's PC; windeployqt does not ship it): the loads crashed in physicalpowders.
    add_compile_definitions(_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR)
endif()
find_program(CSR_CCACHE ccache)
if(CSR_CCACHE)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CSR_CCACHE})
endif()
option(CSR_UNITY_BUILD "Combine source files for faster full builds" OFF)
if(CSR_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
endif()
if(CMAKE_GENERATOR MATCHES "Makefiles" AND NOT DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL} AND NOT DEFINED ENV{MAKEFLAGS})
    message(STATUS "Tip: Makefile generators build serially unless you pass -j<N>; "
                   "use 'cmake --preset debug' (Ninja) or 'cmake --build . --parallel'.")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Xml Charts PrintSupport Test LinguistTools)

# ---- core library (src/core) and GUI library (src/app) ----
add_subdirectory(src/core)
add_subdirectory(src/app)

# flags for the language menu (PNG renderings of resources/flags/*.svg)
file(GLOB CSR_FLAG_ICONS RELATIVE "${CMAKE_SOURCE_DIR}" CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/resources/flags/*.png")
qt_add_resources(csrui "flags" PREFIX "/" FILES ${CSR_FLAG_ICONS})

qt_add_executable(ChamberSimReloaded
    MANUAL_FINALIZATION
    src/app/main.cpp
)

target_link_libraries(ChamberSimReloaded PRIVATE csrui)

# the program icon (PNG renderings of packaging/linux/chambersimreloaded.svg: no SVG plugin needed)
qt_add_resources(ChamberSimReloaded "icons" PREFIX "/"
    FILES resources/icons/chambersimreloaded-32.png resources/icons/chambersimreloaded-64.png resources/icons/chambersimreloaded-256.png)

# the project's records (data/<calibers|projectiles|propellants>/*.json), read at startup as ":/data/..."
file(GLOB_RECURSE CSR_PROJECT_DATA CONFIGURE_DEPENDS RELATIVE "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/data/*.json")
if(CSR_PROJECT_DATA)
    qt_add_resources(ChamberSimReloaded "projectdata" PREFIX "/" FILES ${CSR_PROJECT_DATA})
endif()

# Windows: the icon and version information of the .exe (the Explorer icon; version info also lowers antivirus false positives)
if(WIN32)
    set(CSR_ICON_FILE "${CMAKE_SOURCE_DIR}/resources/icons/chambersimreloaded.ico")
    configure_file(resources/icons/chambersimreloaded.rc.in "${CMAKE_BINARY_DIR}/chambersimreloaded.rc" @ONLY)
    target_sources(ChamberSimReloaded PRIVATE "${CMAKE_BINARY_DIR}/chambersimreloaded.rc")
endif()

set_target_properties(ChamberSimReloaded PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS ChamberSimReloaded
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# the user manual (Help > User manual): next to the executable on Windows, in the data directory elsewhere
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/docs/manual/manual.pdf)
    if(WIN32)
        install(FILES docs/manual/manual.pdf DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        install(FILES docs/manual/manual.pdf DESTINATION ${CMAKE_INSTALL_DATADIR}/ChamberSimReloaded)
    endif()
endif()

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/chambersimreloaded-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
endif()

# ---- translations ----
# American English (en_US) is the source language and the default. To add a language, append its
# code here (e.g. de_DE), run the `update_translations` target to create/refresh
# translations/ChamberSimReloaded_<lang>.ts, translate it in Qt Linguist, and rebuild: the .qm files are
# generated and embedded automatically (resource prefix :/i18n) and show up in View > Language.
set(CSR_LANGUAGES en_US de_DE fr_FR it_IT)
set(CSR_TS_FILES "")
foreach(lang IN LISTS CSR_LANGUAGES)
    list(APPEND CSR_TS_FILES ${CMAKE_SOURCE_DIR}/translations/ChamberSimReloaded_${lang}.ts)
endforeach()
# the sources of the libraries are listed relative to their own directories
get_target_property(CSR_APP_SOURCES csrui SOURCES)
list(TRANSFORM CSR_APP_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/app/")
list(APPEND CSR_APP_SOURCES src/app/main.cpp)
get_target_property(CSR_CORE_SOURCES csrcore SOURCES)
list(TRANSFORM CSR_CORE_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/src/core/")
qt_add_translations(ChamberSimReloaded
    TS_FILES ${CSR_TS_FILES}
    SOURCES ${CSR_APP_SOURCES} ${CSR_CORE_SOURCES}
    RESOURCE_PREFIX /i18n
    INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/app
    LUPDATE_OPTIONS -source-language en_US -locations relative -no-obsolete
)

qt_finalize_executable(ChamberSimReloaded)

# ---- developer tool: compare the solver with the reference runs of the original program ----
# checks the project's record files (used by CI): csr_datacheck [data directory]
add_executable(csr_datacheck tools/csr_datacheck.cpp)
target_link_libraries(csr_datacheck PRIVATE csrcore)
target_compile_definitions(csr_datacheck PRIVATE CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# fits a propellant model to the measurements in *.grtload files: csr_fitpowder --help
add_executable(csr_fitpowder tools/csr_fitpowder.cpp)
target_link_libraries(csr_fitpowder PRIVATE csrcore)
install(TARGETS csr_fitpowder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# fits a propellant model to published load data (CSV from scripts/data/loaddata): csr_fitdata --help
add_executable(csr_fitdata tools/csr_fitdata.cpp)
target_link_libraries(csr_fitdata PRIVATE csrcore)
target_compile_definitions(csr_fitdata PRIVATE CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# simulates published load data with an adjustable solver calibration (analysis/calibrate_published.py)
add_executable(csr_eval tools/csr_eval.cpp)
target_link_libraries(csr_eval PRIVATE csrcore)
target_compile_definitions(csr_eval PRIVATE CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# simulates loads of records of the original's database (analysis/compare_grt_published.py): csr_dbeval --help
add_executable(csr_dbeval tools/csr_dbeval.cpp)
target_link_libraries(csr_dbeval PRIVATE csrcore)

# headless simulation for scripts: csr_sim --help
add_executable(csr_sim tools/csr_sim.cpp)
target_link_libraries(csr_sim PRIVATE csrcore)
if(CSR_PROJECT_DATA)  # the physical burn values and caliber factors as in the program (":/data/physical/...")
    qt_add_resources(csr_sim "projectdata" PREFIX "/" FILES ${CSR_PROJECT_DATA})
endif()
install(TARGETS csr_sim RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(grt_compare tools/grt_compare.cpp)
target_link_libraries(grt_compare PRIVATE csrcore)
target_compile_definitions(grt_compare PRIVATE CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# ---- formatting: clang-format (see .clang-format) runs on the sources before every build ----
# It only rewrites files that change. Turn it off with -DCSR_FORMAT_ON_BUILD=OFF (CI builds that must not touch the tree).
option(CSR_FORMAT_ON_BUILD "Run clang-format -i on the sources before every build" ON)
find_program(CSR_CLANG_FORMAT clang-format)
if(CSR_FORMAT_ON_BUILD AND CSR_CLANG_FORMAT)
    file(GLOB_RECURSE CSR_FORMAT_FILES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h" "${CMAKE_SOURCE_DIR}/src/*.cpp"
         "${CMAKE_SOURCE_DIR}/tests/*.cpp" "${CMAKE_SOURCE_DIR}/tools/*.cpp")
    add_custom_target(clang_format
        COMMAND ${CSR_CLANG_FORMAT} -i --style=file ${CSR_FORMAT_FILES}
        COMMENT "clang-format"
        VERBATIM)
    foreach(t csrcore csrui ChamberSimReloaded csr_datacheck csr_fitpowder csr_fitdata csr_eval csr_dbeval csr_sim grt_compare)
        add_dependencies(${t} clang_format)
    endforeach()
elseif(CSR_FORMAT_ON_BUILD)
    message(STATUS "clang-format not found: the sources are not formatted by the build")
endif()

# ---- tests ----
enable_testing()
add_subdirectory(tests)
