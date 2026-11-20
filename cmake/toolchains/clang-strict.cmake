# L4D2 strict profile.
#
# Usage:
#   cmake -S . -B build-strict \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/clang-strict.cmake \
#     -DTEMPLATE_BUILD_EXTENSION=OFF
#
# Requires a `clang++-strict` wrapper (build machines may preinstall clang++-strict,
# or use scripts/clang-strict.sh installed as clang++-strict).

find_program(TEMPLATE_STRICT_CXX NAMES clang++-strict)
if(NOT TEMPLATE_STRICT_CXX)
    message(FATAL_ERROR
        "clang++-strict not found.\n"
        "Install the wrapper, e.g. scripts/clang-strict.sh as clang++-strict in PATH.")
endif()

set(CMAKE_CXX_COMPILER "${TEMPLATE_STRICT_CXX}" CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Make the annotation header available even outside the  image.
include_directories(BEFORE "${CMAKE_CURRENT_LIST_DIR}/../../include")

