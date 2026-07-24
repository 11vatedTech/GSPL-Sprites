#include "gspl/studio/code_folding.hpp"
#include <sstream>
#include <stack>
#include <algorithm>
#include <cctype>

namespace gspl::studio {

CodeFoldingModel::CodeFoldingModel(std::string_view text) : text_(text) {
    find_brace_regions();
}

void CodeFoldingModel::find_brace_regions() {
    regions_.clear();
    
    // Split into lines
    std::vector<std::string> lines;
    std::istringstream stream(text_);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    // Track indent-based folding regions
    std::stack<FoldRegion> region_stack;
    std::stack<size_t> indent_stack;
    std::vector<FoldRegion> regions;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string_view lv = lines[i];
        
        // Skip empty lines
        auto first_non_space = lv.find_first_not_of(" \t");
        if (first_non_space == std::string_view::npos) continue;
        
        // Skip comment-only lines
        if (lv.substr(first_non_space, 2) == "//") continue;
        
        // Check for closing brace regions
        if (lv[first_non_space] == '{') {
            FoldRegion fr;
            fr.start_line = i;
            fr.preview = lv;
            region_stack.push(fr);
            indent_stack.push(first_non_space);
            continue;
        }
        
        if (lv[first_non_space] == '}' && !region_stack.empty()) {
            if (first_non_space == indent_stack.top()) {
                auto fr = region_stack.top();
                region_stack.pop();
                indent_stack.pop();
                fr.end_line = i;
                if (fr.end_line > fr.start_line + 1) {
                    regions.push_back(fr);
                }
            }
            continue;
        }
        
        // Check for keyword-based regions (e.g. entity, morphology, etc.)
        if (lv[first_non_space] == 'e' && lv.substr(first_non_space, 6) == "entity") {
            // entity block - find the opening brace
            auto brace_pos = lv.find('{');
            if (brace_pos != std::string_view::npos) {
                FoldRegion fr;
                fr.start_line = i;
                fr.preview = lv;
                region_stack.push(fr);
                indent_stack.push(first_non_space);
            }
        }
    }
    
    // Close any unclosed regions
    while (!region_stack.empty()) {
        auto fr = region_stack.top(); region_stack.pop();
        indent_stack.pop();
        fr.end_line = lines.size() - 1;
        if (fr.end_line > fr.start_line + 1) {
            regions.push_back(fr);
        }
    }
    
    // Sort by start_line
    std::sort(regions.begin(), regions.end(),
        [](auto const& a, auto const& b) { return a.start_line < b.start_line; });
    
    regions_ = std::move(regions);
}

void CodeFoldingModel::toggle(size_t line) {
    for (auto& r : regions_) {
        if (r.start_line == line) {
            r.collapsed = !r.collapsed;
            return;
        }
    }
}

bool CodeFoldingModel::is_hidden(size_t line) const {
    for (auto const& r : regions_) {
        if (r.collapsed && line > r.start_line && line <= r.end_line)
            return true;
    }
    return false;
}

size_t CodeFoldingModel::visible_line(size_t line) const {
    size_t hidden_before = 0;
    for (auto const& r : regions_) {
        if (r.collapsed && r.start_line < line) {
            hidden_before += (r.end_line - r.start_line);
        }
    }
    return line - hidden_before;
}

void CodeFoldingModel::reset() {
    for (auto& r : regions_) r.collapsed = false;
}

} // namespace gspl::studio
