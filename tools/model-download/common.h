#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <string_view>

#ifdef _WIN32
#define DIRECTORY_SEPARATOR '\\'
#else
#define DIRECTORY_SEPARATOR '/'
#endif

#ifndef GGML_ASSERT
#define GGML_ASSERT(cond) assert(cond)
#endif

bool tty_can_use_colors();
std::string fs_get_cache_file(const std::string & filename);

// Minimal struct matching common_params_model from common/common.h
struct common_params_model {
    std::string path        = "";
    std::string url         = "";
    std::string hf_repo     = "";
    std::string hf_file     = "";
    std::string ms_repo     = "";
    std::string docker_repo = "";
    std::string name        = "";
};

// Declaring functions implemented in common_stubs.cpp
std::string string_format(const char * fmt, ...);
void string_replace_all(std::string & s, const std::string & search, const std::string & replace);
std::vector<std::string> string_split(const std::string & str, const std::string & delimiter);
std::string common_get_model_endpoint();

// Inline string helpers matching common/common.h
inline bool string_starts_with(std::string_view str, std::string_view prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

inline bool string_starts_with(std::string_view str, char prefix) {
    return !str.empty() && str.front() == prefix;
}

inline bool string_ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline bool string_remove_suffix(std::string & str, std::string_view suffix) {
    if (string_ends_with(str, suffix)) {
        str.resize(str.size() - suffix.size());
        return true;
    }
    return false;
}

template<typename T>
static std::vector<T> string_split(const std::string & str, char delim) {
    std::vector<T> values;
    std::string val;
    std::istringstream iss(str);
    while (std::getline(iss, val, delim)) {
        values.push_back(val);
    }
    return values;
}

template<>
inline std::vector<std::string> string_split<std::string>(const std::string & str, char delim) {
    std::vector<std::string> values;
    std::string val;
    std::istringstream iss(str);
    while (std::getline(iss, val, delim)) {
        values.push_back(val);
    }
    return values;
}
