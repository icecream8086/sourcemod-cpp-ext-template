cmake_minimum_required(VERSION 3.16)
project(sourcemod_cpp_ext_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(TEMPLATE_BUILD_EXTENSION "Build the SourceMod extension target (needs SM/HL2 SDK)" ON)
option(TEMPLATE_BUILD_TESTS "Build host-side tests" ON)
option(TEMPLATE_BUILD_BENCH "Build host-side benchmark" ON)
option(TEMPLATE_DISABLE_AVX2 "Compile the AVX2 variant as scalar fallback" OFF)
option(TEMPLATE_ALLOW_UNSAFE_ISA_FLAGS "Allow -march=native/-mavx* in global flags" OFF)
option(TEMPLATE_WERROR "Treat host-side warnings as errors" ON)
set(TEMPLATE_SANITIZE "" CACHE STRING "Host-side sanitizers, e.g. address,undefined")
set(TEMPLATE_ARCH "native" CACHE STRING "Target arch: native | 32 | 64")
set(TEMPLATE_GLIBC_MAX "2.31" CACHE STRING "Maximum allowed GLIBC symbol version")

set(SM_SDK_DIR "" CACHE PATH "SourceMod source tree (contains public/ and sourcepawn/)")
set(HL2SDK_DIR "" CACHE PATH "hl2sdk-l4d2 source tree")
set(MMSOURCE_DIR "" CACHE PATH "metamod-source source tree")

# --------------------------------------------------------------- arch/ABI
set(TEMPLATE_TARGET_FLAGS "")
if(TEMPLATE_ARCH STREQUAL "32")
    set(TEMPLATE_TARGET_FLAGS "-m32")
    add_compile_definitions(TEMPLATE_EXPECT_POINTER_BITS=32)
elseif(TEMPLATE_ARCH STREQUAL "64")
    add_compile_definitions(TEMPLATE_EXPECT_POINTER_BITS=64)
elseif(NOT TEMPLATE_ARCH STREQUAL "native")
    message(FATAL_ERROR "TEMPLATE_ARCH must be native, 32 or 64 (got '${TEMPLATE_ARCH}')")
endif()
if(NOT "${TEMPLATE_TARGET_FLAGS}" STREQUAL "")
    add_compile_options(${TEMPLATE_TARGET_FLAGS})
    add_link_options(${TEMPLATE_TARGET_FLAGS})
endif()

# Baseline ISA: distro compilers may default to x86-64-v3 (AVX2). Force the
# baseline path to stay VEX-free; per-function target attributes still enable
# AVX2 inside the annotated function.
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
    add_compile_options(-mno-avx -mno-avx2 -mno-avx512f)
endif()

# Safety: global ISA flags make every function AVX2-only and crash old CPUs.
if(NOT TEMPLATE_ALLOW_UNSAFE_ISA_FLAGS)
    foreach(_flags_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_RELEASE
                       CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if("${${_flags_var}}" MATCHES "-march=native|-mtune=native|-mavx|-msse4")
            message(FATAL_ERROR
                "Unsafe global ISA flag in ${_flags_var}: '${${_flags_var}}'.\n"
                "Use per-function target attributes (docs/ISA.md); set "
                "TEMPLATE_ALLOW_UNSAFE_ISA_FLAGS=ON only if you really mean it.")
        endif()
    endforeach()
endif()

if(NOT "${TEMPLATE_SANITIZE}" STREQUAL "")
    message(STATUS "Host sanitizers enabled: ${TEMPLATE_SANITIZE}")
    add_compile_options(-fsanitize=${TEMPLATE_SANITIZE} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${TEMPLATE_SANITIZE})
endif()

function(template_warnings target)
    if(TEMPLATE_WERROR)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()

# ---------------------------------------------------------------- host code
add_library(template_base STATIC
    src/dispatch.cpp
    src/base/distance_scalar.cpp
    src/isa/avx2/distance_avx2.cpp)
target_include_directories(template_base PUBLIC include)
template_warnings(template_base)
if(TEMPLATE_DISABLE_AVX2)
    target_compile_definitions(template_base PRIVATE TEMPLATE_DISABLE_AVX2=1)
endif()

if(TEMPLATE_BUILD_TESTS)
    enable_testing()
    add_executable(template_tests tests/test_distance.cpp)
    target_link_libraries(template_tests PRIVATE template_base)
    template_warnings(template_tests)
    add_test(NAME distance COMMAND template_tests)

    add_executable(smoke_dispatch tests/smoke_dispatch.cpp)
    target_link_libraries(smoke_dispatch PRIVATE template_base)
    template_warnings(smoke_dispatch)
    add_test(NAME smoke_dispatch COMMAND smoke_dispatch)
    set_tests_properties(smoke_dispatch PROPERTIES LABELS "smoke;isa")

    add_executable(smoke_edge tests/smoke_edge.cpp)
    target_link_libraries(smoke_edge PRIVATE template_base)
    template_warnings(smoke_edge)
    add_test(NAME smoke_edge COMMAND smoke_edge)
    set_tests_properties(smoke_edge PROPERTIES LABELS "smoke;isa")

    add_custom_target(check_isa
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/audit_isa.sh
                $<TARGET_FILE:template_tests>
        DEPENDS template_tests
        COMMENT "Auditing baseline functions for ISA instructions")
endif()

if(TEMPLATE_BUILD_BENCH)
    add_executable(template_bench bench/bench_distance.cpp)
    target_include_directories(template_bench PRIVATE third_party/nanobench)
    target_link_libraries(template_bench PRIVATE template_base)
endif()

# --------------------------------------------------------- formal verification
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_FOUND)
    add_custom_target(verify
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify/verify_dispatch.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify/verify_completeness.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify/verify_distance.py
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/verify/verify_sort.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running Z3 formal checks (requires z3-solver)")
endif()

# ------------------------------------------------------------- SM extension
if(TEMPLATE_BUILD_EXTENSION)
    find_path(SM_SDK_HEADER smsdk_ext.h
        PATHS "${SM_SDK_DIR}/public" "${SM_SDK_DIR}/public/sourcepawn"
        NO_DEFAULT_PATH)
    find_path(HL2SDK_HEADER tier1.h
        PATHS "${HL2SDK_DIR}/public/tier1" "${HL2SDK_DIR}/public"
        NO_DEFAULT_PATH)
    find_path(MMSOURCE_HEADER ISmmPlugin.h
        PATHS "${MMSOURCE_DIR}/core" "${MMSOURCE_DIR}/core/sourcehook"
        NO_DEFAULT_PATH)

    if(SM_SDK_HEADER AND HL2SDK_HEADER AND MMSOURCE_HEADER)
        add_library(template_ext SHARED
            src/extension.cpp
            src/dispatch.cpp
            src/base/distance_scalar.cpp
            src/isa/avx2/distance_avx2.cpp)
        target_include_directories(template_ext PRIVATE
            include
            "${SM_SDK_DIR}/public"
            "${SM_SDK_DIR}/public/sourcepawn"
            "${SM_SDK_DIR}/sourcepawn/include"
            "${MMSOURCE_DIR}/core"
            "${MMSOURCE_DIR}/core/sourcehook"
            "${HL2SDK_DIR}/public"
            "${HL2SDK_DIR}/public/engine"
            "${HL2SDK_DIR}/public/tier1"
            "${HL2SDK_DIR}/public/game/server")
        target_compile_options(template_ext PRIVATE
            ${TEMPLATE_TARGET_FLAGS} -fPIC -fvisibility=hidden
            -static-libgcc -static-libstdc++)
        target_link_options(template_ext PRIVATE
            ${TEMPLATE_TARGET_FLAGS} -shared -static-libgcc -static-libstdc++
            -Wl,--strip-debug)
        target_link_directories(template_ext PRIVATE "${HL2SDK_DIR}/lib/linux")
        target_link_libraries(template_ext PRIVATE tier1 dl m pthread)
        set_target_properties(template_ext PROPERTIES
            PREFIX "" OUTPUT_NAME "template.ext.2.l4d2")

        add_custom_target(check_glibc
            COMMAND ${CMAKE_COMMAND}
                -DSO=$<TARGET_FILE:template_ext>
                -DGLIBC_MAX=${TEMPLATE_GLIBC_MAX}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_glibc.cmake
            DEPENDS template_ext
            COMMENT "Checking GLIBC symbol ceiling")
    else()
        message(STATUS "SM/HL2/MM SDK not found - extension target skipped "
                       "(set SM_SDK_DIR/HL2SDK_DIR/MMSOURCE_DIR)")
    endif()
endif()
