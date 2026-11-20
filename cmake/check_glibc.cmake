# Verify that a shared object does not require GLIBC symbols newer than GLIBC_MAX.
# Usage: cmake -DSO=path/to/foo.so -DGLIBC_MAX=2.31 -P check_glibc.cmake

if(NOT DEFINED SO OR NOT EXISTS "${SO}")
    message(FATAL_ERROR "SO is not set or does not exist: '${SO}'")
endif()
if(NOT DEFINED GLIBC_MAX)
    set(GLIBC_MAX "2.31")
endif()

execute_process(
    COMMAND objdump -T "${SO}"
    OUTPUT_VARIABLE symbols
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "objdump failed for ${SO}")
endif()

string(REGEX MATCHALL "GLIBC_[0-9.]+" versions "${symbols}")
if(NOT versions)
    message(STATUS "No GLIBC versioned symbols found")
    return()
endif()

set(max_version "0")
foreach(entry IN LISTS versions)
    string(REPLACE "GLIBC_" "" value "${entry}")
    if(value VERSION_GREATER max_version)
        set(max_version "${value}")
    endif()
endforeach()

message(STATUS "max GLIBC symbol: ${max_version} (allowed <= ${GLIBC_MAX})")
if(max_version VERSION_GREATER GLIBC_MAX)
    message(FATAL_ERROR "GLIBC requirement too new: ${max_version} > ${GLIBC_MAX}")
endif()
message(STATUS "GLIBC OK")
