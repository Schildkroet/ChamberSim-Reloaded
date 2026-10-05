add_executable(tst_core tst_core.cpp)
target_link_libraries(tst_core PRIVATE grtcore Qt6::Test)
target_compile_definitions(tst_core PRIVATE
    GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GRT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME core COMMAND tst_core)

add_executable(tst_gui tst_gui.cpp)
target_link_libraries(tst_gui PRIVATE grtui Qt6::Test)
target_compile_definitions(tst_gui PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GRT_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    GRT_QM_DIR="${CMAKE_BINARY_DIR}")  # the .qm files (embedded only into the program): the English plural forms
add_dependencies(tst_gui release_translations)
add_test(NAME gui COMMAND tst_gui)
set_tests_properties(gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the project's record files must all be usable
add_test(NAME projectdata COMMAND grt_datacheck "${CMAKE_SOURCE_DIR}/data")

if(TARGET clang_format)
    add_dependencies(tst_core clang_format)
    add_dependencies(tst_gui clang_format)
endif()
