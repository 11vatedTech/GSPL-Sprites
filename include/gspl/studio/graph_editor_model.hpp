#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace gspl::studio {

struct GraphNodePort {
    std::string name;
    std::string type;
    bool is_input = true;
};

struct GraphNode {
    std::string id;
    std::string label;
    std::string node_type;
    double x = 0, y = 0;
    std::vector<GraphNodePort> inputs;
    std::vector<GraphNodePort> outputs;
};

struct GraphEdge {
    std::string id;
    std::string source_node;
    std::string source_port;
    std::string target_node;
    std::string target_port;
};

class GraphEditorModel {
public:
    GraphEditorModel() = default;

    std::vector<GraphNode> const& nodes() const { return nodes_; }
    std::vector<GraphEdge> const& edges() const { return edges_; }
    GraphNode const* node(std::string_view id) const;
    GraphEdge const* edge(std::string_view id) const;

    std::string add_node(std::string_view label, std::string_view node_type, double x, double y);
    bool remove_node(std::string_view id);
    bool move_node(std::string_view id, double x, double y);

    std::string add_edge(std::string_view src_node, std::string_view src_port,
                         std::string_view tgt_node, std::string_view tgt_port);
    bool remove_edge(std::string_view id);

    void clear();
    std::vector<GraphNode> connected_nodes(std::string_view node_id) const;

    using ChangeCallback = std::function<void()>;
    void set_change_callback(ChangeCallback cb) { change_cb_ = std::move(cb); }

private:
    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    ChangeCallback change_cb_;
    std::string generate_id();
};

} // namespace gspl::studio
