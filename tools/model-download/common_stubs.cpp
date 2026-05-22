#include "common.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <filesystem>
#include <system_error>

#if defined(_WIN32)
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

std::string string_format(const char * fmt, ...) {
    va_list ap;
    va_list ap2;
    va_start(ap, fmt);
    va_copy(ap2, ap);
    int size = vsnprintf(NULL, 0, fmt, ap);
    assert(size >= 0 && size < INT_MAX);
    std::vector<char> buf(size + 1);
    int size2 = vsnprintf(buf.data(), size + 1, fmt, ap2);
    assert(size2 == size);
    va_end(ap2);
    va_end(ap);
    return std::string(buf.data(), size);
}

void string_replace_all(std::string & s, const std::string & search, const std::string & replace) {
    if (search.empty()) {
        return;
    }
    std::string builder;
    builder.reserve(s.length());
    size_t pos = 0;
    size_t last_pos = 0;
    while ((pos = s.find(search, last_pos)) != std::string::npos) {
        builder.append(s, last_pos, pos - last_pos);
        builder.append(replace);
        last_pos = pos + search.length();
    }
    builder.append(s, last_pos, std::string::npos);
    s = std::move(builder);
}

std::vector<std::string> string_split(const std::string & str, const std::string & delimiter) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        parts.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    parts.push_back(str.substr(start));

    return parts;
}

std::string common_get_model_endpoint() {
    const char * model_endpoint_env = getenv("MODEL_ENDPOINT");
    const char * hf_endpoint_env = getenv("HF_ENDPOINT");
    const char * endpoint_env = model_endpoint_env ? model_endpoint_env : hf_endpoint_env;
    std::string model_endpoint = "https://huggingface.co/";
    if (endpoint_env) {
        model_endpoint = endpoint_env;
        if (model_endpoint.back() != '/') {
            model_endpoint += '/';
        }
    }
    return model_endpoint;
}

bool tty_can_use_colors() {
    if (const char * no_color = std::getenv("NO_COLOR")) {
        if (no_color[0] != '\0') {
            return false;
        }
    }
    if (const char * term = std::getenv("TERM")) {
        if (std::strcmp(term, "dumb") == 0) {
            return false;
        }
    }
    bool stdout_is_tty = isatty(fileno(stdout));
    bool stderr_is_tty = isatty(fileno(stderr));
    return stdout_is_tty || stderr_is_tty;
}

std::string fs_get_cache_file(const std::string & filename) {
    GGML_ASSERT(filename.find(DIRECTORY_SEPARATOR) == std::string::npos);
    std::string cache_directory = "";
    if (const char * env = std::getenv("LLAMA_CACHE")) {
        cache_directory = env;
    } else {
#if defined(__linux__) || defined(__FreeBSD__) || defined(_AIX) || \
        defined(__OpenBSD__) || defined(__NetBSD__)
        if (std::getenv("XDG_CACHE_HOME")) {
            cache_directory = std::getenv("XDG_CACHE_HOME");
        } else if (std::getenv("HOME")) {
            cache_directory = std::getenv("HOME") + std::string("/.cache/");
        }
#elif defined(__APPLE__)
        if (std::getenv("HOME")) {
            cache_directory = std::getenv("HOME") + std::string("/Library/Caches/");
        }
#elif defined(_WIN32)
        if (std::getenv("LOCALAPPDATA")) {
            cache_directory = std::getenv("LOCALAPPDATA");
        }
#endif
        if (cache_directory.empty()) {
            cache_directory = ".";
        }
        if (cache_directory.back() != DIRECTORY_SEPARATOR) {
            cache_directory += DIRECTORY_SEPARATOR;
        }
        cache_directory += "llama.cpp";
    }

    if (cache_directory.back() != DIRECTORY_SEPARATOR) {
        cache_directory += DIRECTORY_SEPARATOR;
    }

    std::error_code ec;
    std::filesystem::create_directories(cache_directory, ec);
    if (ec) {
        throw std::runtime_error("failed to create cache directory: " + cache_directory + " (" + ec.message() + ")");
    }
    return cache_directory + filename;
}

extern "C" {
    void ggml_abort(const char * file, int line, const char * fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "%s:%d: ggml_abort: ", file, line);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
        va_end(ap);
        std::abort();
    }
}
