#include "gspl/source.hpp"
#include "gspl/utf8.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace gspl {

SourceBuffer SourceBuffer::from_string(std::string id, std::string content) {
    SourceBuffer buf;
    buf.identifier_ = std::move(id);
    buf.content_ = std::move(content);
    return buf;
}

SourceBuffer SourceBuffer::from_file(std::filesystem::path path) {
    auto ident = path.string();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::ios_base::failure("Cannot open source file: " + ident);
    auto size = static_cast<std::size_t>(file.tellg());
    file.seekg(0);
    SourceBuffer buf;
    buf.identifier_ = std::move(ident);
    buf.content_.resize(size);
    file.read(buf.content_.data(), static_cast<std::streamsize>(size));
    buf.content_ = strip_bom(std::move(buf.content_));
    return buf;
}

SourceLocation SourceBuffer::location_for_offset(std::uint64_t offset) const {
    build_line_offsets();
    auto it = std::ranges::upper_bound(line_offsets_, offset) - 1;
    auto line_idx = static_cast<std::uint32_t>(it - line_offsets_.begin());
    auto col = static_cast<std::uint32_t>(offset - *it + 1);
    return {line_idx + 1, col};
}

std::uint64_t SourceBuffer::offset_for_location(SourceLocation loc) const {
    build_line_offsets();
    if (loc.line == 0 || loc.line > line_offsets_.size()) return 0;
    auto off = line_offsets_[loc.line - 1] + (loc.column - 1);
    return std::min<std::uint64_t>(off, content_.size());
}

void SourceBuffer::build_line_offsets() const {
    if (!line_offsets_.empty()) return;
    line_offsets_.push_back(0);
    for (std::size_t i = 0; i < content_.size(); ++i) {
        if (content_[i] == '\n') line_offsets_.push_back(i + 1);
    }
}

SourceId SourceManager::register_buffer(SourceBuffer buffer) {
    auto const id = next_id_++;
    buffers_.push_back(std::move(buffer));
    buffers_.back().id_ = id;
    return id;
}

SourceBuffer const* SourceManager::lookup(SourceId id) const {
    for (auto const& buf : buffers_) {
        if (buf.id() == id) return &buf;
    }
    return nullptr;
}

bool SourceManager::is_known_root(std::filesystem::path const& path) const {
    auto norm = std::filesystem::weakly_canonical(path);
    return std::ranges::any_of(roots_, [&](auto const& r) {
        auto rn = std::filesystem::weakly_canonical(r);
        auto [end, _] = std::mismatch(rn.begin(), rn.end(), norm.begin(), norm.end());
        return end == rn.end();
    });
}

void SourceManager::add_source_root(std::filesystem::path root) {
    roots_.push_back(std::filesystem::weakly_canonical(root));
}

SourceSpan SourceManager::make_span(SourceId sid, SourceLocation start, SourceLocation end) const {
    auto const* buf = lookup(sid);
    if (!buf) return {};
    auto boff = buf->offset_for_location(start);
    auto eoff = buf->offset_for_location(end);
    return {sid, start, end, boff, eoff > boff ? eoff - boff : 0};
}

} // namespace gspl
