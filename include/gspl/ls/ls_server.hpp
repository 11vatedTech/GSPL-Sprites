#pragma once

#include "gspl/ls/diagnostic.hpp"
#include "gspl/ls/completion.hpp"
#include "gspl/ls/navigation.hpp"
#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <vector>

namespace gspl::ls {

struct LsRequest {
    std::uint64_t id{0};
    std::string method;
    std::string params;
};

struct LsResponse {
    std::uint64_t id{0};
    std::string result;
    bool error{false};
    std::string error_message;
};

class LsServer {
public:
    using SendResponse = std::function<void(LsResponse)>;

    LsServer();
    ~LsServer();

    LsServer(const LsServer&) = delete;
    LsServer& operator=(const LsServer&) = delete;

    void initialize(std::string_view workspace_root);
    void shutdown();
    void handle_request(LsRequest req, SendResponse send);
    LsResponse handle_request_sync(LsRequest req);

    void open_document(std::string_view uri, std::string_view content);
    std::vector<Diagnostic> get_diagnostics(std::string_view uri) const;
    std::vector<CompletionItem> get_completions(std::string_view uri, std::uint32_t line, std::uint32_t column) const;
    std::vector<Location> goto_definition(std::string_view uri, std::uint32_t line, std::uint32_t column) const;
    std::vector<Reference> find_references(std::string_view uri, std::uint32_t line, std::uint32_t column) const;
    HoverInfo get_hover(std::string_view uri, std::uint32_t line, std::uint32_t column) const;
    std::vector<SymbolInfo> get_document_symbols(std::string_view uri) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace gspl::ls
