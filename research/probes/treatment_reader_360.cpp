#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" char* __real_getenv(const char* name);

extern "C" char* __wrap_getenv(const char* name) {
    char* value = __real_getenv(name);
    if (name != nullptr && std::strcmp(name, "UDON_RESOURCE_MARGINAL_357") == 0) {
        const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        // A separate probe file preserves the existing zero-stderr contract.
        // No environment values or credentials are written, including the token.
        static std::FILE* trace = []() {
            const char* path = __real_getenv("UDON_360_TRACE");
            std::FILE* file = path == nullptr ? nullptr : std::fopen(path, "ax");
            if (file == nullptr) {
                std::fputs("contract360 cannot create exclusive treatment trace\n", stderr);
                std::abort();
            }
            return file;
        }();
        std::fprintf(trace, "%lld\n", static_cast<long long>(stamp));
        std::fflush(trace);
    }
    return value;
}
