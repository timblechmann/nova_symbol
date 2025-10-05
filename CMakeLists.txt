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

option(NOVA_SYMBOL_USE_CPM "Use CPM.cmake for dependency management" ON)

if (NOVA_SYMBOL_USE_CPM)
    if (NOT CPM_SOURCE_CACHE)
        set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_BINARY_DIR}/.cpm_cache")
    endif()

    file(
        DOWNLOAD
        https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
        EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
    )
    include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

    if (false)
        CPMAddPackage(
          NAME Boost
          VERSION 1.89.0
          URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.tar.xz
          URL_HASH SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
          OPTIONS "BOOST_SKIP_INSTALL_RULES ON"
                  "BUILD_SHARED_LIBS OFF"
                  "BOOST_INCLUDE_LIBRARIES intrusive\\\;container"
        )
    else()
        include(FetchContent)
        FetchContent_Declare(
          Boost
          URL https://github.com/boostorg/boost/releases/download/boost-1.89.0/boost-1.89.0-cmake.tar.xz
          URL_HASH SHA256=67acec02d0d118b5de9eb441f5fb707b3a1cdd884be00ca24b9a73c995511f74
          DOWNLOAD_EXTRACT_TIMESTAMP ON
        )

        set(BOOST_SKIP_INSTALL_RULES ON)
        set(BOOST_INCLUDE_LIBRARIES intrusive container)
        FetchContent_MakeAvailable(Boost)
    endif()
    CPMAddPackage("gh:catchorg/Catch2#v3.10.0")

    CPMAddPackage(
        NAME Cityhash
        GITHUB_REPOSITORY google/cityhash
        # There are no version tags, so download the latest commit from the master branch.
        GIT_TAG master
        DOWNLOAD_ONLY TRUE
    )

    add_library(cityhash STATIC
        ${Cityhash_SOURCE_DIR}/src/city.cc
    )

    # Create a dummy config.h for Cityhash
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash/config.h "#ifndef CITYHASH_CONFIG_H\n#define CITYHASH_CONFIG_H\n\n// Assume modern system with stdint.h\n#define HAVE_STDINT_H 1\n\n// Assume GCC/Clang built-ins for modern compilers\n#define HAVE_BUILTIN_EXPECT 1\n#define HAVE_BUILTIN_CTZ 1\n#define HAVE_BUILTIN_CLZ 1\n\n// Assume these headers are available on modern Unix-like systems\n#define HAVE_BYTESWAP_H 1\n#define HAVE_SYS_ENDIAN_H 1\n\n// Define package information (optional, but good practice)\n#define PACKAGE_NAME \"cityhash\"\n#define PACKAGE_VERSION \"1.0.1\"\n\n#endif // CITYHASH_CONFIG_H\n")

    target_include_directories(cityhash PUBLIC
        ${Cityhash_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/cpm_patches/cityhash
    )
    add_library(cityhash::cityhash ALIAS cityhash)

else()
    find_package(Boost)
    find_package(Cityhash)
    find_package(Catch2 CONFIG)
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

    add_executable(nova_symbol_test test/symbol_test.cpp)

    add_test(NAME nova_symbol_test COMMAND nova_symbol_test)

    target_link_libraries(nova_symbol_test
        PUBLIC Catch2::Catch2WithMain nova_symbol)

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES test/symbol_test.cpp)
endif()
