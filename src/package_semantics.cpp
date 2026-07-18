#include "package_semantics.hpp"

#include "gspl_sprites/core.hpp"

#include <algorithm>
#include <charconv>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace gspl::sprites {
namespace {
struct GraphNode {
  std::string content_hash;
  std::vector<std::string> dependencies;
  std::string id;
  std::string compiler_pass;
  std::string provenance;
  std::uint32_t schema_version{};
  std::string target;
  std::string type;
  std::string validation;
};

struct Provenance {
  std::uint32_t actor{};
  std::string actor_identity;
  std::string id;
  std::vector<std::string> inputs;
  std::string operation;
  std::string output;
};

class Reader final {
public:
  explicit Reader(std::string_view source) : source_(source) {}

  void expect(std::string_view token) {
    if (source_.substr(cursor_, token.size()) != token)
      fail("canonical token mismatch");
    cursor_ += token.size();
  }

  bool consume(char value) noexcept {
    if (cursor_ < source_.size() && source_[cursor_] == value) {
      ++cursor_;
      return true;
    }
    return false;
  }

  std::string string() {
    if (!consume('"'))
      fail("expected string");
    std::string result;
    while (cursor_ < source_.size()) {
      const auto value = static_cast<unsigned char>(source_[cursor_++]);
      if (value == '"')
        return result;
      if (value < 0x20)
        fail("control character in string");
      if (value != '\\') {
        result += static_cast<char>(value);
        continue;
      }
      if (cursor_ == source_.size())
        fail("truncated escape");
      switch (source_[cursor_++]) {
      case '"':
        result += '"';
        break;
      case '\\':
        result += '\\';
        break;
      case 'n':
        result += '\n';
        break;
      case 'r':
        result += '\r';
        break;
      case 't':
        result += '\t';
        break;
      default:
        fail("non-canonical escape");
      }
    }
    fail("unterminated string");
  }

  std::uint32_t integer() {
    std::uint32_t result{};
    const auto begin = source_.data() + cursor_;
    const auto [end, error] =
        std::from_chars(begin, source_.data() + source_.size(), result);
    if (error != std::errc{} || end == begin)
      fail("invalid integer");
    cursor_ = static_cast<std::size_t>(end - source_.data());
    return result;
  }

  std::vector<std::string> strings() {
    std::vector<std::string> result;
    expect("[");
    if (consume(']'))
      return result;
    for (;;) {
      result.push_back(string());
      if (consume(']'))
        return result;
      expect(",");
    }
  }

  void finish() const {
    if (cursor_ != source_.size())
      fail("trailing data");
  }

private:
  [[noreturn]] void fail(std::string_view message) const {
    throw std::runtime_error(std::string(message) + " at byte " +
                             std::to_string(cursor_));
  }

  std::string_view source_;
  std::size_t cursor_{};
};

std::vector<GraphNode> parse_graph(std::string_view source,
                                   std::uint32_t maximum) {
  Reader reader(source);
  std::vector<GraphNode> result;
  reader.expect("{\"artifacts\":[");
  if (!reader.consume(']')) {
    for (;;) {
      if (result.size() >= maximum)
        throw std::runtime_error("asset graph record limit exceeded");
      GraphNode node;
      reader.expect("{\"contentHash\":");
      node.content_hash = reader.string();
      reader.expect(",\"dependencies\":");
      node.dependencies = reader.strings();
      reader.expect(",\"id\":");
      node.id = reader.string();
      reader.expect(",\"pass\":");
      node.compiler_pass = reader.string();
      reader.expect(",\"provenance\":");
      node.provenance = reader.string();
      reader.expect(",\"schemaVersion\":");
      node.schema_version = reader.integer();
      reader.expect(",\"target\":");
      node.target = reader.string();
      reader.expect(",\"type\":");
      node.type = reader.string();
      reader.expect(",\"validation\":");
      node.validation = reader.string();
      reader.expect("}");
      result.push_back(std::move(node));
      if (reader.consume(']'))
        break;
      reader.expect(",");
    }
  }
  reader.expect("}");
  reader.finish();
  return result;
}

std::vector<Provenance> parse_provenance(std::string_view source,
                                         std::uint32_t maximum) {
  Reader reader(source);
  std::vector<Provenance> result;
  reader.expect("{\"records\":[");
  if (!reader.consume(']')) {
    for (;;) {
      if (result.size() >= maximum)
        throw std::runtime_error("provenance record limit exceeded");
      Provenance record;
      reader.expect("{\"actor\":");
      record.actor = reader.integer();
      reader.expect(",\"actorIdentity\":");
      record.actor_identity = reader.string();
      reader.expect(",\"id\":");
      record.id = reader.string();
      reader.expect(",\"inputs\":");
      record.inputs = reader.strings();
      reader.expect(",\"operation\":");
      record.operation = reader.string();
      reader.expect(",\"output\":");
      record.output = reader.string();
      reader.expect("}");
      result.push_back(std::move(record));
      if (reader.consume(']'))
        break;
      reader.expect(",");
    }
  }
  reader.expect("}");
  reader.finish();
  return result;
}

bool hash(std::string_view value) {
  return value.size() == 64 &&
         std::ranges::all_of(value, [](unsigned char item) {
           return (item >= '0' && item <= '9') || (item >= 'a' && item <= 'f');
         });
}

std::string node_identity(const GraphNode &node) {
  std::ostringstream identity;
  identity << node.type << '\n'
           << node.schema_version << '\n'
           << node.content_hash << '\n'
           << node.compiler_pass << '\n'
           << node.provenance << '\n'
           << node.target;
  for (const auto &dependency : node.dependencies)
    identity << '\n' << dependency;
  return sha256(identity.str());
}
} // namespace

ValidationResult
validate_package_semantic_closure(std::string_view asset_graph,
                                  std::string_view provenance,
                                  std::uint32_t maximum_records) {
  ValidationResult result;
  const auto add = [&](std::string code, std::string message) {
    result.diagnostics.push_back({std::move(code), std::move(message)});
  };
  try {
    const auto nodes = parse_graph(asset_graph, maximum_records);
    const auto records = parse_provenance(provenance, maximum_records);
    if (nodes.empty() || records.empty())
      add("SPRITE_PACKAGE_SEMANTIC_CLOSURE_EMPTY",
          "asset graph and provenance closure must not be empty");
    std::map<std::string, const GraphNode *, std::less<>> node_by_id;
    std::map<std::string, const Provenance *, std::less<>> record_by_id;
    std::string previous;
    for (const auto &node : nodes) {
      if (!hash(node.id) || !hash(node.content_hash) ||
          node.schema_version != 1 || node.validation != "VALID" ||
          node.type.empty() || node.compiler_pass.empty() ||
          node.provenance.empty() || node.target.empty())
        add("SPRITE_PACKAGE_GRAPH_NODE_INVALID",
            "asset graph node metadata is invalid: " + node.id);
      if (!previous.empty() && node.id <= previous)
        add("SPRITE_PACKAGE_GRAPH_NONCANONICAL",
            "asset graph nodes are not strictly ordered");
      previous = node.id;
      if (!std::ranges::is_sorted(node.dependencies) ||
          std::ranges::adjacent_find(node.dependencies) !=
              node.dependencies.end())
        add("SPRITE_PACKAGE_GRAPH_DEPENDENCIES_NONCANONICAL",
            "asset dependencies are unsorted or duplicated: " + node.id);
      if (node_identity(node) != node.id)
        add("SPRITE_PACKAGE_GRAPH_IDENTITY_MISMATCH",
            "asset node identity does not match semantic content: " + node.id);
      if (!node_by_id.emplace(node.id, &node).second)
        add("SPRITE_PACKAGE_GRAPH_NODE_DUPLICATE",
            "duplicate asset graph node: " + node.id);
    }
    previous.clear();
    for (const auto &record : records) {
      if (record.id.empty() || record.actor > 3 ||
          record.actor_identity.empty() || record.operation.empty() ||
          !hash(record.output))
        add("SPRITE_PACKAGE_PROVENANCE_RECORD_INVALID",
            "provenance record metadata is invalid: " + record.id);
      if (!previous.empty() && record.id <= previous)
        add("SPRITE_PACKAGE_PROVENANCE_NONCANONICAL",
            "provenance records are not strictly ordered");
      previous = record.id;
      if (!std::ranges::is_sorted(record.inputs) ||
          std::ranges::adjacent_find(record.inputs) != record.inputs.end())
        add("SPRITE_PACKAGE_PROVENANCE_INPUTS_NONCANONICAL",
            "provenance inputs are unsorted or duplicated: " + record.id);
      if (!record_by_id.emplace(record.id, &record).second)
        add("SPRITE_PACKAGE_PROVENANCE_DUPLICATE",
            "duplicate provenance record: " + record.id);
    }
    for (const auto &node : nodes) {
      for (const auto &dependency : node.dependencies)
        if (!node_by_id.contains(dependency))
          add("SPRITE_PACKAGE_GRAPH_DEPENDENCY_MISSING",
              "asset dependency is absent: " + dependency);
      const auto found = record_by_id.find(node.provenance);
      if (found == record_by_id.end()) {
        add("SPRITE_PACKAGE_PROVENANCE_MISSING",
            "asset provenance is absent: " + node.provenance);
        continue;
      }
      const auto &record = *found->second;
      if (record.inputs != node.dependencies ||
          record.output != node.content_hash)
        add("SPRITE_PACKAGE_PROVENANCE_MISMATCH",
            "provenance does not describe its asset node: " + node.id);
    }
    std::map<std::string, unsigned, std::less<>> visit;
    bool cycle_reported = false;
    std::function<void(const GraphNode &)> visit_node =
        [&](const GraphNode &node) {
          auto &state = visit[node.id];
          if (state == 2)
            return;
          if (state == 1) {
            if (!cycle_reported)
              add("SPRITE_PACKAGE_GRAPH_CYCLE",
                  "asset graph contains a dependency cycle");
            cycle_reported = true;
            return;
          }
          state = 1;
          for (const auto &dependency : node.dependencies) {
            const auto found = node_by_id.find(dependency);
            if (found != node_by_id.end())
              visit_node(*found->second);
          }
          state = 2;
        };
    for (const auto &node : nodes)
      visit_node(node);
    for (const auto &record : records)
      if (std::ranges::none_of(nodes, [&](const auto &node) {
            return node.provenance == record.id;
          }))
        add("SPRITE_PACKAGE_PROVENANCE_ORPHANED",
            "provenance record has no asset node: " + record.id);
  } catch (const std::exception &error) {
    add("SPRITE_PACKAGE_SEMANTIC_DOCUMENT_MALFORMED", error.what());
  }
  return result;
}

} // namespace gspl::sprites
