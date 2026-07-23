#include "gspl/studio/ipc_envelope.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace {

std::string json_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (auto ch : s) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch;
        }
    }
    return out;
}

std::string json_unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (auto i = s.begin(); i != s.end(); ++i) {
        if (*i == '\\' && (i + 1) != s.end()) {
            switch (*(i + 1)) {
                case '"': out += '"'; ++i; break;
                case '\\': out += '\\'; ++i; break;
                case 'n': out += '\n'; ++i; break;
                case 'r': out += '\r'; ++i; break;
                case 't': out += '\t'; ++i; break;
                default: out += *i;
            }
        } else {
            out += *i;
        }
    }
    return out;
}

} // namespace

namespace gspl::studio {

std::string IpcEnvelope::serialize() const {
    std::ostringstream oss;
    oss << "{\"magic\":" << magic
        << ",\"version\":" << version
        << ",\"message_id\":" << message_id
        << ",\"method\":\"" << json_escape(method) << "\""
        << ",\"payload\":\"" << json_escape(payload) << "\""
        << ",\"has_shared_memory\":" << (has_shared_memory ? "true" : "false");
    if (has_shared_memory) {
        oss << ",\"shared_memory_key\":\"" << json_escape(shared_memory_key) << "\""
            << ",\"shared_memory_size\":" << shared_memory_size;
    }
    oss << "}";
    return oss.str();
}

auto IpcEnvelope::deserialize(std::string_view data) -> IpcEnvelope {
    IpcEnvelope env;
    auto extract = [&](std::string_view key) -> std::string {
        auto pos = data.find(key);
        if (pos == std::string_view::npos) return {};
        pos = data.find(':', pos + key.size());
        if (pos == std::string_view::npos) return {};
        ++pos;
        while (pos < data.size() && (data[pos] == ' ' || data[pos] == '\t')) ++pos;
        if (pos >= data.size()) return {};
        if (data[pos] == '"') {
            ++pos;
            std::string val;
            while (pos < data.size() && data[pos] != '"') {
                if (data[pos] == '\\' && pos + 1 < data.size()) {
                    char esc = data[pos + 1];
                    switch (esc) {
                        case '"': val += '"'; break;
                        case '\\': val += '\\'; break;
                        case '/': val += '/'; break;
                        case 'n': val += '\n'; break;
                        case 'r': val += '\r'; break;
                        case 't': val += '\t'; break;
                        case 'b': val += '\b'; break;
                        case 'f': val += '\f'; break;
                        case 'u': {
                            if (pos + 5 < data.size()) {
                                unsigned cp = 0;
                                for (int j = 0; j < 4; ++j) {
                                    auto h = data[pos + 2 + j];
                                    cp <<= 4;
                                    if (h >= '0' && h <= '9') cp |= static_cast<unsigned>(h - '0');
                                    else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned>(h - 'a' + 10);
                                    else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned>(h - 'A' + 10);
                                }
                                if (cp < 0x80) val += static_cast<char>(cp);
                                else if (cp < 0x800) { val += static_cast<char>(0xC0 | (cp >> 6)); val += static_cast<char>(0x80 | (cp & 0x3F)); }
                                else { val += static_cast<char>(0xE0 | (cp >> 12)); val += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); val += static_cast<char>(0x80 | (cp & 0x3F)); }
                                pos += 5;
                            }
                            break;
                        }
                        default: val += esc; break;
                    }
                    pos += 2;
                } else {
                    val += data[pos];
                    ++pos;
                }
            }
            return val;
        }
        if (data[pos] == 't' || data[pos] == 'f') {
            return (data.substr(pos, 4) == "true") ? "true" : "false";
        }
        std::string num;
        while (pos < data.size() && (std::isdigit(static_cast<unsigned char>(data[pos])) || data[pos] == '-')) {
            num += data[pos];
            ++pos;
        }
        return num;
    };
    auto num_val = [&](std::string_view key) -> std::uint64_t {
        auto s = extract(key);
        if (s.empty()) return 0;
        return std::stoull(s);
    };
    auto bool_val = [&](std::string_view key) -> bool {
        return extract(key) == "true";
    };
    env.magic = static_cast<std::uint32_t>(num_val("magic"));
    env.version = static_cast<std::uint32_t>(num_val("version"));
    env.message_id = num_val("message_id");
    env.method = extract("method");
    env.payload = extract("payload");
    env.has_shared_memory = bool_val("has_shared_memory");
    env.shared_memory_key = extract("shared_memory_key");
    env.shared_memory_size = num_val("shared_memory_size");
    return env;
}

} // namespace gspl::studio
