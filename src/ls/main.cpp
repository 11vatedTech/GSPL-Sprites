#include "gspl/ls/ls_server.hpp"
#include <iostream>
#include <string>
#include <cstdio>

int main(int argc, char* argv[]) {
    gspl::ls::LsServer server;
    
    std::string workspace_root = ".";
    if (argc > 1) workspace_root = argv[1];
    
    server.initialize(workspace_root);
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        gspl::ls::LsRequest req;
        req.id++;
        req.method = line;
        
        auto resp = server.handle_request_sync(req);
        std::cout << resp.result << std::endl;
    }
    
    server.shutdown();
    return 0;
}
