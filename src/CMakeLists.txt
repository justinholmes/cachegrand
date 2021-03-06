macro(CACHEGRAND_BUILD_ADHOC_HASHTABLE_SUPPORT_OP_ARCH ARCH)
    set(HASHTABLE_SUPPORT_OP_ARCH_OPT_avx2 -mavx2 -mbmi -mfma -mtune=haswell)
    set(HASHTABLE_SUPPORT_OP_ARCH_OPT_avx -mno-avx256-split-unaligned-load -mno-avx2 -mavx -mbmi)
    set(HASHTABLE_SUPPORT_OP_ARCH_OPT_sse42 -msse4.2)
    set(HASHTABLE_SUPPORT_OP_ARCH_OPT_ss3 -msse3)

    message(STATUS "Enabling accelerated hashtable support operations -- ${ARCH}")

    set(hashtable_support_op_arch_target_name "cachegrand_internal_hashtable_support_op_arch_${ARCH}")

    add_library(
            ${hashtable_support_op_arch_target_name}
            OBJECT
            "${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable_support_op_arch.c")

    target_compile_definitions(
            "${hashtable_support_op_arch_target_name}"
            PRIVATE
            "-DCACHEGRAND_HASHTABLE_SUPPORT_OP_ARCH_SUFFIX=${ARCH}")

    target_compile_options(
            "${hashtable_support_op_arch_target_name}"
            PRIVATE
            ${HASHTABLE_SUPPORT_OP_ARCH_OPT_${ARCH}})

    target_include_directories(
            ${hashtable_support_op_arch_target_name}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR} ${DEPS_LIST_INCLUDE_DIRS})

    list(APPEND SRC_FILES_CACHEGRAND "$<TARGET_OBJECTS:${hashtable_support_op_arch_target_name}>")
endmacro()

# Check the selected hash algorithm
if(USE_HASHTABLE_HASH_ALGORITHM_T1HA2 AND USE_HASHTABLE_HASH_ALGORITHM_CRC32C)
    message(FATAL_ERROR "Can't enable at the same time T1HA2 and CRC32C for the hashtable")
elseif(NOT USE_HASHTABLE_HASH_ALGORITHM_T1HA2 AND NOT USE_HASHTABLE_HASH_ALGORITHM_CRC32C)
    message(FATAL_ERROR "Please select T1HA2 and CRC32C as hashing algorithm for the hashtable")
else()
    message(STATUS "Hashtable hashing algorithm")
    if (USE_HASHTABLE_HASH_ALGORITHM_T1HA2)
        message(STATUS "Hashtable hashing algorithm - t1ha2")
    elseif (USE_HASHTABLE_HASH_ALGORITHM_CRC32C)
        message(STATUS "Hashtable hashing algorithm - crc32c")
    endif()
endif()

# Build the base cachegrand library
file(GLOB_RECURSE SRC_FILES_CACHEGRAND "*.c")
list(APPEND SRC_FILES_CACHEGRAND "${CACHEGRAND_CMAKE_CONFIG_C_SRC}")
list(REMOVE_ITEM SRC_FILES_CACHEGRAND ${CMAKE_CURRENT_SOURCE_DIR}/main.c)

# Remove all the architecture dependant implementation of the hash crc32 algorithm
list(REMOVE_ITEM SRC_FILES_CACHEGRAND "${CMAKE_CURRENT_SOURCE_DIR}/hash/hash_crc32c_sse42.c")

# Remove all the architecture dependant implementation of the search loop and re-inglude it
list(FILTER SRC_FILES_CACHEGRAND EXCLUDE REGEX ".*hashtable_support_hash_search_[a-z0-9]+.c$")
list(APPEND SRC_FILES_CACHEGRAND "${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable_support_hash_search_loop.c")

if(ARCH_IS_X86_64)
    message(STATUS "Enabling accelerated crc32c hash")

    # hash/hash_crc32c_sse42.c
    message(STATUS "Enabling accelerated crc32c hash -- sse4.2")
    list(APPEND SRC_FILES_CACHEGRAND "${CMAKE_CURRENT_SOURCE_DIR}/hash/hash_crc32c_sse42.c")
    set_source_files_properties(
            "hash/hash_crc32c_sse42.c"
            PROPERTIES COMPILE_FLAGS
            "-msse4.2 -mpclmul")

    message(STATUS "Enabling accelerated hash search")

    # hashtable/hashtable_support_hash_search_avx2.c
    message(STATUS "Enabling accelerated hash search -- avx2")
    list(APPEND SRC_FILES_CACHEGRAND "${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable_support_hash_search_avx2.c")
    set_source_files_properties(
            "hashtable/hashtable_support_hash_search_avx2.c"
            PROPERTIES COMPILE_FLAGS
            "-mno-avx256-split-unaligned-load -mavx2 -mbmi -mfma -mtune=haswell")

    # hashtable/hashtable_support_hash_search_avx.c
    message(STATUS "Enabling accelerated hash search -- avx")
    list(APPEND SRC_FILES_CACHEGRAND "${CMAKE_CURRENT_SOURCE_DIR}/hashtable/hashtable_support_hash_search_avx.c")
    set_source_files_properties(
            "hashtable/hashtable_support_hash_search_avx.c"
            PROPERTIES COMPILE_FLAGS
            "-mno-avx256-split-unaligned-load -mno-avx2 -mavx -mbmi")

    message(STATUS "Enabling accelerated hashtable support operations")
    foreach(HASHTABLE_SUPPORT_OP_ARCH_SUFFIX
            avx2
            avx
            sse42
            sse3)
        CACHEGRAND_BUILD_ADHOC_HASHTABLE_SUPPORT_OP_ARCH(
                ${HASHTABLE_SUPPORT_OP_ARCH_SUFFIX})
    endforeach()

elseif(ARCH_IS_AARCH64)
    message(STATUS "No accelerated implementations for AARCH64, using portable")
else()
    message(FATAL_ERROR "Unsupported architecture")
endif()

add_library(
        cachegrand-internal
        ${SRC_FILES_CACHEGRAND})

add_dependencies(
        cachegrand-internal
        __internal_refresh_cmake_config t1ha uring)

target_compile_options(
        cachegrand-internal
        PUBLIC
        -include cmake_config.h)

target_link_directories(
        cachegrand-internal
        PUBLIC
        ${DEPS_LIST_LIBRARY_DIRS})

target_link_libraries(
        cachegrand-internal
        PUBLIC
        ${DEPS_LIST_LIBRARIES})

target_include_directories(
        cachegrand-internal
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}" ${DEPS_LIST_INCLUDE_DIRS})

if(ARCH_IS_X86_64)
    # Force GCC to emit cmpxchg16b inline (supported only on x86_64 platforms)
    target_compile_options(
            cachegrand-internal
            PUBLIC
            -mcx16)
endif()

if(WIN32)
    target_link_libraries(
            cachegrand-internal
            imagehlp)
endif(WIN32)

# Build the executable
add_executable(
        cachegrand-server
        "main.c")

add_dependencies(
        cachegrand-server
        cachegrand-internal t1ha)

target_link_libraries(
        cachegrand-server
        cachegrand-internal t1ha)
