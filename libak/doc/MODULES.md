# libak modules overview

This document summarizes how modules in `libak/src/ak` are structured and how they depend on each other.

- **ak/base**: foundational types, error handling, platform bindings (e.g., `liburing`), and small utilities like timers and versioning. Public API: `base_api.hpp` (+ inline in `base_api_inl.hpp`).
- **ak/alloc**: memory allocator primitives built on top of `ak/base`. Public API: `alloc_api.hpp` (+ inline and private headers). Umbrella header `alloc.hpp` aggregates the allocator APIs.
- **ak/runtime**: coroutine-based runtime built on top of `ak/base` and `ak/alloc`, integrates with `io_uring`. Public API: `runtime_api.hpp`, aggregated by `runtime.hpp`.
- **ak/sync**: synchronization primitives that rely on the runtime. Public API: `sync_api.hpp`, aggregated by `sync.hpp`.
- **ak/json**: RFC8259-compliant JSON parser and API using base facilities. Public API: `json_api.hpp`, aggregated by `json.hpp`.
- **ak/storage**: storage utilities (frame table, page cache) that depend on `ak/base` and `ak/alloc`. Public API: `storage_api.hpp`, aggregated by `storage.hpp`.
- **ak/lspd**: example/demo program (“light-speed daemon”) using the high-level umbrella `ak.hpp` and `ak/base`. Not a core library module.

At the root, `ak.hpp` is the umbrella header that includes public APIs of all core modules for convenience.

## Dependency rules (from include graph)

- Base is foundational: no internal module depends beneath it.
- Alloc depends on Base.
- Runtime depends on Base and Alloc.
- Sync depends on Runtime.
- JSON depends on Base.
- Storage depends on Base and Alloc.
- lspd depends on the umbrella `ak.hpp` and Base.

## Dependency diagram

```mermaid
%%{init: {'theme':'base','themeVariables': {'primaryColor':'#ffffff','primaryTextColor':'#0b0b0b','primaryBorderColor':'#0b0b0b','lineColor':'#0b0b0b','arrowheadColor':'#0b0b0b','fontFamily':'ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, Noto Sans, Helvetica Neue, Arial'}}}%%
graph LR
  %% Background panel wrapping the entire diagram
  subgraph Language Server
    direction LR

    subgraph Core["Application Kernel"]
      Base["ak/base"]
      Alloc["ak/alloc"]
      Runtime["ak/runtime"]
      Sync["ak/sync"]
      JSON["ak/json"]
      Storage["ak/storage"]
      Umb["ak.hpp"]
    end

    LSPD["ak/lspd
    [Language Server]
    "]
    

    Base --> Alloc
    Base --> Runtime
    Base --> JSON
    Base --> Storage

    Alloc --> Runtime
    Alloc --> Storage

    Runtime --> Sync

    %% Umbrella pulls public APIs from all core modules
    Alloc --> Umb
    Base --> Umb
    Runtime --> Umb
    Sync --> Umb
    JSON --> Umb
    Storage --> Umb
    Umb --> LSPD

    %% High-contrast arrows
    linkStyle default stroke:#0b0b0b,stroke-width:2.0px;

    %% Assign classes for higher-contrast fills/borders per module
    class Base base;
    class Alloc alloc;
    class Runtime runtime;
    class Sync sync;
    class JSON json;
    class Storage storage;
    class Umb umbrella;
    class LSPD example;

  end

  %% Style the background panel
  %% style BG fill:#f3f4f6,stroke:#9ca3af,stroke-width:1.5px

  %% Assign classes for higher-contrast fills/borders per module
  %% Class definitions
  classDef base fill:#ffffff,stroke:#0b0b0b,stroke-width:1.5px,color:#0b0b0b;
  classDef alloc fill:#dbeafe,stroke:#1e3a8a,stroke-width:1.5px,color:#0b0b0b;
  classDef runtime fill:#ede9fe,stroke:#4c1d95,stroke-width:1.5px,color:#0b0b0b;
  classDef sync fill:#ccfbf1,stroke:#065f46,stroke-width:1.5px,color:#0b0b0b;
  classDef json fill:#fef3c7,stroke:#92400e,stroke-width:1.5px,color:#0b0b0b;
  classDef storage fill:#dcfce7,stroke:#14532d,stroke-width:1.5px,color:#0b0b0b;
  classDef umbrella fill:#e5e7eb,stroke:#111827,stroke-width:1.5px,color:#111827;
  classDef example fill:#fee2e2,stroke:#7f1d1d,stroke-width:1.5px,color:#111827;
```

## Notes

- Public headers follow the `*_api.hpp` convention with optional inline headers `*_api_inl.hpp`. Aggregator headers like `alloc.hpp`, `runtime.hpp`, `json.hpp`, `storage.hpp`, and `sync.hpp` re-export their respective public/private APIs for internal use and convenience.
- External dependencies observed: `liburing` and `argtable3` (used in `lspd`).
- JSON implementation is designed to comply with RFC 8259.
