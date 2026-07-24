#include "gspl/package/remote_registry.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <sstream>
#include <cstdio>
#include <cstring>

namespace gspl::package {

struct RemoteRegistry::Impl {
    std::string registry_url;
    ProgressCallback progress_cb;

    void progress(std::string_view op, float pct) {
        if (progress_cb) progress_cb(op, pct);
    }

    // Parse URL into host, port, path
    struct ParsedUrl {
        std::string host;
        int port = 443;
        std::string path;
        bool use_tls = true;
    };

    static ParsedUrl parse_url(const std::string& url) {
        ParsedUrl result;
        std::string s = url;

        // Strip protocol
        size_t proto_end = s.find("://");
        if (proto_end != std::string::npos) {
            std::string proto = s.substr(0, proto_end);
            result.use_tls = (proto != "http");
            s = s.substr(proto_end + 3);
        }

        // Split host:port / path
        size_t path_start = s.find('/');
        std::string host_port;
        if (path_start != std::string::npos) {
            host_port = s.substr(0, path_start);
            result.path = s.substr(path_start);
        } else {
            host_port = s;
            result.path = "/";
        }

        // Check for port
        size_t colon = host_port.find(':');
        if (colon != std::string::npos) {
            result.host = host_port.substr(0, colon);
            result.port = std::stoi(host_port.substr(colon + 1));
        } else {
            result.host = host_port;
            result.port = result.use_tls ? 443 : 80;
        }

        return result;
    }

#if defined(_WIN32)
    // WinHTTP-based implementation
    std::string http_request(const std::string& method, const std::string& url_path,
                             const std::string& body = {}) {
        auto parsed = parse_url(registry_url);
        std::string full_path = parsed.path;
        if (!url_path.empty()) {
            if (full_path.back() == '/') full_path.pop_back();
            full_path += url_path;
        }

        HINTERNET hSession = WinHttpOpen(L"GSPL-Sprites/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, nullptr, nullptr, 0);
        if (!hSession) return {};

        std::wstring whost(parsed.host.begin(), parsed.host.end());
        HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(),
            static_cast<INTERNET_PORT>(parsed.port), 0);
        if (!hConnect) { WinHttpCloseHandle(hSession); return {}; }

        std::wstring wpath(full_path.begin(), full_path.end());
        std::wstring wmethod(method.begin(), method.end());
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, wmethod.c_str(), wpath.c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            parsed.use_tls ? WINHTTP_FLAG_SECURE : 0);
        if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return {}; }

        // Set timeouts
        WinHttpSetTimeouts(hRequest, 5000, 5000, 5000, 10000);

        // Send request
        LPCWSTR headers = L"Content-Type: application/json\r\n";
        void* body_data = body.empty() ? nullptr : const_cast<char*>(body.data());
        DWORD body_len = static_cast<DWORD>(body.size());

        if (!WinHttpSendRequest(hRequest, headers, (DWORD)-1, body_data, body_len, body_len, 0)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return {};
        }

        if (!WinHttpReceiveResponse(hRequest, nullptr)) {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return {};
        }

        // Read response
        std::string response;
        DWORD bytes_available = 0;
        while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
            std::vector<char> buffer(bytes_available);
            DWORD bytes_read = 0;
            if (WinHttpReadData(hRequest, buffer.data(), bytes_available, &bytes_read)) {
                response.append(buffer.data(), bytes_read);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return response;
    }
#else
    // Stub for non-Windows platforms (CORE_ONLY)
    std::string http_request(const std::string& method, const std::string& url_path,
                             const std::string& body = {}) {
        (void)method; (void)url_path; (void)body;
        return {};
    }
#endif
};

RemoteRegistry::RemoteRegistry(std::string registry_url)
    : impl_(std::make_unique<Impl>()) {
    impl_->registry_url = std::move(registry_url);
}

RemoteRegistry::~RemoteRegistry() = default;

void RemoteRegistry::set_progress_callback(ProgressCallback cb) {
    impl_->progress_cb = std::move(cb);
}

bool RemoteRegistry::is_available() const {
#if defined(_WIN32)
    return !impl_->registry_url.empty();
#else
    return false;
#endif
}

std::optional<std::vector<RegistryPackage>> RemoteRegistry::list_versions(const std::string& package_id) {
    std::string api_path = "/api/v1/packages/" + package_id;
    auto response = impl_->http_request("GET", api_path);
    if (response.empty()) return std::nullopt;

    // Simple JSON array parser for version list
    std::vector<RegistryPackage> packages;
    size_t pos = 0;

    // Find array start
    pos = response.find('[');
    if (pos == std::string::npos) return std::nullopt;
    ++pos;

    while (pos < response.size()) {
        // Skip whitespace
        while (pos < response.size() && (response[pos] == ' ' || response[pos] == '\t' || response[pos] == '\n' || response[pos] == '\r'))
            ++pos;
        if (pos >= response.size() || response[pos] == ']') break;

        if (response[pos] == '{') {
            RegistryPackage pkg;
            ++pos;
            while (pos < response.size() && response[pos] != '}') {
                // Skip whitespace
                while (pos < response.size() && (response[pos] == ' ' || response[pos] == '\t' || response[pos] == '\n' || response[pos] == '\r'))
                    ++pos;
                if (pos >= response.size() || response[pos] == '}') break;

                if (response[pos] == '"') {
                    ++pos;
                    std::string key;
                    while (pos < response.size() && response[pos] != '"') {
                        if (response[pos] == '\\' && pos + 1 < response.size()) {
                            key += response[pos + 1];
                            pos += 2;
                        } else {
                            key += response[pos++];
                        }
                    }
                    if (pos < response.size()) ++pos; // skip closing quote

                    // Skip colon
                    while (pos < response.size() && response[pos] != ':') ++pos;
                    if (pos < response.size()) ++pos;

                    // Skip whitespace
                    while (pos < response.size() && (response[pos] == ' ' || response[pos] == '\t' || response[pos] == '\n' || response[pos] == '\r'))
                        ++pos;

                    // Read value
                    if (pos < response.size() && response[pos] == '"') {
                        ++pos;
                        std::string val;
                        while (pos < response.size() && response[pos] != '"') {
                            if (response[pos] == '\\' && pos + 1 < response.size()) {
                                val += response[pos + 1];
                                pos += 2;
                            } else {
                                val += response[pos++];
                            }
                        }
                        if (pos < response.size()) ++pos;

                        if (key == "package_id") pkg.package_id = val;
                        else if (key == "version") pkg.version = val;
                        else if (key == "download_url") pkg.download_url = val;
                        else if (key == "checksum") pkg.checksum = val;
                    } else if (pos < response.size() && (response[pos] == '-' || (response[pos] >= '0' && response[pos] <= '9'))) {
                        size_t num_start = pos;
                        while (pos < response.size() && (response[pos] == '-' || response[pos] == '.' || (response[pos] >= '0' && response[pos] <= '9')))
                            ++pos;
                        if (key == "size") pkg.size = static_cast<size_t>(std::stoll(response.substr(num_start, pos - num_start)));
                    }
                }
                // Skip comma
                while (pos < response.size() && (response[pos] == ',' || response[pos] == ' ' || response[pos] == '\t' || response[pos] == '\n' || response[pos] == '\r'))
                    ++pos;
            }
            if (pos < response.size()) ++pos; // skip }
            packages.push_back(std::move(pkg));
        }
        // Skip comma
        while (pos < response.size() && (response[pos] == ',' || response[pos] == ' ' || response[pos] == '\t' || response[pos] == '\n' || response[pos] == '\r'))
            ++pos;
    }

    return packages;
}

std::optional<RegistryPackage> RemoteRegistry::fetch_package_info(const std::string& package_id, const std::string& version) {
    auto versions = list_versions(package_id);
    if (!versions) return std::nullopt;

    for (auto const& v : *versions) {
        if (v.version == version) return v;
    }

    // Return latest if no version match
    if (!versions->empty()) return versions->front();
    return std::nullopt;
}

std::pair<bool, RegistryError> RemoteRegistry::download(const std::string& package_id, const std::string& version,
                                                          const std::string& dest_path) {
    impl_->progress("downloading", 0.1f);

    auto info = fetch_package_info(package_id, version);
    if (!info) {
        impl_->progress("error", 0);
        return {false, RegistryError::NotFound};
    }

    // Use download_url from registry info
    std::string url = info->download_url;
    if (url.empty()) {
        url = impl_->registry_url + "/api/v1/packages/" + package_id + "/" + version + "/download";
    }

    auto parsed = Impl::parse_url(url);
    std::string response;
    {
#if defined(_WIN32)
        auto saved_url = std::move(impl_->registry_url);
        impl_->registry_url = url;
        response = impl_->http_request("GET", "");
        impl_->registry_url = std::move(saved_url);
#else
        (void)response;
#endif
    }

    if (response.empty()) {
        impl_->progress("error", 0);
        return {false, RegistryError::NetworkError};
    }

    // Write to destination
    FILE* f = nullptr;
#if defined(_MSC_VER)
    fopen_s(&f, dest_path.c_str(), "wb");
#else
    f = std::fopen(dest_path.c_str(), "wb");
#endif
    if (!f) {
        impl_->progress("error", 0);
        return {false, RegistryError::NetworkError};
    }
    std::fwrite(response.data(), 1, response.size(), f);
    std::fclose(f);

    impl_->progress("completed", 1.0f);
    return {true, RegistryError::None};
}

std::pair<bool, RegistryError> RemoteRegistry::publish(const std::string& package_id, const std::string& version,
                                                         const std::string& archive_path) {
    impl_->progress("publishing", 0.1f);

    // Read archive
    FILE* f = nullptr;
#if defined(_MSC_VER)
    fopen_s(&f, archive_path.c_str(), "rb");
#else
    f = std::fopen(archive_path.c_str(), "rb");
#endif
    if (!f) {
        impl_->progress("error", 0);
        return {false, RegistryError::NetworkError};
    }
    std::fseek(f, 0, SEEK_END);
    long file_size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string body(file_size, '\0');
    std::fread(body.data(), 1, file_size, f);
    std::fclose(f);

    std::string api_path = "/api/v1/packages/" + package_id + "/" + version;
    auto response = impl_->http_request("PUT", api_path, body);

    if (response.empty()) {
        impl_->progress("error", 0);
        return {false, RegistryError::NetworkError};
    }

    impl_->progress("completed", 1.0f);
    return {true, RegistryError::None};
}

} // namespace gspl::package
