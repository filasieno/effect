#pragma once

#include "lspd_basic.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "ak/runtime/runtime_api.hpp"

// Simple static hash for method strings -> enum. Initialized at init.

struct lsp_method_kv {
    const char* name;
    lsp_method_type  value;
};

struct lsp_method_map_entry {
    const char* key;
    lsp_method_type  value;
};

struct lsp_method_map {
    lsp_method_map_entry* buckets;
    std::size_t           capacity;
};

// FNV-1a 64-bit hash for strings
static inline std::uint64_t lsp_fnv1a64(const char* s) noexcept {
    const std::uint64_t fnv_offset = 1469598103934665603ull;
    const std::uint64_t fnv_prime  = 1099511628211ull;
    std::uint64_t h = fnv_offset;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        h ^= (std::uint64_t)(*p);
        h *= fnv_prime;
    }
    return h;
}

static inline void lsp_method_map_init(lsp_method_map* map, lsp_method_map_entry* storage, std::size_t capacity) noexcept {
    map->buckets  = storage;
    map->capacity = capacity;
    for (std::size_t i = 0; i < capacity; ++i) {
        map->buckets[i].key = nullptr;
        map->buckets[i].value = LSP_METHOD_COUNT;
    }
}

static inline bool lsp_method_map_put(lsp_method_map* map, const char* key, lsp_method_type value) noexcept {
    std::size_t cap = map->capacity;
    if (cap == 0) { return false; }
    std::uint64_t h = lsp_fnv1a64(key);
    std::size_t idx = (std::size_t)(h % cap);
    for (std::size_t probe = 0; probe < cap; ++probe) {
        std::size_t i = (idx + probe) % cap;
        const char* existing = map->buckets[i].key;
        if (existing == nullptr) {
            map->buckets[i].key = key;
            map->buckets[i].value = value;
            return true;
        }
        if (std::strcmp(existing, key) == 0) {
            map->buckets[i].value = value;
            return true;
        }
    }
    return false;
}

static inline lsp_method_type lsp_method_map_get(const lsp_method_map* map, const char* key) noexcept {
    std::size_t cap = map->capacity;
    if (cap == 0) { return LSP_METHOD_COUNT; }
    std::uint64_t h = lsp_fnv1a64(key);
    std::size_t idx = (std::size_t)(h % cap);
    for (std::size_t probe = 0; probe < cap; ++probe) {
        std::size_t i = (idx + probe) % cap;
        const char* k = map->buckets[i].key;
        if (k == nullptr) {
            return LSP_METHOD_COUNT;
        }
        if (std::strcmp(k, key) == 0) {
            return map->buckets[i].value;
        }
    }
    return LSP_METHOD_COUNT;
}

// Two direction-specific maps
struct lsp_methods_directory {
    lsp_method_map c2s; // client -> server
    lsp_method_map s2c; // server -> client
};

// Initialize both maps with provided storage
static inline void lsp_methods_directory_init(
    lsp_methods_directory* dir,
    lsp_method_map_entry* c2s_storage, std::size_t c2s_cap,
    lsp_method_map_entry* s2c_storage, std::size_t s2c_cap
) noexcept {
    lsp_method_map_init(&dir->c2s, c2s_storage, c2s_cap);
    lsp_method_map_init(&dir->s2c, s2c_storage, s2c_cap);
}

// Build the C2S map. Storage will be allocated with 2x capacity via ak_alloc_mem.
static inline bool lsp_methods_build_c2s(lsp_methods_directory* dir) noexcept {
    // conservative number of entries; grow as needed
    const std::size_t n = 64; // placeholder sized; map is open addressing
    const std::size_t cap = n * 2;
    auto* storage = (lsp_method_map_entry*) ak_alloc_mem(sizeof(lsp_method_map_entry) * cap);
    if (!storage) { return false; }
    lsp_method_map_init(&dir->c2s, storage, cap);

    // Insert methods with messageDirection clientToServer (and both for $/progress)
    lsp_method_map_put(&dir->c2s, "textDocument/implementation", LSP_METHOD_TEXT_DOCUMENT_IMPLEMENTATION);
    lsp_method_map_put(&dir->c2s, "textDocument/typeDefinition", LSP_METHOD_TEXT_DOCUMENT_TYPE_DEFINITION);
    lsp_method_map_put(&dir->c2s, "textDocument/documentColor", LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_COLOR);
    lsp_method_map_put(&dir->c2s, "textDocument/colorPresentation", LSP_METHOD_TEXT_DOCUMENT_COLOR_PRESENTATION);
    lsp_method_map_put(&dir->c2s, "textDocument/foldingRange", LSP_METHOD_TEXT_DOCUMENT_FOLDING_RANGE);
    lsp_method_map_put(&dir->c2s, "textDocument/declaration", LSP_METHOD_TEXT_DOCUMENT_DECLARATION);
    lsp_method_map_put(&dir->c2s, "textDocument/selectionRange", LSP_METHOD_TEXT_DOCUMENT_SELECTION_RANGE);
    lsp_method_map_put(&dir->c2s, "textDocument/prepareCallHierarchy", LSP_METHOD_TEXT_DOCUMENT_PREPARE_CALL_HIERARCHY);
    lsp_method_map_put(&dir->c2s, "callHierarchy/incomingCalls", LSP_METHOD_CALL_HIERARCHY_INCOMING_CALLS);
    lsp_method_map_put(&dir->c2s, "callHierarchy/outgoingCalls", LSP_METHOD_CALL_HIERARCHY_OUTGOING_CALLS);
    lsp_method_map_put(&dir->c2s, "textDocument/semanticTokens/full", LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_FULL);
    lsp_method_map_put(&dir->c2s, "textDocument/semanticTokens/full/delta", LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_FULL_DELTA);
    lsp_method_map_put(&dir->c2s, "textDocument/semanticTokens/range", LSP_METHOD_TEXT_DOCUMENT_SEMANTIC_TOKENS_RANGE);
    lsp_method_map_put(&dir->c2s, "textDocument/linkedEditingRange", LSP_METHOD_TEXT_DOCUMENT_LINKED_EDITING_RANGE);
    lsp_method_map_put(&dir->c2s, "workspace/willCreateFiles", LSP_METHOD_WORKSPACE_WILL_CREATE_FILES);
    lsp_method_map_put(&dir->c2s, "workspace/willRenameFiles", LSP_METHOD_WORKSPACE_WILL_RENAME_FILES);
    lsp_method_map_put(&dir->c2s, "workspace/willDeleteFiles", LSP_METHOD_WORKSPACE_WILL_DELETE_FILES);
    lsp_method_map_put(&dir->c2s, "textDocument/moniker", LSP_METHOD_TEXT_DOCUMENT_MONIKER);
    lsp_method_map_put(&dir->c2s, "textDocument/prepareTypeHierarchy", LSP_METHOD_TEXT_DOCUMENT_PREPARE_TYPE_HIERARCHY);
    lsp_method_map_put(&dir->c2s, "typeHierarchy/supertypes", LSP_METHOD_TYPE_HIERARCHY_SUPERTYPES);
    lsp_method_map_put(&dir->c2s, "typeHierarchy/subtypes", LSP_METHOD_TYPE_HIERARCHY_SUBTYPES);
    lsp_method_map_put(&dir->c2s, "textDocument/inlineValue", LSP_METHOD_TEXT_DOCUMENT_INLINE_VALUE);
    lsp_method_map_put(&dir->c2s, "textDocument/inlineCompletion", LSP_METHOD_TEXT_DOCUMENT_INLINE_COMPLETION);
    lsp_method_map_put(&dir->c2s, "textDocument/inlayHint", LSP_METHOD_TEXT_DOCUMENT_INLAY_HINT);
    lsp_method_map_put(&dir->c2s, "inlayHint/resolve", LSP_METHOD_INLAY_HINT_RESOLVE);
    lsp_method_map_put(&dir->c2s, "textDocument/diagnostic", LSP_METHOD_TEXT_DOCUMENT_DIAGNOSTIC);
    lsp_method_map_put(&dir->c2s, "workspace/diagnostic", LSP_METHOD_WORKSPACE_DIAGNOSTIC);
    lsp_method_map_put(&dir->c2s, "initialize", LSP_METHOD_INITIALIZE);
    lsp_method_map_put(&dir->c2s, "shutdown", LSP_METHOD_SHUTDOWN);
    lsp_method_map_put(&dir->c2s, "textDocument/willSaveWaitUntil", LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE_WAIT_UNTIL);
    lsp_method_map_put(&dir->c2s, "textDocument/completion", LSP_METHOD_TEXT_DOCUMENT_COMPLETION);
    lsp_method_map_put(&dir->c2s, "textDocument/hover", LSP_METHOD_TEXT_DOCUMENT_HOVER);
    lsp_method_map_put(&dir->c2s, "textDocument/signatureHelp", LSP_METHOD_TEXT_DOCUMENT_SIGNATURE_HELP);
    lsp_method_map_put(&dir->c2s, "textDocument/definition", LSP_METHOD_TEXT_DOCUMENT_DEFINITION);
    lsp_method_map_put(&dir->c2s, "textDocument/references", LSP_METHOD_TEXT_DOCUMENT_REFERENCES);
    lsp_method_map_put(&dir->c2s, "textDocument/documentHighlight", LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_HIGHLIGHT);
    lsp_method_map_put(&dir->c2s, "textDocument/documentSymbol", LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_SYMBOL);
    lsp_method_map_put(&dir->c2s, "textDocument/codeAction", LSP_METHOD_TEXT_DOCUMENT_CODE_ACTION);
    lsp_method_map_put(&dir->c2s, "textDocument/codeLens", LSP_METHOD_TEXT_DOCUMENT_CODE_LENS);
    lsp_method_map_put(&dir->c2s, "textDocument/documentLink", LSP_METHOD_TEXT_DOCUMENT_DOCUMENT_LINK);
    lsp_method_map_put(&dir->c2s, "textDocument/formatting", LSP_METHOD_TEXT_DOCUMENT_FORMATTING);
    lsp_method_map_put(&dir->c2s, "textDocument/rangeFormatting", LSP_METHOD_TEXT_DOCUMENT_RANGE_FORMATTING);
    lsp_method_map_put(&dir->c2s, "textDocument/rangesFormatting", LSP_METHOD_TEXT_DOCUMENT_RANGES_FORMATTING);
    lsp_method_map_put(&dir->c2s, "textDocument/onTypeFormatting", LSP_METHOD_TEXT_DOCUMENT_ON_TYPE_FORMATTING);
    lsp_method_map_put(&dir->c2s, "textDocument/rename", LSP_METHOD_TEXT_DOCUMENT_RENAME);
    lsp_method_map_put(&dir->c2s, "textDocument/prepareRename", LSP_METHOD_TEXT_DOCUMENT_PREPARE_RENAME);
    lsp_method_map_put(&dir->c2s, "workspace/executeCommand", LSP_METHOD_WORKSPACE_EXECUTE_COMMAND);
    lsp_method_map_put(&dir->c2s, "workspace/didChangeWorkspaceFolders", LSP_METHOD_WORKSPACE_DID_CHANGE_WORKSPACE_FOLDERS);
    lsp_method_map_put(&dir->c2s, "window/workDoneProgress/cancel", LSP_METHOD_WINDOW_WORK_DONE_PROGRESS_CANCEL);
    lsp_method_map_put(&dir->c2s, "workspace/didCreateFiles", LSP_METHOD_WORKSPACE_DID_CREATE_FILES);
    lsp_method_map_put(&dir->c2s, "workspace/didRenameFiles", LSP_METHOD_WORKSPACE_DID_RENAME_FILES);
    lsp_method_map_put(&dir->c2s, "workspace/didDeleteFiles", LSP_METHOD_WORKSPACE_DID_DELETE_FILES);
    lsp_method_map_put(&dir->c2s, "notebookDocument/didOpen", LSP_METHOD_NOTEBOOK_DOCUMENT_DID_OPEN);
    lsp_method_map_put(&dir->c2s, "notebookDocument/didChange", LSP_METHOD_NOTEBOOK_DOCUMENT_DID_CHANGE);
    lsp_method_map_put(&dir->c2s, "notebookDocument/didSave", LSP_METHOD_NOTEBOOK_DOCUMENT_DID_SAVE);
    lsp_method_map_put(&dir->c2s, "notebookDocument/didClose", LSP_METHOD_NOTEBOOK_DOCUMENT_DID_CLOSE);
    lsp_method_map_put(&dir->c2s, "initialized", LSP_METHOD_INITIALIZED);
    lsp_method_map_put(&dir->c2s, "exit", LSP_METHOD_EXIT);
    lsp_method_map_put(&dir->c2s, "workspace/didChangeConfiguration", LSP_METHOD_WORKSPACE_DID_CHANGE_CONFIGURATION);
    lsp_method_map_put(&dir->c2s, "textDocument/didOpen", LSP_METHOD_TEXT_DOCUMENT_DID_OPEN);
    lsp_method_map_put(&dir->c2s, "textDocument/didChange", LSP_METHOD_TEXT_DOCUMENT_DID_CHANGE);
    lsp_method_map_put(&dir->c2s, "textDocument/didClose", LSP_METHOD_TEXT_DOCUMENT_DID_CLOSE);
    lsp_method_map_put(&dir->c2s, "textDocument/didSave", LSP_METHOD_TEXT_DOCUMENT_DID_SAVE);
    lsp_method_map_put(&dir->c2s, "textDocument/willSave", LSP_METHOD_TEXT_DOCUMENT_WILL_SAVE);
    lsp_method_map_put(&dir->c2s, "workspace/didChangeWatchedFiles", LSP_METHOD_WORKSPACE_DID_CHANGE_WATCHED_FILES);
    // $/progress is both
    lsp_method_map_put(&dir->c2s, "$/progress", LSP_METHOD_PROGRESS_NOTIFICATION);
    // $/setTrace is client->server
    lsp_method_map_put(&dir->c2s, "$/setTrace", LSP_METHOD_SET_TRACE_NOTIFICATION);
    // $/cancelRequest is client->server
    lsp_method_map_put(&dir->c2s, "$/cancelRequest", LSP_METHOD_CANCEL_REQUEST_NOTIFICATION);
    return true;
}

// Build the S2C map. Storage will be allocated with 2x capacity via ak_alloc_mem.
static inline bool lsp_methods_build_s2c(lsp_methods_directory* dir) noexcept {
    const std::size_t n = 48; // placeholder
    const std::size_t cap = n * 2;
    auto* storage = (lsp_method_map_entry*) ak_alloc_mem(sizeof(lsp_method_map_entry) * cap);
    if (!storage) { return false; }
    lsp_method_map_init(&dir->s2c, storage, cap);

    lsp_method_map_put(&dir->s2c, "workspace/workspaceFolders", LSP_METHOD_WORKSPACE_WORKSPACE_FOLDERS);
    lsp_method_map_put(&dir->s2c, "workspace/configuration", LSP_METHOD_WORKSPACE_CONFIGURATION);
    lsp_method_map_put(&dir->s2c, "workspace/foldingRange/refresh", LSP_METHOD_WORKSPACE_FOLDING_RANGE_REFRESH);
    lsp_method_map_put(&dir->s2c, "window/workDoneProgress/create", LSP_METHOD_WINDOW_WORK_DONE_PROGRESS_CREATE);
    lsp_method_map_put(&dir->s2c, "workspace/semanticTokens/refresh", LSP_METHOD_WORKSPACE_SEMANTIC_TOKENS_REFRESH);
    lsp_method_map_put(&dir->s2c, "window/showDocument", LSP_METHOD_WINDOW_SHOW_DOCUMENT);
    lsp_method_map_put(&dir->s2c, "workspace/inlineValue/refresh", LSP_METHOD_WORKSPACE_INLINE_VALUE_REFRESH);
    lsp_method_map_put(&dir->s2c, "workspace/inlayHint/refresh", LSP_METHOD_WORKSPACE_INLAY_HINT_REFRESH);
    lsp_method_map_put(&dir->s2c, "workspace/diagnostic/refresh", LSP_METHOD_WORKSPACE_DIAGNOSTIC_REFRESH);
    lsp_method_map_put(&dir->s2c, "client/registerCapability", LSP_METHOD_CLIENT_REGISTER_CAPABILITY);
    lsp_method_map_put(&dir->s2c, "client/unregisterCapability", LSP_METHOD_CLIENT_UNREGISTER_CAPABILITY);
    lsp_method_map_put(&dir->s2c, "window/showMessageRequest", LSP_METHOD_WINDOW_SHOW_MESSAGE_REQUEST);
    lsp_method_map_put(&dir->s2c, "workspace/applyEdit", LSP_METHOD_WORKSPACE_APPLY_EDIT);
    lsp_method_map_put(&dir->s2c, "window/showMessage", LSP_METHOD_WINDOW_SHOW_MESSAGE);
    lsp_method_map_put(&dir->s2c, "window/logMessage", LSP_METHOD_WINDOW_LOG_MESSAGE);
    lsp_method_map_put(&dir->s2c, "telemetry/event", LSP_METHOD_TELEMETRY_EVENT);
    lsp_method_map_put(&dir->s2c, "textDocument/publishDiagnostics", LSP_METHOD_TEXT_DOCUMENT_PUBLISH_DIAGNOSTICS);
    // $/progress is both
    lsp_method_map_put(&dir->s2c, "$/progress", LSP_METHOD_PROGRESS_NOTIFICATION);
    // $/logTrace is server->client
    lsp_method_map_put(&dir->s2c, "$/logTrace", LSP_METHOD_LOG_TRACE_NOTIFICATION);
    return true;
}


