#include "gspl/diagnostics.hpp"
#include <sstream>

namespace gspl {

std::string DiagnosticResult::to_json() const {
    std::ostringstream ss;
    ss << "[";
    for (std::size_t i = 0; i < diagnostics.size(); ++i) {
        auto const& d = diagnostics[i];
        if (i > 0) ss << ",";
        ss << "{"
            << "\"code\":" << static_cast<std::uint32_t>(d.code) << ","
            << "\"severity\":" << static_cast<int>(d.severity) << ","
            << "\"message\":\"" << d.message << "\","
            << "\"span\":{\"offset\":" << d.span.byte_offset
            << ",\"length\":" << d.span.byte_length << "}"
            << "}";
    }
    ss << "]";
    return ss.str();
}

} // namespace gspl
