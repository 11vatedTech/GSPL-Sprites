#include "gspl/studio/ipc_channel.hpp"
#include "gspl/studio/ipc_envelope.hpp"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#else
#include <poll.h>
#include <unistd.h>
#endif

namespace gspl::studio {

namespace {

std::string escape_json(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (auto c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

std::string unescape_json(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            switch (s[i + 1]) {
                case '"': out += '"'; ++i; break;
                case '\\': out += '\\'; ++i; break;
                case '/': out += '/'; ++i; break;
                case 'b': out += '\b'; ++i; break;
                case 'f': out += '\f'; ++i; break;
                case 'n': out += '\n'; ++i; break;
                case 'r': out += '\r'; ++i; break;
                case 't': out += '\t'; ++i; break;
                case 'u':
                    if (i + 5 < s.size()) {
                        unsigned cp = 0;
                        for (int j = 0; j < 4; ++j) {
                            auto h = s[i + 2 + j];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned>(h - 'A' + 10);
                        }
                        if (cp < 0x80) out += static_cast<char>(cp);
                        else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        i += 5;
                    }
                    break;
                default: out += '\\'; break;
            }
        } else {
            out += s[i];
        }
    }
    return out;
}

std::string read_json_string(std::string_view json, std::size_t& pos) {
    if (pos >= json.size() || json[pos] != '"') return {};
    ++pos;
    std::string val;
    bool escaped = false;
    while (pos < json.size()) {
        auto c = json[pos];
        if (escaped) {
            if (c == 'u') {
                if (pos + 4 < json.size()) {
                    val += '\\';
                    val += 'u';
                    val += json.substr(pos + 1, 4);
                    pos += 5;
                } else { ++pos; }
            } else {
                val += '\\';
                val += c;
                ++pos;
            }
            escaped = false;
        } else if (c == '\\') {
            escaped = true;
            ++pos;
        } else if (c == '"') {
            ++pos;
            break;
        } else {
            val += c;
            ++pos;
        }
    }
    return unescape_json(val);
}

std::uint64_t read_json_number(std::string_view json, std::size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\n' || json[pos] == '\r')) ++pos;
    std::string num;
    while (pos < json.size() && ((json[pos] >= '0' && json[pos] <= '9') || json[pos] == '-' || json[pos] == '+' || json[pos] == '.' || json[pos] == 'e' || json[pos] == 'E')) {
        num += json[pos];
        ++pos;
    }
    if (num.empty()) return 0;
    return static_cast<std::uint64_t>(std::stoull(num));
}

bool read_json_bool(std::string_view json, std::size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\n' || json[pos] == '\r')) ++pos;
    if (json.substr(pos, 4) == "true") { pos += 4; return true; }
    if (json.substr(pos, 5) == "false") { pos += 5; return false; }
    return false;
}

void skip_json_whitespace(std::string_view json, std::size_t& pos) {
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' ||
                                 json[pos] == '\n' || json[pos] == '\r')) ++pos;
}

void skip_json_value(std::string_view json, std::size_t& pos) {
    if (pos >= json.size()) return;
    if (json[pos] == '"') { read_json_string(json, pos); }
    else if (json[pos] == 't' || json[pos] == 'f') { read_json_bool(json, pos); }
    else if (json[pos] == 'n') { pos += 4; }
    else {
        while (pos < json.size() && json[pos] != ',' && json[pos] != '}' && json[pos] != ']') ++pos;
    }
}

std::string find_json_string_field(std::string_view json, std::string_view field) {
    auto fpos = json.find(field);
    if (fpos == std::string_view::npos) return {};
    fpos += field.size();
    skip_json_whitespace(json, fpos);
    if (fpos >= json.size() || json[fpos] != ':') return {};
    ++fpos;
    skip_json_whitespace(json, fpos);
    return read_json_string(json, fpos);
}

std::uint64_t find_json_number_field(std::string_view json, std::string_view field) {
    auto fpos = json.find(field);
    if (fpos == std::string_view::npos) return 0;
    fpos += field.size();
    skip_json_whitespace(json, fpos);
    if (fpos >= json.size() || json[fpos] != ':') return 0;
    ++fpos;
    return read_json_number(json, fpos);
}

bool find_json_bool_field(std::string_view json, std::string_view field) {
    auto fpos = json.find(field);
    if (fpos == std::string_view::npos) return false;
    fpos += field.size();
    skip_json_whitespace(json, fpos);
    if (fpos >= json.size() || json[fpos] != ':') return false;
    ++fpos;
    return read_json_bool(json, fpos);
}

#ifdef _WIN32
bool wait_for_data(int fd, std::chrono::milliseconds timeout) {
    HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD ret = WaitForSingleObject(h, static_cast<DWORD>(timeout.count()));
    return ret == WAIT_OBJECT_0;
}

int platform_read(int fd, void* buf, unsigned int count) {
    return static_cast<int>(_read(fd, buf, count));
}

int platform_write(int fd, const void* buf, unsigned int count) {
    return static_cast<int>(_write(fd, buf, count));
}
#else
bool wait_for_data(int fd, std::chrono::milliseconds timeout) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, static_cast<int>(timeout.count()));
    return ret > 0;
}

int platform_read(int fd, void* buf, unsigned int count) {
    return static_cast<int>(::read(fd, buf, count));
}

int platform_write(int fd, const void* buf, unsigned int count) {
    return static_cast<int>(::write(fd, buf, count));
}
#endif

} // anonymous namespace

struct IpcChannel::Impl {
    int read_fd;
    int write_fd;
    std::mutex write_mutex;
    std::thread reader_thread;
    std::atomic<bool> running{false};
    MessageHandler handler;
    bool owns_fds{true};

    Impl(int rfd, int wfd) : read_fd(rfd), write_fd(wfd) {}
    ~Impl() {
        if (owns_fds) {
#ifdef _WIN32
            _close(read_fd);
            _close(write_fd);
#else
            ::close(read_fd);
            ::close(write_fd);
#endif
        }
    }
};

IpcChannel::IpcChannel(int read_fd, int write_fd)
    : impl_(std::make_unique<Impl>(read_fd, write_fd)) {}

IpcChannel::~IpcChannel() {
    stop_async();
}

IpcChannel::IpcChannel(IpcChannel&&) noexcept = default;
IpcChannel& IpcChannel::operator=(IpcChannel&&) noexcept = default;

void IpcChannel::send(const IpcEnvelope& envelope) {
    auto data = envelope.serialize();
    std::uint32_t len = static_cast<std::uint32_t>(data.size());
    std::lock_guard<std::mutex> lock(impl_->write_mutex);
    platform_write(impl_->write_fd, &len, sizeof(len));
    platform_write(impl_->write_fd, data.data(), len);
}

auto IpcChannel::receive(std::chrono::milliseconds timeout) -> IpcEnvelope {
    if (!wait_for_data(impl_->read_fd, timeout)) {
        return IpcEnvelope{};
    }
    std::uint32_t len = 0;
    int nread = platform_read(impl_->read_fd, &len, sizeof(len));
    if (nread != static_cast<int>(sizeof(len))) {
        return IpcEnvelope{};
    }
    if (len == 0) return IpcEnvelope{};
    std::vector<char> buf(len);
    std::size_t total = 0;
    while (total < len) {
        nread = platform_read(impl_->read_fd, buf.data() + total,
                              static_cast<unsigned int>(len - total));
        if (nread <= 0) return IpcEnvelope{};
        total += static_cast<std::size_t>(nread);
    }
    return IpcEnvelope::deserialize(std::string_view(buf.data(), buf.size()));
}

void IpcChannel::start_async(MessageHandler on_message) {
    impl_->handler = std::move(on_message);
    impl_->running = true;
    impl_->reader_thread = std::thread([this]() {
        while (impl_->running) {
            auto env = receive(std::chrono::milliseconds(200));
            if (impl_->handler && (!env.method.empty() || env.message_id != 0)) {
                impl_->handler(std::move(env));
            }
        }
    });
}

void IpcChannel::stop_async() {
    impl_->running = false;
    if (impl_->reader_thread.joinable()) {
        impl_->reader_thread.join();
    }
}

std::string IpcEnvelope::serialize() const {
    std::ostringstream json;
    json << "{\"magic\":" << magic
         << ",\"version\":" << version
         << ",\"message_id\":" << message_id
         << ",\"method\":\"" << escape_json(method)
         << "\",\"payload\":\"" << escape_json(payload)
         << "\",\"has_shared_memory\":" << (has_shared_memory ? "true" : "false")
         << ",\"shared_memory_key\":\"" << escape_json(shared_memory_key)
         << "\",\"shared_memory_size\":" << shared_memory_size
         << "}";
    return json.str();
}

auto IpcEnvelope::deserialize(std::string_view data) -> IpcEnvelope {
    IpcEnvelope env;
    env.magic = static_cast<std::uint32_t>(find_json_number_field(data, "\"magic\""));
    env.version = static_cast<std::uint32_t>(find_json_number_field(data, "\"version\""));
    env.message_id = find_json_number_field(data, "\"message_id\"");
    env.method = find_json_string_field(data, "\"method\"");
    env.payload = find_json_string_field(data, "\"payload\"");
    env.has_shared_memory = find_json_bool_field(data, "\"has_shared_memory\"");
    env.shared_memory_key = find_json_string_field(data, "\"shared_memory_key\"");
    env.shared_memory_size = find_json_number_field(data, "\"shared_memory_size\"");
    return env;
}

} // namespace gspl::studio
