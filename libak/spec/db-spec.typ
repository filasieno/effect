= ARIES Description and Page Records

== Reliability

"if you skip reliability for performance

ARIES (Algorithms for Recovery and Isolation Exploiting Semantics) is a robust database recovery mechanism that ensures ACID properties through write-ahead logging (WAL). It supports fine-grained locking, operation logging, and efficient crash recovery via three phases: Analysis (rebuild state from checkpoint), Redo (reapply committed changes), and Undo (rollback uncommitted changes). Key features include:

- *Write-Ahead Logging*: Changes are logged before being applied to disk.
- *Log Sequence Numbers (LSNs)*: Monotonic identifiers for log records.
- *Checkpoints*: Periodic snapshots to speed up recovery.
- *Dirty Page Table*: Tracks pages modified since last checkpoint.
- *Transaction Table*: Tracks active transactions.

== Physical files

- *.wkspc* - the main database file that represents a workspace
- *.wkidx* - the index file is associated with a specific veriosn of the `wkspc` file; it doesnt match it fan be rebuild; its there for efficiency and it's not transactional.
- *.wklog* - the log file is used to store the changes to the database; it's used as WAL to recover the database in case of a crash and it associated to a `wspsc`

== Records Required for ARIES in Pages

To implement ARIES, each database page (e.g., in db.data) must include these fields for recovery tracking:

- *Page LSN (uint64)*: The LSN of the most recent log record that modified this page. Used during redo to determine if a page needs replaying (if Page LSN < log record LSN).
- *Page ID (uint64)*: Unique identifier for the page, referenced in log records for physical redo/undo.
- *Transaction ID (uint64)*: ID of the last transaction that modified the page, for undo operations.
- *Checksum (uint64)*: For detecting corruption; computed over the page contents excluding ARIES fields.

These are placed in a fixed header (e.g., first 32-64 bytes of each 4KB page).

== Other Essential Records for Reliability

Beyond ARIES, include these common records in pages for enhanced reliability and error handling:

- *Dirty Flag (uint8)*: Indicates if the page has unsynced changes (1 = dirty, 0 = clean). Helps in checkpointing.
- *Recovery Flag (uint8)*: Marks if the page is in a recovery state (e.g., being redone).
- *Page Type (uint8)*: Indicate the type of the page, e.g., data page, index page, etc.
- *Version Number (uint32)*: Page format version for backward compatibility.
- *Timestamp (uint64)*: Last modification time, for auditing or consistency checks.
- *Redundant Checksum (uint64)*: A secondary checksum (e.g., CRC32) for paranoia-level verification.
- *Padding/Reserved (variable)*: Zero-filled space for future extensions.

These ensure durability, detect errors, and support features like replication or backups.

== System Design Overview

The database is designed for an interactive compiler workload, supporting notebooks with cells that define or use languages, enabling literate programming where code is built in chunks. It handles heavy writes for small files (e.g., cells as files), assembling notebooks, grammar definitions, AST/CAST files, scopegraphs, and semantic tokens. Workloads include PIE-like build chains, StrategoXT transformations, partial evaluation, and JIT compilation.

=== Core Design Principles
- *Log-Structured Storage*: All data (file contents, directories) is append-only, inspired by SpriteFS. Changes create new versions without overwriting, ensuring immutability and easy versioning (foundation for Git-like features).
- *Bf-Tree Indexing*: Hybrid Bf-tree for read-optimized queries on metadata (e.g., file lookups, scopegraphs) based on mini-pages. Buffered nodes allow fast inserts; merges handle write amplification.
- *File and Notebook Model*:
  - *Cells as Files*: Each cell is a small, log-structured file with chunks for literate programming.
  - *Notebooks*: Assemble cells via pointers; changes append new cell versions.
  - *Languages and Grammars*: Metalanguage cells define grammars; used immediately in other cells via references.
  - *Support Files*: AST/CAST, scopegraphs, semantic tokens stored as small, indexed logs.
- *Execution Workload*: PIE build chains trigger incremental recompiles; StrategoXT for transformations; partial eval/JIT output as new log entries.
- *Reliability (No Data Loss)*: ARIES for recovery; all ops transactional. Versioning ensures history is preserved, aiming to replace Git with built-in branching/merging.

=== Integration with ARIES

ARIES ensures no work is lost: WAL logs all changes; checkpoints every few MB; recovery < 1s for GB-scale logs using coroutines.

=== Challenges to Solve

Implementing this design involves addressing:

- *Heavy Small-File Writes*: Log-structured appends minimize I/O, but need efficient coalescing to avoid fragmentation. 
  _Challenge_: Balancing write amplification with read performance in Bf-Tree. 
  _Possible solution_: Use a Hybrid SpritFS/Bf-tree approah to efficiently store large trees of files.
- *Assembly and Dependencies*: Notebooks/cells with interdependencies (e.g., language defs used in same file). 
  _Challenge_: Incremental updates without full recompiles; 
  _Possible solution_: use PIE for dependency tracking.
- *Literate Programming Chunks*: Code in non-linear chunks. 
  _Challenge_: Efficient assembly into executable units; log-structured merging with versioning.
  _Possible solution_: log-structured merging with versioning
- *Scalability for Small Files*: Thousands of tiny files (ASTs, tokens). 
  _Challenge_: Metadata overhead; use compact Bf-Tree leaves and batching.
  _Possible solution_: Use a Hybrid SpritFS/Bf-tree approah to efficiently store large trees of files.
- *Build Chain Execution*: PIE/StrategoXT/partial eval/JIT. 
  _Challenge_: Integrating with log-structured storage; ensure transformations are atomic and recoverable.
- *No Data Loss (Git Replacement)*: ARIES provides recovery, but need immutable history. 
  _Challenge_: Implementing branching/merging without Git; handle conflicts, large histories without bloat.
- *Performance*: Interactive (low-latency) despite logs. Challenge: Caching, prefetching, and async compaction.
- *Concurrency*: Multi-user LSP edits. 
  _Challenge_: Fine-grained locking in ARIES without blocking.
- *Extensibility*: Adding new lang features. 
  _Challenge_: Dynamic schema updates in log-structured system.


=== Notes on Entities
- *Notebook*: Assemblies of cells; links to Cell via IDs.
- *Cell*: Individual units (code/metalang); references Language/Grammar.
- *File*: Generic for all small files (temporary/permanent); points to Log_Entry chunks.
- *Language*: Defines metalanguages or user langs.
- *Grammar*: Stores grammar specs (e.g., SDF3 for StrategoXT).
- *AST_Node*: Generic tree nodes for AST/CAST; supports StrategoXT signatures (e.g., constructor, children, annotations).
- *Scopegraph node*: For name resolution; edges to other nodes.
- *Semantic token*: Tokens with types/positions.
- *Catalog entry*: Indexes files with metadata (path, type).
- *Version*: Tracks file versions (immutable snapshots).
- *Log entry*: Append-only chunks; includes ARIES fields for recovery.

This covers log-structured storage, cataloging, and AST support for StrategoXT.

=== Entities for LSP Support (Relational Design)

Following constraints: Data in two containers—log-structured (SumTree for arrays, HAMT for hashes, ART for sorted) or relational tables (heaps, sorted/hashed sets/maps). Tables avoid direct JSON/arrays/maps, using normalized schemas or references to log-structured files. User-modified files use persistent PieceTables. Maximized cache locality via clustering (e.g., by document_id) for related data.

Deep analysis of LSP 3.17 metamodel: Flattened complex JSON structures (e.g., Capabilities) into relational tables with foreign keys. Log-structured for dynamic/mutable data (e.g., document content, token arrays). Relational for static/metadata (e.g., client info). For messages like Initialize (params: InitializeParams with capabilities), store flattened; for DidOpen (params: DidOpenTextDocumentParams with TextDocumentItem), reference PieceTable for content.

- *LSP_Client* (Relational table, clustered by client_id for locality with related workspaces/requests):
  - client_id: uint64 (PK)
  - name: string
  - trace_level: enum (off, messages, verbose)
  - connected_at: timestamp
  - capabilities_ref: ref(Log_Structured_File) // HAMT reference for ClientCapabilities (e.g., textDocument.sync as key-value)
  - init_options_ref: ref(Log_Structured_File) // HAMT for arbitrary options

- *LSP_Server_Capabilities* (Relational table, clustered by server_id):
  - server_id: uint64 (PK)
  - name: string
  - version: string
  - capabilities_ref: ref(Log_Structured_File) // HAMT for ServerCapabilities (e.g., completionProvider as sub-maps)

- *Workspace* (Relational table, clustered by workspace_id; folders as SumTree reference for array-like structure):
  - workspace_id: uint64 (PK)
  - client_id: uint64 (FK to LSP_Client)
  - folders_ref: ref(Log_Structured_File) // SumTree of WorkspaceFolder (each node: uri, name)
  - symbols_ref: ref(Log_Structured_File) // ART sorted by symbol name

- *Document* (Relational table, clustered by document_id for locality with diagnostics/tokens/AST):
  - document_id: uint64 (PK)
  - uri: string
  - language_id: string
  - version: int
  - content_ref: ref(Log_Structured_File) // Persistent PieceTable for text content
  - open_state: enum (open, changed, closed)

- *Document_Diagnostic* (Relational table, clustered by document_id; ranges as references to SumTree for positions):
  - diagnostic_id: uint64 (PK)
  - document_id: uint64 (FK to Document)
  - severity: enum
  - code: string
  - message: string
  - source: string
  - range_ref: ref(Log_Structured_File) // SumTree for start/end positions (line, char)
  - related_info_ref: ref(Log_Structured_File) // SumTree of related diagnostics

- *Document_Semantic_Token* (Relational table, clustered by document_id; data as SumTree for delta-encoded array):
  - token_id: uint64 (PK)
  - document_id: uint64 (FK to Document)
  - data_ref: ref(Log_Structured_File) // SumTree of uint32 deltas
  - legend_ref: ref(Log_Structured_File) // HAMT for tokenTypes/modifiers arrays

- *Workspace_Symbol* (Relational table, clustered by workspace_id; location as SumTree reference):
  - symbol_id: uint64 (PK)
  - workspace_id: uint64 (FK to Workspace)
  - name: string
  - kind: enum
  - container_name: string
  - location_ref: ref(Log_Structured_File) // SumTree for uri/range

- *LSP_Request* (Relational table, clustered by request_id for in-progress tracking):
  - request_id: uint64 (PK) // Or string if needed
  - client_id: uint64 (FK to LSP_Client)
  - method: string
  - status: enum
  - started_at: timestamp
  - completed_at: timestamp
  - params_ref: ref(Log_Structured_File) // HAMT for request params (e.g., CompletionParams)
  - response_ref: ref(Log_Structured_File) // HAMT for result/error

- *Capability_Registration* (Relational table, clustered by registration_id):
  - registration_id: string (PK)
  - method: string
  - options_ref: ref(Log_Structured_File) // HAMT for registerOptions (e.g., documentSelector as array)

*Storage Decisions*:
- Relational for metadata/static data (fast queries, low mutability).
- Log-structured for dynamic content (e.g., PieceTable for documents to handle edits efficiently).
- Cache locality: Clustering by IDs groups related rows; log-structures use locality-optimized trees (e.g., SumTree for sequential access).

This normalizes LSP metamodel, avoids embedded complex types, and ensures efficiency.

=== ECS-Style Grouping of Attributes (Redesigned for Cache Locality)

Redesigned based on LSP operations (e.g., didChange: update document state/analysis; publishDiagnostics: fetch all per-document diagnostics; completion: access local AST/tokens). Maximized locality by clustering data accessed together (e.g., all diagnostics for a document stored contiguously). Used per-entity ID pools. Components as clustered tables (for relational) or dedicated log files (for mutable data), deviating from SumTree for better sequential access in operations like diagnostics publishing.

==== Entities and ID Pools

- Notebook (notebook_id: uint64 pool).
- Cell (cell_id: uint64 pool).
- File (file_id: uint64 pool).
- Language (language_id: uint64 pool).
- Grammar (grammar_id: uint64 pool).
- AST_Node (ast_node_id: uint64 pool).
- Scopegraph_Node (scopegraph_node_id: uint64 pool).
- Semantic_Token (semantic_token_id: uint64 pool).
- Catalog_Entry (catalog_entry_id: uint64 pool).
- Version (version_id: uint64 pool).
- Log_Entry (log_entry_id: uint64 pool).
- LSP_Client (client_id: uint64 pool).
- LSP_Server_Capabilities (server_caps_id: uint64 pool).
- Workspace (workspace_id: uint64 pool).
- Document (document_id: uint64 pool).
- Diagnostic (diagnostic_id: uint64 pool).
- Workspace_Symbol (symbol_id: uint64 pool).
- LSP_Request (request_id: uint64 pool).
- Capability_Registration (registration_id: uint64 pool).

==== Components (Optimized Layout)

- *Client_Core_Component* (Clustered table by client_id; for initialize: quick fetch):
  - client_id: uint64 (PK, clustered)
  - name: string
  - connected_at: timestamp

- *Client_State_Component* (Clustered table by client_id; for ongoing sessions):
  - client_id: uint64 (PK)
  - trace_level: enum

- *Capability_Component* (Dedicated log file per entity_id; for initialize/response: sequential read of capabilities):
  - entity_id: uint64 (clustered)
  - cap_type: string
  - value: ref(Log_Structured_File) // Details

- *Workspace_Core_Component* (Clustered table by workspace_id; for workspace ops):
  - workspace_id: uint64 (PK)
  - client_id: uint64

- *Workspace_Reference_Component* (Clustered table by workspace_id; for symbol queries: indexed by ref_type):
  - workspace_id: uint64 (clustered)
  - ref_type: enum
  - ref_id: uint64

- *Document_Core_Component* (Clustered table by document_id; for didOpen: fast metadata load):
  - document_id: uint64 (PK)
  - uri: string
  - language_id: string
  - version: int

- *Document_State_Component* (Clustered table by document_id; for didChange: quick updates):
  - document_id: uint64 (PK)
  - open_state: enum
  - content_ref: ref(Log_Structured_File) // PieceTable

- *Document_Analysis_Component* (Dedicated log file per document_id; for analysis ops: append-only updates):
  - document_id: uint64 (clustered)
  - analysis_type: enum
  - ref: ref(Log_Structured_File)

- *Diagnostic_Component* (Clustered table by document_id; for publishDiagnostics: sequential scan of all for a document):
  - diagnostic_id: uint64 (PK)
  - document_id: uint64 (clustered)
  - severity: enum
  - code: string
  - message: string
  - source: string
  - range_start_line: uint32
  - range_start_char: uint32
  - range_end_line: uint32
  - range_end_char: uint32
  - related_message: string

- *Semantic_Token_Component* (Dedicated log file per document_id; for semanticTokens/full: delta-encoded sequential access):
  - semantic_token_id: uint64 (PK)
  - document_id: uint64 (clustered)
  - delta_line: uint32
  - delta_start: uint32
  - length: uint32
  - token_type: uint32
  - token_modifiers: uint32

- *Symbol_Component* (Clustered table by workspace_id; for workspace/symbol: indexed by name):
  - symbol_id: uint64 (PK)
  - workspace_id: uint64 (clustered)
  - name: string
  - kind: enum
  - container_name: string
  - location_uri: string
  - location_start_line: uint32
  - location_start_char: uint32
  - location_end_line: uint32
  - location_end_char: uint32

- *Request_Component* (Clustered table by client_id then request_id; for request handling: group by client):
  - request_id: uint64 (PK)
  - client_id: uint64 (clustered)
  - method: string
  - status: enum
  - started_at: timestamp
  - completed_at: timestamp
  - param_key: string
  - param_value: string

- *Registration_Component* (Clustered table by registration_id; for registerCapability: quick lookup):
  - registration_id: uint64 (PK)
  - method: string
  - option_key: string
  - option_value: string

This layout ensures operations like didChange (update document cluster) or publishDiagnostics (scan document's diagnostics table) hit localized data, minimizing cache misses.
