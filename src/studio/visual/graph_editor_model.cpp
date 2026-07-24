#include "gspl/studio/graph_editor_model.hpp"
#include <algorithm>
#include <sstream>

namespace gspl::studio {

std::string GraphEditorModel::generate_id() {
    static std::uint64_t counter = 0;
    return "node_" + std::to_string(counter++);
}

GraphNode const* GraphEditorModel::node(std::string_view id) const {
    for (auto const& n : nodes_) if (n.id == id) return &n;
    return nullptr;
}

GraphEdge const* GraphEditorModel::edge(std::string_view id) const {
    for (auto const& e : edges_) if (e.id == id) return &e;
    return nullptr;
}

std::string GraphEditorModel::add_node(std::string_view label, std::string_view node_type, double x, double y) {
    GraphNode n;
    n.id = generate_id();
    n.label = std::string(label);
    n.node_type = std::string(node_type);
    n.x = x;
    n.y = y;
    // Add default ports based on type
    GraphNodePort input;
    input.name = "in";
    input.type = node_type == "gene" ? "gene" : "signal";
    input.is_input = true;
    n.inputs.push_back(input);
    GraphNodePort output;
    output.name = "out";
    output.type = node_type == "gene" ? "gene" : "signal";
    output.is_input = false;
    n.outputs.push_back(output);
    nodes_.push_back(std::move(n));
    if (change_cb_) change_cb_();
    return nodes_.back().id;
}

bool GraphEditorModel::remove_node(std::string_view id) {
    // Remove connected edges first
    edges_.erase(std::remove_if(edges_.begin(), edges_.end(),
        [&](GraphEdge const& e) { return e.source_node == id || e.target_node == id; }),
        edges_.end());
    // Remove node
    auto it = std::find_if(nodes_.begin(), nodes_.end(),
        [&](GraphNode const& n) { return n.id == id; });
    if (it == nodes_.end()) return false;
    nodes_.erase(it);
    if (change_cb_) change_cb_();
    return true;
}

bool GraphEditorModel::move_node(std::string_view id, double x, double y) {
    for (auto& n : nodes_) { if (n.id == id) { n.x = x; n.y = y; if (change_cb_) change_cb_(); return true; } }
    return false;
}

std::string GraphEditorModel::add_edge(std::string_view src_node, std::string_view src_port,
                                        std::string_view tgt_node, std::string_view tgt_port) {
    // Validate nodes exist
    if (!node(src_node) || !node(tgt_node)) return "";
    GraphEdge e;
    static std::uint64_t ec = 0;
    e.id = "edge_" + std::to_string(ec++);
    e.source_node = std::string(src_node);
    e.source_port = std::string(src_port);
    e.target_node = std::string(tgt_node);
    e.target_port = std::string(tgt_port);
    edges_.push_back(std::move(e));
    if (change_cb_) change_cb_();
    return edges_.back().id;
}

bool GraphEditorModel::remove_edge(std::string_view id) {
    auto it = std::find_if(edges_.begin(), edges_.end(),
        [&](GraphEdge const& e) { return e.id == id; });
    if (it == edges_.end()) return false;
    edges_.erase(it);
    if (change_cb_) change_cb_();
    return true;
}

void GraphEditorModel::clear() {
    nodes_.clear();
    edges_.clear();
    if (change_cb_) change_cb_();
}

std::vector<GraphNode> GraphEditorModel::connected_nodes(std::string_view node_id) const {
    std::vector<GraphNode> result;
    for (auto const& e : edges_) {
        if (e.source_node == node_id) {
            auto* n = node(e.target_node);
            if (n) result.push_back(*n);
        }
        if (e.target_node == node_id) {
            auto* n = node(e.source_node);
            if (n) result.push_back(*n);
        }
    }
    return result;
}

} // namespace gspl::studio
