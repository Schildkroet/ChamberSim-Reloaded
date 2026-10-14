add_executable(tst_core tst_core.cpp)
target_link_libraries(tst_core PRIVATE csrcore Qt6::Test)
target_compile_definitions(tst_core PRIVATE
    CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    CSR_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME core COMMAND tst_core)

add_executable(tst_gui tst_gui.cpp)
target_link_libraries(tst_gui PRIVATE csrui Qt6::Test)
target_compile_definitions(tst_gui PRIVATE CSR_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    CSR_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    CSR_QM_DIR="${CMAKE_BINARY_DIR}")  # the .qm files (embedded only into the program): the English plural forms
add_dependencies(tst_gui release_translations)
add_test(NAME gui COMMAND tst_gui)
set_tests_properties(gui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# the project's record files must all be usable
add_test(NAME projectdata COMMAND csr_datacheck "${CMAKE_SOURCE_DIR}/data")

# headless simulation
add_test(NAME sim_cli COMMAND csr_sim --format json "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/8x57is.grtload")
set_tests_properties(sim_cli PROPERTIES PASS_REGULAR_EXPRESSION "muzzleVelocity_ms")
add_test(NAME sim_cli_missing_file COMMAND csr_sim "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/does-not-exist.grtload")
set_tests_properties(sim_cli_missing_file PROPERTIES WILL_FAIL TRUE)

if(TARGET clang_format)
    add_dependencies(tst_core clang_format)
    add_dependencies(tst_gui clang_format)
endif()
