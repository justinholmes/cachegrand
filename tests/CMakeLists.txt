# Setup catch2, it's the testing framework
include(ExternalProject)
find_package(Wget REQUIRED)

ExternalProject_Add(
        catch2-external-project
        PREFIX ${CMAKE_BINARY_DIR}/catch2
        DOWNLOAD_DIR catch2
        DOWNLOAD_COMMAND ${WGET_EXECUTABLE} https://github.com/catchorg/Catch2/releases/download/v2.1.2/catch.hpp
        TIMEOUT 10
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(catch2-external-project download_dir)

add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE ${download_dir}/..)
add_library(catch2::catch ALIAS catch2)

# Build the tests
file(GLOB SRC_CACHEGRAND_TESTS "*.cpp")

add_executable(tests ${SRC_CACHEGRAND_TESTS})
add_dependencies(tests cachegrand-internal catch2-external-project catch2)
set_target_properties(tests PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(tests catch2)
target_link_libraries(tests cachegrand-internal t1ha2)
target_include_directories(tests PUBLIC ".")
target_include_directories(tests PUBLIC "../src")
target_include_directories(tests PUBLIC "${download_dir}")

add_test(NAME tests COMMAND tests)