// Research-only byte transport. Original production files are never replaced.
#ifndef _WIN32
int _dupenv_s(char** out, std::size_t* size, const char* name) {
    *out = nullptr; *size = 0;
    const char* value = std::getenv(name);
    if (!value) return 0;
    *size = std::strlen(value) + 1;
    *out = static_cast<char*>(std::malloc(*size));
    if (!*out) return 12;
    std::memcpy(*out, value, *size);
    return 0;
}
#endif

class WinHttpRequestError final : public std::runtime_error {
public:
    explicit WinHttpRequestError(bool retryable)
        : std::runtime_error("synthetic RPC transport failure"), retryable_(retryable) {}
    bool retryable_resend() const noexcept { return retryable_; }
private:
    bool retryable_;
};

class WinHttpClient {
public:
    WinHttpClient(const std::string& url, const std::string& token) {
        if (url != "http://127.0.0.1:352" ||
            token != "synthetic-loopback-only-no-credential")
            throw std::runtime_error("352 admits only synthetic process RPC, never credentials");
    }
    HttpResponse request(const std::string& method, const std::string& path,
        const std::optional<std::string>& body = std::nullopt,
        std::int32_t ioTimeoutMs = 0) const {
        using J = udon::JsonValue;
        const auto id = ++sequence_;
        J q(J::Object{{"transportRPC",J(std::int64_t{352})}, {"id",J(id)},
            {"method",J(method)}, {"path",J(path)}, {"hasBody",J(body.has_value())},
            {"bodyRaw",J(body.value_or(""))}, {"ioTimeoutMs",J(std::int64_t{ioTimeoutMs})}});
        std::cout << q.dump() << std::endl;
        std::string line;
        if (!std::getline(std::cin,line)) throw std::runtime_error("352 RPC EOF");
        const J response = J::parse(line);
        if (response.at("id").integer() != id) throw std::runtime_error("352 RPC identity mismatch");
        if (response.contains("retryableError"))
            throw WinHttpRequestError(response.at("retryableError").boolean());
        const auto status = response.at("status").integer();
        if (status < 100 || status > 599) throw std::runtime_error("352 RPC invalid status");
        return HttpResponse{static_cast<std::int32_t>(status),response.at("bodyRaw").string(),0};
    }
private:
    mutable std::int64_t sequence_ = 0;
};
