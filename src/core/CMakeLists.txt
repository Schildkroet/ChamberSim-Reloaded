# core library (no GUI): file formats, database access, ballistics solver
add_library(csrcore STATIC
    loadfile.h
    numeric.h
    loadfile.cpp
    solver.h
    solver.cpp
    simconfig.h
    simconfig.cpp
    loadspec.h
    loadbuilder.h
    loadbuilder.cpp
    report.h
    report.cpp
    wiki.h
    wiki.cpp
    reportformat.h
    reportformat.cpp
    units.h
    units.cpp
    language.h
    language.cpp
    reference.h
    reference.cpp
    database.h
    database.cpp
    measurement.h
    measurement.cpp
    dates.h
    dates.cpp
    chronograph.h
    chronograph.cpp
    log.h
    log.cpp
    dbfields.h
    dbfields.cpp
    projectdata.h
    projectdata.cpp
    burnfactors.h
    burnfactors.cpp
    powderfit.h
    powderfit.cpp
    fieldnames.h
    fieldnames.cpp
    physicalpowders.h
    physicalpowders.cpp
    shotparts.h
    shotparts.cpp
)

target_include_directories(csrcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(csrcore PUBLIC Qt6::Core Qt6::Xml Qt6::Sql)
target_compile_options(csrcore PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)
