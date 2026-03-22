cmake_minimum_required(VERSION 3.25)

# Set policies
cmake_policy(SET CMP0167 NEW) # The FindBoost module is removed

project(nova_symbol VERSION 0.1.0 LANGUAGES CXX)

if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

########################################################################################################################

add_custom_target(nova_symbol_project_files SOURCES
    .clang-tidy
    .gitignore
    .pre-commit-config.yaml
    _clang-format
    Readme.md
    License.txt
    .github/workflows/ci.yml
)

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

if(NOT (TARGET Boost::headers AND TARGET Boost::intrusive AND (NOT APPLE OR TARGET Boost::container)))
    nova_symbol_install_cpm()
    CPMAddPackage(
        NAME Boost
        VERSION 1.90.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
        URL_HASH SHA256=aca59f889f0f32028ad88ba6764582b63c916ce5f77b31289ad19421a96c555f
        OPTIONS "BOOST_SKIP_INSTALL_RULES ON"
                "BUILD_SHARED_LIBS OFF"
                "BOOST_INCLUDE_LIBRARIES intrusive\\;container"
    )
endif()

if(NOT (TARGET cityhash::cityhash OR TARGET Cityhash))
    nova_symbol_install_cpm()
    CPMAddPackage(
        NAME Cityhash
        GITHUB_REPOSITORY google/cityhash
        GIT_TAG master
        DOWNLOAD_ONLY TRUE
    )

    add_library(cityhash STATIC ${Cityhash_SOURCE_DIR}/src/city.cc)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash/config.h "#ifndef CITYHASH_CONFIG_H\n#define CITYHASH_CONFIG_H\n\n// Assume modern system with stdint.h\n#define HAVE_STDINT_H 1\n#define HAVE_BUILTIN_EXPECT 1\n#define HAVE_BUILTIN_CTZ 1\n#define HAVE_BUILTIN_CLZ 1\n#define HAVE_BYTESWAP_H 1\n#define HAVE_SYS_ENDIAN_H 1\n#define PACKAGE_NAME \"cityhash\"\n#define PACKAGE_VERSION \"1.0.1\"\n\n#endif // CITYHASH_CONFIG_H\n")

    target_include_directories(cityhash PUBLIC
        ${Cityhash_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash
    )
    add_library(cityhash::cityhash ALIAS cityhash)
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
    PRIVATE cityhash::cityhash
)

if (TARGET Boost::headers)
    target_link_libraries(nova_symbol
        PRIVATE Boost::headers)
endif()
if(TARGET Boost::intrusive)
    target_link_libraries(nova_symbol
        PRIVATE Boost::intrusive)
endif()

if (APPLE)
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
            CPMAddPackage("gh:catchorg/Catch2#v3.11.0")
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
