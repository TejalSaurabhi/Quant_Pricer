cmake_minimum_required(VERSION 3.20)
project(QuantPricer VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -Wall")

# Add Conan support if available
if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

# Find packages
find_package(Catch2 QUIET)
find_package(Eigen3 QUIET)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Core library with implementations
add_library(quant_core STATIC
    core/DayCount.cpp
    core/DiscountCurve.cpp
    core/CashFlow.cpp
    engines/YieldSolver.cpp
    engines/Sensitivity.cpp
    engines/Black76.cpp
    engines/MonteCarlo.cpp
    instruments/Bond.cpp
    instruments/EuropeanBondOption.cpp
)
target_include_directories(quant_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(quant_core PUBLIC cxx_std_20)

# Link Eigen if available
if(Eigen3_FOUND)
    target_link_libraries(quant_core PUBLIC Eigen3::Eigen)
    message(STATUS "Eigen3 found: Vectorized Monte Carlo enabled")
else()
    message(WARNING "Eigen3 not found. Monte Carlo will use scalar implementation.")
    # Add fallback header-only Eigen if available
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen")
        target_include_directories(quant_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/eigen")
    endif()
endif()

# Demo executables
add_executable(option_demo option_demo.cpp)
target_link_libraries(option_demo PRIVATE quant_core)
target_compile_features(option_demo PRIVATE cxx_std_20)

add_executable(mc_demo mc_demo.cpp)
target_link_libraries(mc_demo PRIVATE quant_core)
target_compile_features(mc_demo PRIVATE cxx_std_20)

# Create test executables only if Catch2 is found
if(Catch2_FOUND)
    # Core functionality tests
    add_executable(simple_test tests/simple_test.cpp)
    target_link_libraries(simple_test PRIVATE quant_core Catch2::Catch2WithMain)
    target_compile_features(simple_test PRIVATE cxx_std_20)
    
    # Bond instrument tests
    add_executable(bond_test tests/bond_test.cpp)
    target_link_libraries(bond_test PRIVATE quant_core Catch2::Catch2WithMain)
    target_compile_features(bond_test PRIVATE cxx_std_20)
    
    # Bond instrument tests (new)
    add_executable(bond_test_new tests/bond_test_new.cpp)
    target_link_libraries(bond_test_new PRIVATE quant_core Catch2::Catch2WithMain)
    target_compile_features(bond_test_new PRIVATE cxx_std_20)
    
    # Option tests
    add_executable(option_test tests/option_test.cpp)
    target_link_libraries(option_test PRIVATE quant_core Catch2::Catch2WithMain)
    target_compile_features(option_test PRIVATE cxx_std_20)
    
    # Enable CTest
    enable_testing()
    add_test(NAME CoreTests COMMAND simple_test)
    add_test(NAME BondTests COMMAND bond_test)
    add_test(NAME BondNewTests COMMAND bond_test_new)
    add_test(NAME OptionTests COMMAND option_test)
    
    message(STATUS "Tests enabled. Run 'make test' or 'ctest' to execute.")
else()
    message(WARNING "Catch2 not found. Tests will not be built.")
endif()

# Installation rules
install(TARGETS quant_core
        EXPORT QuantPricerTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

install(DIRECTORY core/ DESTINATION include/quant/core)
install(DIRECTORY instruments/ DESTINATION include/quant/instruments)
install(DIRECTORY engines/ DESTINATION include/quant/engines)

# Export package
install(EXPORT QuantPricerTargets
        FILE QuantPricerTargets.cmake
        NAMESPACE quant::
        DESTINATION lib/cmake/QuantPricer)

# Create package config file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    QuantPricerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES 
        "${CMAKE_CURRENT_BINARY_DIR}/QuantPricerConfigVersion.cmake"
        DESTINATION lib/cmake/QuantPricer)

# Print configuration summary
message(STATUS "=== QuantPricer Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Tests: ${Catch2_FOUND}")
message(STATUS "=================================")

# Custom targets for development
add_custom_target(format
    COMMAND clang-format -i 
        core/*.hpp 
        instruments/*.hpp 
        engines/*.hpp 
        tests/*.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Formatting source files")

add_custom_target(docs
    COMMAND doxygen Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation")

# Add valgrind target for memory leak detection
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE)
        add_custom_target(valgrind
            COMMAND ${VALGRIND_EXECUTABLE} --tool=memcheck --leak-check=full --show-leak-kinds=all 
                    ./simple_test
            DEPENDS simple_test
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running Valgrind memory checks")
    endif()
endif()
