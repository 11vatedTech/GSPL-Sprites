#pragma once

#include <stdint.h>

#ifdef _WIN32
#define GSPL_PLUGIN_EXPORT __declspec(dllexport)
#else
#define GSPL_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define GSPL_PLUGIN_API_VERSION 1

typedef struct {
    uint32_t api_version;
    const char* plugin_id;
    const char* plugin_version;
    const char* plugin_name;
    const char* plugin_description;
    const char* plugin_author;
} GsplPluginInfo;

typedef struct {
    // Lifecycle
    int (*initialize)(void* context);
    void (*shutdown)(void* context);
    
    // Hook points (set to NULL if not used)
    void (*on_document_open)(const char* uri, void* context);
    void (*on_document_close)(const char* uri, void* context);
    void (*on_diagnostic)(const char* uri, uint32_t line, const char* message, void* context);
    void (*on_build_start)(const char* project_path, void* context);
    void (*on_build_end)(const char* project_path, int success, void* context);
    
    // Editor extension
    const char* (*get_tool_window_title)(void* context);
    void (*render_tool_window)(void* context);
} GsplPluginCallbacks;

typedef int (*GsplPluginInitFunc)(GsplPluginInfo* info, GsplPluginCallbacks* callbacks, void* context);
typedef void (*GsplPluginShutdownFunc)(void* context);

#ifdef __cplusplus
}
#endif
