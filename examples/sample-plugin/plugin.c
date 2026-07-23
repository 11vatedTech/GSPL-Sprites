#include "gspl/plugin/plugin_api.h"
#include <stdio.h>
#include <string.h>

static int initialized = 0;
static int documents_opened = 0;
static int builds_completed = 0;

static int plugin_initialize(void* context) {
    (void)context;
    initialized = 1;
    printf("[SamplePlugin] initialized\n");
    return 0;
}

static void plugin_shutdown(void* context) {
    (void)context;
    printf("[SamplePlugin] shutdown (opened=%d, builds=%d)\n",
           documents_opened, builds_completed);
    initialized = 0;
}

static void plugin_on_document_open(const char* uri, void* context) {
    (void)context;
    documents_opened++;
    printf("[SamplePlugin] document opened: %s\n", uri);
}

static void plugin_on_document_close(const char* uri, void* context) {
    (void)context;
    printf("[SamplePlugin] document closed: %s\n", uri);
}

static void plugin_on_diagnostic(const char* uri, uint32_t line, const char* message, void* context) {
    (void)context;
    printf("[SamplePlugin] diagnostic at %s:%u: %s\n", uri, line, message);
}

static void plugin_on_build_start(const char* project_path, void* context) {
    (void)context;
    printf("[SamplePlugin] build started: %s\n", project_path);
}

static void plugin_on_build_end(const char* project_path, int success, void* context) {
    (void)context;
    builds_completed++;
    printf("[SamplePlugin] build %s: %s\n",
           success ? "succeeded" : "failed", project_path);
}

int gspl_plugin_init(GsplPluginInfo* info, GsplPluginCallbacks* callbacks, void* context) {
    (void)context;

    info->api_version = GSPL_PLUGIN_API_VERSION;
    info->plugin_id = "gspl-sample-plugin";
    info->plugin_version = "1.0.0";
    info->plugin_name = "GSPL Sample Plugin";
    info->plugin_description = "Demonstrates all plugin API hook points";
    info->plugin_author = "GSPL Sprites Team";

    callbacks->initialize = plugin_initialize;
    callbacks->shutdown = plugin_shutdown;
    callbacks->on_document_open = plugin_on_document_open;
    callbacks->on_document_close = plugin_on_document_close;
    callbacks->on_diagnostic = plugin_on_diagnostic;
    callbacks->on_build_start = plugin_on_build_start;
    callbacks->on_build_end = plugin_on_build_end;

    return 0;
}

void gspl_plugin_shutdown(void* context) {
    (void)context;
    plugin_shutdown(context);
}
