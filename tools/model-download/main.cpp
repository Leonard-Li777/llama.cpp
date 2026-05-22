#include "common.h"
#include "download.h"
#include "ms.h"
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <cstdlib>
#include <filesystem>

#if defined(_WIN32)
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

// Set environment variable portably
static void set_env(const std::string & name, const std::string & value) {
#if defined(_WIN32)
    _putenv_s(name.c_str(), value.c_str());
#else
    setenv(name.c_str(), value.c_str(), 1);
#endif
}

static bool is_output_a_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout));
#else
    return isatty(1);
#endif
}

class JsonProgressBar : public common_download_callback {
public:
    std::string filename;
    bool json_mode;
    mutable std::mutex mutex;
    bool started = false;
    bool finished = false;

    JsonProgressBar(bool json_mode) : json_mode(json_mode) {}

    void on_start(const common_download_progress & p) override {
        std::lock_guard<std::mutex> lock(mutex);
        filename = p.url;
        if (auto pos = filename.rfind('/'); pos != std::string::npos) {
            filename = filename.substr(pos + 1);
        }
        if (auto pos = filename.find('?'); pos != std::string::npos) {
            filename = filename.substr(0, pos);
        }

        if (json_mode) {
            std::cout << "{\"status\":\"start\",\"file\":\"" << filename 
                      << "\",\"url\":\"" << p.url << "\"}\n" << std::flush;
        } else {
            std::cout << "Downloading " << filename << " ...\n" << std::flush;
        }
        started = true;
    }

    void on_update(const common_download_progress & p) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!started) {
            filename = p.url;
            if (auto pos = filename.rfind('/'); pos != std::string::npos) {
                filename = filename.substr(pos + 1);
            }
            if (auto pos = filename.find('?'); pos != std::string::npos) {
                filename = filename.substr(0, pos);
            }
            started = true;
        }

        double percent = 0.0;
        if (p.total > 0) {
            percent = (100.0 * p.downloaded) / p.total;
        }

        if (json_mode) {
            std::cout << "{\"status\":\"progress\",\"file\":\"" << filename 
                      << "\",\"downloaded\":" << p.downloaded 
                      << ",\"total\":" << p.total 
                      << ",\"percent\":" << percent << "}\n" << std::flush;
        } else {
            if (p.total > 0) {
                std::cout << "\rDownloading " << filename << ": " 
                          << p.downloaded << "/" << p.total 
                          << " (" << percent << "%)" << std::flush;
            } else {
                std::cout << "\rDownloading " << filename << ": " 
                          << p.downloaded << " bytes" << std::flush;
            }
        }
    }

    void on_done(const common_download_progress & p, bool ok) override {
        std::lock_guard<std::mutex> lock(mutex);
        finished = true;
        if (json_mode) {
            std::cout << "{\"status\":\"done\",\"file\":\"" << filename 
                      << "\",\"ok\":" << (ok ? "true" : "false") << "}\n" << std::flush;
        } else {
            if (ok) {
                std::cout << "\nFinished downloading " << filename << ".\n" << std::flush;
            } else {
                std::cout << "\nFailed downloading " << filename << ".\n" << std::flush;
            }
        }
    }
};

void print_help(const char * prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n\n"
              << "Options:\n"
              << "  -hf, --hf-repo REPO     Hugging Face model repository (e.g. user/model[:quant])\n"
              << "  -hff, --hf-file FILE    Hugging Face model file name (optional)\n"
              << "  -ms, --ms-repo REPO     ModelScope model repository (e.g. user/model[:quant])\n"
              << "  -msf, --ms-file FILE    ModelScope model file name (optional)\n"
              << "  -t, --token TOKEN       Hugging Face / ModelScope access token\n"
              << "  --json                  Output download progress in JSON format (single lines)\n"
              << "  --cache-dir DIR         Custom cache directory (sets LLAMA_CACHE)\n"
              << "  -h, --help              Show this help message\n";
}

int main(int argc, char ** argv) {
    std::string hf_repo;
    std::string hf_file;
    std::string ms_repo;
    std::string ms_file;
    std::string token;
    std::string cache_dir;
    bool force_json = false;

    // Parse options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "-hf" || arg == "--hf-repo") {
            if (i + 1 < argc) {
                hf_repo = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-hff" || arg == "--hf-file") {
            if (i + 1 < argc) {
                hf_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-ms" || arg == "--ms-repo") {
            if (i + 1 < argc) {
                ms_repo = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-msf" || arg == "--ms-file") {
            if (i + 1 < argc) {
                ms_file = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "-t" || arg == "--token") {
            if (i + 1 < argc) {
                token = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else if (arg == "--json") {
            force_json = true;
        } else if (arg == "--cache-dir") {
            if (i + 1 < argc) {
                cache_dir = argv[++i];
            } else {
                std::cerr << "Error: " << arg << " requires an argument\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown argument '" << arg << "'\n";
            print_help(argv[0]);
            return 1;
        }
    }

    if (hf_repo.empty() && ms_repo.empty()) {
        std::cerr << "Error: Either -hf or -ms option must be specified.\n\n";
        print_help(argv[0]);
        return 1;
    }

    if (!hf_repo.empty() && !ms_repo.empty()) {
        std::cerr << "Error: Cannot specify both -hf and -ms options.\n";
        return 1;
    }

    // Set cache directory if provided
    if (!cache_dir.empty()) {
        set_env("LLAMA_CACHE", cache_dir);
    }

    bool json_mode = force_json || !is_output_a_tty();
    JsonProgressBar callback(json_mode);

    try {
        if (!hf_repo.empty()) {
            common_params_model model;
            model.hf_repo = hf_repo;
            model.hf_file = hf_file;

            common_download_opts opts;
            opts.bearer_token = token;
            opts.callback = &callback;

            auto download_result = common_download_model(model, opts, true, false);

            if (download_result.model_path.empty()) {
                if (json_mode) {
                    std::cout << "{\"status\":\"failed\",\"error\":\"failed to download model from Hugging Face\"}\n" << std::flush;
                } else {
                    std::cerr << "Error: failed to download model from Hugging Face\n";
                }
                return 1;
            }

            if (!callback.finished) {
                callback.on_done(common_download_progress{download_result.model_path, 0, 0, true}, true);
            }
        } else if (!ms_repo.empty()) {
            auto [ms_repo_clean, ms_tag] = common_download_split_repo_tag(ms_repo);

            auto download_result = ms::download_model(ms_repo_clean, ms_file, false, ms_tag, token, &callback);

            if (download_result.model_path.empty()) {
                if (json_mode) {
                    std::cout << "{\"status\":\"failed\",\"error\":\"failed to download model from ModelScope\"}\n" << std::flush;
                } else {
                    std::cerr << "Error: failed to download model from ModelScope\n";
                }
                return 1;
            }

            if (!callback.finished) {
                callback.on_done(common_download_progress{download_result.model_path, 0, 0, true}, true);
            }
        }
    } catch (const std::exception & e) {
        if (json_mode) {
            std::cout << "{\"status\":\"failed\",\"error\":\"" << e.what() << "\"}\n" << std::flush;
        } else {
            std::cerr << "Error: " << e.what() << "\n";
        }
        return 1;
    }

    return 0;
}
