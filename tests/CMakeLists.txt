add_executable(tst_core tst_core.cpp)
target_link_libraries(tst_core PRIVATE grtcore Qt6::Test)
target_compile_definitions(tst_core PRIVATE
    GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GRT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME core COMMAND tst_core)

add_executable(tst_gui tst_gui.cpp)
target_link_libraries(tst_gui PRIVATE grtui Qt6::Test)
target_compile_definitions(tst_gui PRIVATE GRT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GRT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME gui COMMAND tst_gui)
set_tests_properties(gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
