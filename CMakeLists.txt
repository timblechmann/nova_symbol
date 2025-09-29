cmake_minimum_required(VERSION 3.25)

# Set policies
cmake_policy(SET CMP0167 NEW) # The FindBoost module is removed

project(nova_symbol VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

########################################################################################################################

add_custom_target(nova_symbol_project_files SOURCES
    .clang-tidy
    .pre-commit-config.yaml
    _clang-format
    conanfile.txt
    Readme.md
    License.txt
    .github/workflows/ci.yml
    cmake/conan_provider.cmake
)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS conanfile.txt)

########################################################################################################################

find_package(Boost)
find_package(Cityhash)
find_package(Catch2 CONFIG)

########################################################################################################################

set(Sources
    inc/nova/symbol/symbol.hpp
    src/nova/symbol/symbol.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})

add_library(nova_symbol STATIC ${Sources})

target_include_directories(nova_symbol PUBLIC inc)
target_link_libraries(nova_symbol
    PUBLIC  Boost::headers
    PRIVATE cityhash::cityhash
)

if (APPLE)
    target_link_libraries(nova_symbol
        PRIVATE Boost::container # pmr fallback
    )
endif()


########################################################################################################################

option(NOVA_SYMBOL_BUILD_TEST "Build unit tests" ON)

if (NOVA_SYMBOL_BUILD_TEST)
    enable_testing()

    add_executable(nova_symbol_test test/symbol_test.cpp)

    add_test(NAME nova_symbol_test COMMAND nova_symbol_test)

    target_link_libraries(nova_symbol_test
        PUBLIC Catch2::Catch2WithMain nova_symbol)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES test/symbol_test.cpp)
endif()
