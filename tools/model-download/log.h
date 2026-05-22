#pragma once
#include <cstdio>
#include <cstdlib>

#define LOG_INF(...) printf(__VA_ARGS__)
#define LOG_WRN(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERR(...) fprintf(stderr, __VA_ARGS__)

#define LOG_DBG(...) do { \
    if (std::getenv("LLAMA_DOWNLOAD_DEBUG")) { \
        printf(__VA_ARGS__); \
    } \
} while(0)

#define LOG_TRC(...) do { \
    if (std::getenv("LLAMA_DOWNLOAD_TRACE")) { \
        printf(__VA_ARGS__); \
    } \
} while(0)
