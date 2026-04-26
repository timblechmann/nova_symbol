cmake_minimum_required(VERSION 3.25)

# Set policies
cmake_policy(SET CMP0167 NEW) # The FindBoost module is removed

project(nova_symbol VERSION 0.1.0 LANGUAGES CXX)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

if(NOT CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
endif()

########################################################################################################################

add_custom_target(nova_symbol_project_files SOURCES
    .clang-tidy
    .gitignore
    .pre-commit-config.yaml
    _clang-format
    Readme.md
    LICENSE
    .github/workflows/ci.yml
)

########################################################################################################################

include(CheckCXXSourceCompiles)
check_cxx_source_compiles([[
    #include <version>
    int main() {
    static_assert(__cpp_lib_memory_resource);
    }
    ]] NOVA_PMR_COMPILES)

########################################################################################################################



function(nova_symbol_install_cpm)
    if (NOT COMMAND CPMAddPackage)
        set(CPM_DOWNLOAD_VERSION 0.42.1)
        if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
            message(STATUS "Downloading CPM.cmake ${CPM_DOWNLOAD_VERSION}...")
            file(DOWNLOAD
                "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
                "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
            )
        endif()
        include("${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    endif()
endfunction()

set(NeedBoostContainer OFF)
if(APPLE AND NOT NOVA_PMR_COMPILES)
    set(NeedBoostContainer ON)
endif()

set(HasBoost TRUE)
if(NOT TARGET Boost::headers)
    set(HasBoost FALSE)
endif()
if(NOT TARGET Boost::intrusive)
    set(HasBoost FALSE)
endif()
if(NeedBoostContainer)
    if(NOT TARGET Boost::container)
        set(HasBoost FALSE)
    endif()
endif()

if(NOT Boost::headers)
    nova_symbol_install_cpm()
    if(NeedBoostContainer)
        set(_boost_include_option "BOOST_INCLUDE_LIBRARIES intrusive\\;container")
    else()
        set(_boost_include_option "BOOST_INCLUDE_LIBRARIES intrusive")
    endif()

    CPMAddPackage(
        NAME Boost
        VERSION 1.90.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
        URL_HASH SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
        OPTIONS "BOOST_SKIP_INSTALL_RULES ON"
                "BUILD_SHARED_LIBS OFF"
                ${_boost_include_option}
    )
endif()

if(NOT TARGET rapidhash::rapidhash)
    nova_symbol_install_cpm()
    CPMAddPackage(
        NAME Rapidhash
        GITHUB_REPOSITORY Nicoshev/rapidhash
        GIT_TAG rapidhash_v3
        DOWNLOAD_ONLY TRUE
    )

    add_library(rapidhash INTERFACE)
    target_include_directories(rapidhash INTERFACE
        ${Rapidhash_SOURCE_DIR}
    )
    add_library(rapidhash::rapidhash ALIAS rapidhash)
endif()

########################################################################################################################

set(Sources
    inc/nova/symbol/symbol.hpp
    src/nova/symbol/symbol.cpp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})

add_library(nova_symbol STATIC ${Sources})

target_include_directories(nova_symbol PUBLIC inc)
target_link_libraries(nova_symbol
    PRIVATE rapidhash::rapidhash
)

if (TARGET Boost::headers)
    target_link_libraries(nova_symbol
        PRIVATE Boost::headers)
endif()
if(TARGET Boost::intrusive)
    target_link_libraries(nova_symbol
        PRIVATE Boost::intrusive)
endif()

if(NOT NOVA_PMR_COMPILES)
    target_link_libraries(nova_symbol
        PRIVATE Boost::container
    )
endif()

########################################################################################################################

option(NOVA_SYMBOL_BUILD_TEST "Build unit tests" ON)

if (NOVA_SYMBOL_BUILD_TEST)
    enable_testing()

    block()
        nova_symbol_install_cpm()
        set(CMAKE_UNITY_BUILD ON)
        if(NOT (TARGET Catch2::Catch2WithMain OR TARGET Catch2::Catch2))
            CPMAddPackage("gh:catchorg/Catch2#v3.14.0")
        endif()
        if(NOT TARGET fmt::fmt)
            CPMAddPackage(
                NAME fmt
                GIT_REPOSITORY https://github.com/fmtlib/fmt.git
                GIT_TAG 12.1.0
            )
        endif()
    endblock()

    add_executable(nova_symbol_test test/symbol_test.cpp)

    add_test(NAME nova_symbol_test COMMAND nova_symbol_test)

    target_link_libraries(nova_symbol_test
        PUBLIC Catch2::Catch2WithMain nova_symbol fmt::fmt)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES test/symbol_test.cpp)
endif()
