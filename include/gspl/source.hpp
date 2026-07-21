#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gspl {

using SourceId = std::uint64_t;

struct SourceLocation {
    std::uint32_t line{};
    std::uint32_t column{};
    auto operator<=>(const SourceLocation&) const = default;
};

struct SourceSpan {
    SourceId source_id{};
    SourceLocation start;
    SourceLocation end;
    std::uint64_t byte_offset{};
    std::uint64_t byte_length{};
};

class SourceBuffer {
    friend class SourceManager;
public:
    static SourceBuffer from_string(std::string id, std::string content);
    static SourceBuffer from_file(std::filesystem::path path);
    SourceId id() const { return id_; }
    std::string_view content() const { return content_; }
    std::string const& identifier() const { return identifier_; }
    SourceLocation location_for_offset(std::uint64_t offset) const;
    std::uint64_t offset_for_location(SourceLocation loc) const;
private:
    SourceId id_{};
    std::string identifier_;
    std::string content_;
    mutable std::vector<std::uint64_t> line_offsets_;
    void build_line_offsets() const;
};

class SourceManager {
public:
    SourceId register_buffer(SourceBuffer buffer);
    SourceBuffer const* lookup(SourceId id) const;
    std::size_t count() const { return buffers_.size(); }
    bool is_known_root(std::filesystem::path const& path) const;
    void add_source_root(std::filesystem::path root);
    std::vector<std::filesystem::path> const& source_roots() const { return roots_; }
    SourceSpan make_span(SourceId sid, SourceLocation start, SourceLocation end) const;
private:
    std::vector<SourceBuffer> buffers_;
    std::vector<std::filesystem::path> roots_;
    SourceId next_id_{1};
};

} // namespace gspl
