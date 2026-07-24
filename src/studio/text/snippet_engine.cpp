#include "gspl/studio/snippet_engine.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace gspl::studio {

SnippetEngine::SnippetEngine() {
    add_builtins();
}

void SnippetEngine::add_snippet(Snippet snip) {
    snippets_.push_back(std::move(snip));
}

void SnippetEngine::add_builtins() {
    std::vector<Snippet> builtins = {
        {"entity", "entity declaration",
         "entity ${1:EntityName} {\n    gene identity {\n        stable_id: \"${2:id}\";\n    }\n\n    morphology {\n        part ${3:core} { size 1.0, 1.0, 1.0; }\n    }\n}",
         "gspl"},
        {"module", "module declaration",
         "module ${1:namespace}.${2:name};\n\n$0",
         "gspl"},
        {"part", "morphology part",
         "part ${1:name} {\n    size ${2:1.0}, ${3:1.0}, ${4:1.0};\n    offset ${5:0.0}, ${6:0.0}, ${7:0.0};\n}",
         "gspl"},
        {"gene", "gene declaration",
         "gene ${1:name} {\n    ${2:stable_id}: \"${3:id}\";\n}",
         "gspl"},
        {"if", "if statement",
         "if (${1:condition}) {\n    $0\n}",
         "gspl"},
        {"for", "for loop",
         "for ${1:i} in ${2:range} {\n    $0\n}",
         "gspl"},
        {"rule", "behavior rule",
         "rule ${1:name} {\n    on stimulus \"${2:stimulus}\";\n    action ${3:action};\n    priority ${4:5};\n}",
         "gspl"},
        {"animation", "animation block",
         "animation ${1:name} {\n    loop ${2:true};\n    fps ${3:12};\n    keyframe 0 { $0 }\n}",
         "gspl"},
        {"keyframe", "animation keyframe",
         "keyframe ${1:frame} {\n    part ${2:name} {\n        offset ${3:0.0}, ${4:0.0}, ${5:0.0};\n    }\n}",
         "gspl"},
        {"joint", "morphology joint",
         "joint ${1:name} {\n    from ${2:parent};\n    to ${3:child};\n    type ${4:hinge};\n}",
         "gspl"},
    };
    for (auto& s : builtins) {
        snippets_.push_back(std::move(s));
    }
}

void SnippetEngine::clear() {
    snippets_.clear();
}

std::vector<Snippet const*> SnippetEngine::find(std::string_view prefix) const {
    std::vector<Snippet const*> result;
    for (auto const& s : snippets_) {
        if (s.prefix.find(prefix) == 0)
            result.push_back(&s);
    }
    return result;
}

SnippetExpansion SnippetEngine::expand(std::string_view body) {
    SnippetExpansion result;
    std::string output;
    std::vector<SnippetTabStop> stops;
    size_t max_index = 0;

    for (size_t i = 0; i < body.size(); ++i) {
        if (body[i] == '\\' && i + 1 < body.size()) {
            output += body[i + 1];
            i += 1;
            continue;
        }
        if (body[i] == '$' && i + 1 < body.size()) {
            if (body[i + 1] == '$') {
                output += '$';
                i += 1;
                continue;
            }
            if (body[i + 1] == '{') {
                // ${index:default} or ${index}
                size_t end = body.find('}', i + 2);
                if (end == std::string_view::npos) {
                    output += body.substr(i);
                    break;
                }
                std::string content(body.substr(i + 2, end - i - 2));
                size_t colon = content.find(':');
                SnippetTabStop stop;
                stop.start = output.size();
                if (colon != std::string_view::npos) {
                    stop.index = static_cast<size_t>(std::stoi(content.substr(0, colon)));
                    stop.default_value = content.substr(colon + 1);
                    stop.is_placeholder = true;
                    output += stop.default_value;
                } else {
                    stop.index = static_cast<size_t>(std::stoi(content));
                    stop.default_value.clear();
                    stop.is_placeholder = (stop.index != 0);
                    output += "";
                }
                stop.end = output.size();
                if (stop.index > 0) stops.push_back(stop);
                if (stop.index > max_index) max_index = stop.index;
                i = end;
                continue;
            }
            if (body[i + 1] >= '0' && body[i + 1] <= '9') {
                // $0, $1, etc. - simple tab stop
                size_t num_end = i + 2;
                while (num_end < body.size() && body[num_end] >= '0' && body[num_end] <= '9') ++num_end;
                size_t index = static_cast<size_t>(std::stoi(std::string(body.substr(i + 1, num_end - i - 1))));
                SnippetTabStop stop;
                stop.index = index;
                stop.start = output.size();
                stop.end = output.size();
                stop.is_placeholder = (index != 0);
                if (index > 0) stops.push_back(stop);
                if (index > max_index) max_index = index;
                i = num_end - 1;
                continue;
            }
        }
        output += body[i];
    }

    result.text = output;
    result.tab_stops = std::move(stops);
    result.end_cursor_pos = output.size();
    return result;
}

} // namespace gspl::studio
