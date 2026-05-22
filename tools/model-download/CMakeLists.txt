set(TARGET llama-model-download)

# Copy hf-cache.cpp to the binary directory so it compiles using local stub headers
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/hf-cache.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/hf-cache.cpp
    COPYONLY
)

# Copy stub headers to CMAKE_CURRENT_BINARY_DIR so that hf-cache.cpp finds them first
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/common.h
    ${CMAKE_CURRENT_BINARY_DIR}/common.h
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/arg.h
    ${CMAKE_CURRENT_BINARY_DIR}/arg.h
    COPYONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/log.h
    ${CMAKE_CURRENT_BINARY_DIR}/log.h
    COPYONLY
)

add_executable(${TARGET}
    main.cpp
    common_stubs.cpp
    download.cpp
    ms.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/hf-cache.cpp
)

# Place binary directory and source directory first in include path
target_include_directories(${TARGET} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../common
    ../../vendor
    ../../ggml/include
)

target_link_libraries(${TARGET} PRIVATE
    cpp-httplib
    llama-common-base
    Threads::Threads
)

target_compile_features(${TARGET} PRIVATE cxx_std_17)

if (WIN32)
    target_link_libraries(${TARGET} PRIVATE ws2_32 crypt32)
endif()

if(LLAMA_TOOLS_INSTALL)
    install(TARGETS ${TARGET} RUNTIME)
endif()
