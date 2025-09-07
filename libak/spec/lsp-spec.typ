= Language Server Protocol (LSP) Specification

This document outlines the minimal set of notifications, requests, and responses required to implement a fully featured LSP server. The server supports notebook and text document incremental synchronization, semantic tokens for a 'grammar' language, all possible requests from server to client, and required methods for initialization and shutdown. The goal is to create a skeleton server with full protocol support (including logging and telemetry) but no actual language-specific features yet.

== Server Capabilities

The server will advertise the following capabilities in the `initialize` response:

- *Text Document Synchronization*: Incremental (kind: 2)
- *Notebook Document Synchronization*: Enabled with incremental cell changes
- *Semantic Tokens*: Full, range, and delta support with custom legend for 'grammar' language
  - Token Types: namespace, type, class, enum, interface, struct, typeParameter, parameter, variable, property, enumMember, event, function, method, macro, keyword, modifier, comment, string, number, regexp, operator, nonterminal, terminal, production, rule
  - Token Modifiers: declaration, definition, readonly, static, deprecated, abstract, async, modification, documentation, defaultLibrary
- *Workspace*: Full support including folders, edits, symbols, commands, file operations, and configuration
  - workspaceFolders: supported, changeNotifications
  - symbol: dynamicRegistration, symbol kinds (full set)
  - executeCommand: dynamicRegistration
  - applyEdit: true
  - workspaceEdit: documentChanges, resourceOperations (create/rename/delete)
  - didChangeConfiguration: dynamicRegistration
  - didChangeWatchedFiles: dynamicRegistration
  - configuration: true
  - fileOperations: willCreate/willRename/willDelete/didCreate/didRename/didDelete with patterns
- *Diagnostic*: Pull model support with relatedInformation, tagSupport, versionSupport
- *Progress*: true (for workDoneProgress)
- *CancelRequest*: true (via \$/cancelRequest)
- *Other*: dynamicRegistration for capabilities, partialResult support where applicable
- *Notebook Document* (3.18+): semanticTokens (full/range/delta with legend), diagnostic pull support

== Initialization and Shutdown

These are required for LSP handshake and termination.

- *Request: initialize* (Client → Server)
  - Params: InitializeParams (processId, rootUri, capabilities, etc.)
  - Response: InitializeResult (capabilities as above)

- *Notification: initialized* (Client → Server)
  - Params: InitializedParams (empty)

- *Request: shutdown* (Client → Server)
  - Params: null
  - Response: null

- *Notification: exit* (Client → Server)
  - Params: null

== Text Document Synchronization (Incremental)

Supports incremental sync for text documents.

- *Notification: textDocument/didOpen* (Client → Server)
  - Params: DidOpenTextDocumentParams (textDocument: uri, languageId, version, text)

- *Notification: textDocument/didChange* (Client → Server)
  - Params: DidChangeTextDocumentParams (textDocument: versioned identifier, contentChanges: array of incremental changes)

- *Notification: textDocument/didSave* (Client → Server)
  - Params: DidSaveTextDocumentParams (textDocument: uri, text?: string)

- *Notification: textDocument/didClose* (Client → Server)
  - Params: DidCloseTextDocumentParams (textDocument: uri)

== Notebook Document Synchronization (Incremental)

Supports incremental sync for notebooks, including cell changes.

- *Notification: notebookDocument/didOpen* (Client → Server)
  - Params: DidOpenNotebookDocumentParams (notebookDocument: uri, metadata, cells)

- *Notification: notebookDocument/didChange* (Client → Server)
  - Params: DidChangeNotebookDocumentParams (metadata?: changes, cells: structural/text content changes)

- *Notification: notebookDocument/didSave* (Client → Server)
  - Params: DidSaveNotebookDocumentParams (notebookDocument: uri)

- *Notification: notebookDocument/didClose* (Client → Server)
  - Params: DidCloseNotebookDocumentParams (notebookDocument: uri)

== Semantic Tokens for 'Grammar' Language

Provides semantic highlighting for grammar definitions.

- *Request: textDocument/semanticTokens/full* (Client → Server)
  - Params: SemanticTokensParams (textDocument: uri)
  - Response: SemanticTokens (data: encoded tokens)

- *Request: textDocument/semanticTokens/range* (Client → Server)
  - Params: SemanticTokensRangeParams (textDocument: uri, range)
  - Response: SemanticTokens

- *Request: textDocument/semanticTokens/full/delta* (Client → Server)
  - Params: SemanticTokensDeltaParams (textDocument: uri, previousResultId)
  - Response: SemanticTokensDelta (edits)

- *Request: workspace/semanticTokens/refresh* (Server → Client)
  - Params: null
  - Response: null

== All Possible Requests from Server to Client

These are all standard requests the server can send to the client for a fully featured implementation.

- *Request: window/showMessageRequest* (Server → Client)
  - Params: ShowMessageRequestParams (type, message, actions)
  - Response: MessageActionItem or null

- *Request: window/showDocument* (Server → Client)
  - Params: ShowDocumentParams (uri, external?, takeFocus?, selection?)
  - Response: ShowDocumentResult (success)

- *Request: window/workDoneProgress/create* (Server → Client)
  - Params: WorkDoneProgressCreateParams (token)
  - Response: null

- *Request: workspace/applyEdit* (Server → Client)
  - Params: ApplyWorkspaceEditParams (label?, edit)
  - Response: ApplyWorkspaceEditResult (applied, failureReason?)

- *Request: workspace/configuration* (Server → Client)
  - Params: ConfigurationParams (items)
  - Response: any[]

- *Request: workspace/workspaceFolders* (Server → Client)
  - Params: null
  - Response: WorkspaceFolder[] or null

- *Request: client/registerCapability* (Server → Client)
  - Params: RegistrationParams (registrations)
  - Response: null

- *Request: client/unregisterCapability* (Server → Client)
  - Params: UnregistrationParams (unregisterations)
  - Response: null

- *Request: workspace/inlayHint/refresh* (Server → Client, if inlay hints enabled)
  - Params: null
  - Response: null

- *Request: workspace/inlineValue/refresh* (Server → Client, if inline values enabled)
  - Params: null
  - Response: null

- *Request: workspace/diagnostic/refresh* (Server → Client, if pull diagnostics enabled)
  - Params: null
  - Response: null

- *Request: textDocument/diagnostic* (Client → Server, but for pull model; server doesn't request this)

== Notifications from Server to Client (Logging, Telemetry, etc.)

These support logging, telemetry, and other features.

- *Notification: window/showMessage* (Server → Client)
  - Params: ShowMessageParams (type, message)

- *Notification: window/logMessage* (Server → Client)
  - Params: LogMessageParams (type, message)

- *Notification: telemetry/event* (Server → Client)
  - Params: any (telemetry data)

- *Notification: textDocument/publishDiagnostics* (Server → Client)
  - Params: PublishDiagnosticsParams (uri, diagnostics[], version?)

- *Notification: \$/progress* (Server → Client)
  - Params: ProgressParams (token, value: begin/work/report/end)

- *Notification: \$/logTrace* (Server → Client, if tracing enabled)
  - Params: LogTraceParams (message, verbose?)

- *Notification: \$/setTrace* (Client → Server, but server can respond accordingly)

- *Notification: \$/cancelRequest* (Both directions)
  - Params: CancelParams (id)

== Additional Required Notifications

- *Notification: \$/setTrace* (Client → Server)
  - Params: SetTraceParams (value: off/ messages/ verbose)

- *Notification: \$/cancelRequest* (Client → Server or Server → Client)

== Workspace Support

These methods provide full workspace management, including folder handling, configuration, symbols, commands, and file operations.

=== Requests and Notifications from Client to Server

- *Notification: workspace/didChangeWorkspaceFolders* (Client → Server)
  - Params: DidChangeWorkspaceFoldersParams (event: added/removed folders)

- *Notification: workspace/didChangeConfiguration* (Client → Server)
  - Params: DidChangeConfigurationParams (settings: any)

- *Notification: workspace/didChangeWatchedFiles* (Client → Server)
  - Params: DidChangeWatchedFilesParams (changes: array of file events)

- *Request: workspace/symbol* (Client → Server)
  - Params: WorkspaceSymbolParams (query: string, partialResultToken?)
  - Response: SymbolInformation[] or WorkspaceSymbol[] or null

- *Request: workspace/executeCommand* (Client → Server)
  - Params: ExecuteCommandParams (command: string, arguments?: any[])
  - Response: any or null

- *Request: workspace/willCreateFiles* (Client → Server, if fileOperations enabled)
  - Params: CreateFilesParams (files: array of file creates)
  - Response: WorkspaceEdit or null

- *Request: workspace/willRenameFiles* (Client → Server, if fileOperations enabled)
  - Params: RenameFilesParams (files: array of file renames)
  - Response: WorkspaceEdit or null

- *Request: workspace/willDeleteFiles* (Client → Server, if fileOperations enabled)
  - Params: DeleteFilesParams (files: array of file deletes)
  - Response: WorkspaceEdit or null

- *Notification: workspace/didCreateFiles* (Client → Server, if fileOperations enabled)
  - Params: CreateFilesParams (files: array of file creates)

- *Notification: workspace/didRenameFiles* (Client → Server, if fileOperations enabled)
  - Params: RenameFilesParams (files: array of file renames)

- *Notification: workspace/didDeleteFiles* (Client → Server, if fileOperations enabled)
  - Params: DeleteFilesParams (files: array of file deletes)

=== Requests from Server to Client (Already Partially Covered)

These are workspace-specific requests the server can send; some are already in the "All Possible Requests" section.

- *Request: workspace/workspaceFolders* (Server → Client)
  - Params: null
  - Response: WorkspaceFolder[] or null

- *Request: workspace/configuration* (Server → Client)
  - Params: ConfigurationParams (items: array of section/scopeUri)
  - Response: any[]

- *Request: workspace/applyEdit* (Server → Client)
  - Params: ApplyWorkspaceEditParams (label?, edit: WorkspaceEdit)
  - Response: ApplyWorkspaceEditResult (applied, failureReason?, failedChange?)

== Diagnostic Support (Pull Model)

These methods allow the client to pull diagnostics from the server, complementing publishDiagnostics.

- *Request: textDocument/diagnostic* (Client → Server)
  - Params: DocumentDiagnosticParams (textDocument: uri, identifier?, previousResultId?, partialResultToken?, workDoneToken?)
  - Response: DocumentDiagnosticReport (kind: full/changed/unchanged, resultId?, items: Diagnostic[])

- *Request: workspace/diagnostic* (Client → Server)
  - Params: WorkspaceDiagnosticParams (identifier?, previousResultIds?, partialResultToken?, workDoneToken?)
  - Response: WorkspaceDiagnosticReport (items: array of workspace document reports)

- *Request: workspace/diagnostic/refresh* (Server → Client)
  - Params: null
  - Response: null

== Progress and Cancellation

These support long-running operations and cancellation.

- *Request: window/workDoneProgress/create* (Server → Client)
  - Params: WorkDoneProgressCreateParams (token: ProgressToken)
  - Response: null

- *Notification: \$/progress* (Server → Client)
  - Params: ProgressParams (token: ProgressToken, value: WorkDoneProgressBegin | WorkDoneProgressReport | WorkDoneProgressEnd)

- *Notification: \$/cancelRequest* (Client → Server or Server → Client)
  - Params: CancelParams (id: number | string)

== Dynamic Registration

These allow runtime registration of capabilities.

- *Request: client/registerCapability* (Server → Client)
  - Params: RegistrationParams (registrations: array)
  - Response: null

- *Request: client/unregisterCapability* (Server → Client)
  - Params: UnregistrationParams (unregisterations: array)
  - Response: null

== Error Handling and General Messages

- Standard LSP error codes should be used in responses (e.g., -32600 Invalid Request, -32099 Server Error Start, etc.).
- All requests support optional `error` in responses: ResponseError (code, message, data?).

This completes the protocol skeleton, covering LSP version 3.17 features without language-specific services.

== LSP 3.18 Updates

These are extensions introduced in LSP 3.18, primarily for better notebook support. They can be implemented optionally after the 3.17 baseline.

=== Notebook-Specific Semantic Tokens

- *Request: notebookDocument/semanticTokens/full* (Client → Server)
  - Params: NotebookDocumentSemanticTokensParams (notebookDocument: uri)
  - Response: SemanticTokens (data: encoded tokens)

- *Request: notebookDocument/semanticTokens/range* (Client → Server)
  - Params: NotebookDocumentSemanticTokensRangeParams (notebookDocument: uri, range)
  - Response: SemanticTokens

- *Request: notebookDocument/semanticTokens/full/delta* (Client → Server)
  - Params: NotebookDocumentSemanticTokensDeltaParams (notebookDocument: uri, previousResultId)
  - Response: SemanticTokensDelta (edits)

=== Notebook-Specific Diagnostics (Pull Model)

- *Request: notebookDocument/diagnostic* (Client → Server)
  - Params: NotebookDocumentDiagnosticParams (notebookDocument: uri, identifier?, previousResultId?)
  - Response: NotebookDocumentDiagnosticReport (kind: full/changed/unchanged, resultId?, items: Diagnostic[] per cell)

=== Other 3.18 Enhancements

- Refined params for notebookDocument/didChange (e.g., better cell execution order and metadata handling), but no new message types.
- Capability advertisements under notebookDocument for semanticTokens and diagnostics.

== Handling Remote LSP Servers

=== Problem
When the LSP server is remote (different machine from client), it can't directly access client-local files for initial workspace indexing (e.g., listing files/content for language X). LSP is event-driven (client pushes changes via didOpen/didChange), lacking standard "pull" mechanisms. This hinders initial sync and full indexing without shared access.

=== Possible Solutions
- *Client-Driven Sync*: Client sends didOpen for all files initially; server stores in Document entities (content in PieceTables).
- *Custom LSP Requests*: Server requests file lists/content via experimental methods (e.g., "workspace/initialFileList"); client uploads, server appends to Log_Entry.
- *Hybrid DB API*: Client uploads via DB API; server queries Catalog_Entry for lists, Log_Entry for content.

== Detailed Handling of Remote LSP Servers

=== Problem in Depth
In scenarios where the LSP server runs remotely (e.g., on a separate machine, cloud VM, or container), it lacks direct access to the client's local filesystem. This poses challenges for initial workspace setup and indexing:
- LSP is inherently event-driven: The client pushes file events (e.g., textDocument/didOpen provides content for opened files, didChange sends updates).
- No standard mechanism exists for the server to "pull" a full file list or content from arbitrary workspace paths.
- For your workload (notebooks with cells as small files, grammar defs, ASTs), the server needs to index everything initially (e.g., for language X) to enable features like completion or diagnostics.
- Without access, indexing is incomplete (limited to client-pushed files), delaying interactive compilation or literate programming.
- Additional issues: Network latency, security (can't expose full client FS), and consistency (ensure DB reflects client state without data loss).

This is common in distributed setups (e.g., VS Code Remote), but requires careful integration with your log-structured DB and ARIES for reliability.

=== Initial State synchronization

==== Standard LSP Approach: Client-Driven Sync

*How it Works*: Leverage core LSP messages for client-initiated sync, with custom client behavior for initial population.
- On client connect, send "initialize" request; client includes workspaceFolders in params if supported.
- Client proactively sends textDocument/didOpen for all workspace files (e.g., notebooks, cells)—implement client-side logic to traverse and "open" everything (non-standard but allowed; batch to avoid overload).
- For updates, use didChange (incremental diffs) or didSave.
- Server processes events asynchronously.

*Key Issue*: The server does not have access to the initial list

=== Solutions

==== LSP Extension: Custom Requests for File Sync

*How it Works*: Extend LSP with experimental methods (advertised in ServerCapabilities.experimental) for server-driven pulls.
- After initialize, server sends custom request "workspace/initialFileList" (params: {workspaceUri: string, recursive: bool}).
- Client responds with array of {uri: string, metadata: {size: int, modified: timestamp}}—no content yet.
- For content, server follows with "textDocument/requestContent" (params: {uri: string}), client sends content (as string or chunks).
- Batch for efficiency (e.g., zip-like compression).

*DB Integration*:
- On response: Create File entities (file_id), append content to Log_Entry (PieceTable), reference in Catalog_Entry/Version for history (clustered by workspace_id).
- Atomic via ARIES: Log requests/responses in LSP_Request component; commit batches as txns.
- Indexing: Cluster diagnostics/semantic tokens by document_id for fast, local access during ops like publishDiagnostics.

*Pros*: Server controls sync; efficient for targeted pulls; integrates seamlessly with your ECS (e.g., request tracking clustered by client_id).
*Cons*: Needs client support for custom methods (non-standard); potential security (limit to workspace URIs); higher implementation effort.

==== Hybrid with External Sync (e.g., Via DB API or Git-Like Sync)

*How it Works*: DB as central repo (your Git replacement); client syncs via non-LSP channel (e.g., HTTP API to DB or Git protocol).
- On connect, client uploads workspace snapshot (e.g., via "db/syncWorkspace" API endpoint, sending tar/zip of files).
- Server ingests directly into DB, or client uses Git-like push to versioned branches.
- Ongoing: Client sends didChange over LSP; server pulls full updates via API if needed.

*DB Integration*:
- Uploads create new Versions (immutable, clustered by file_id); content in Log_Entry (PieceTables).
- Server queries Catalog_Entry (clustered by workspace_id) for lists; loads via clustered components.
- ARIES ensures no loss: Sync as txn; WAL logs uploads.
- For Git replacement: Treat sync as "commit"; branches for versions.

*Pros*: Robust for remote; aligns with no-loss/Git goals; offloads sync from LSP for bandwidth-heavy ops.
*Cons*: Requires separate API (beyond LSP); client must implement uploader; potential duplication if using didOpen too.

==== LSP FUSE Module Design

Goals:
- Provide a transparent, local-looking workspace for editors/tools by mounting a .wkspc database via FUSE.
- Eliminate polling; propagate changes via kernel notifications.
- Preserve DB guarantees (ARIES, versioning) while maximizing cache locality and throughput.

Architecture:
- Single DB daemon manages N workspaces (each workspace = separate DB). Daemon exposes a control socket (UDS locally; TLS/QUIC TCP optionally for remote).
- FUSE client mounts a .wkspc path as a POSIX tree; all FS ops are RPCd to the daemon.
- Path mapping: /mount/\<project>/... maps to DB objects (e.g., notebooks, cells, files). Stable path→object-id mapping maintained in Catalog.

FS Ops → DB Mapping:
- getattr/stat: Read metadata from Catalog (clustered by path) + latest Version; include size, mtime, inode-like id.
- readdir: Enumerate Catalog entries under a directory node; return names/types; paginate for large dirs.
- open: Return a file handle bound to (file_id, version_head). For write, start a DB txn.
- read: Read from PieceTable view; serve from in-memory cache first; fall back to Log_Entry ranges.
- write: Append edits as PieceTable ops (insert/replace) to Log_Entry; coalesce small writes; mark dirty.
- create/mkdir: Allocate new ids; insert into Catalog; initialize empty PieceTable or dir node; WAL-record in ARIES txn.
- unlink/rmdir: Mark tombstone in Catalog; keep historical Versions for recovery.
- rename: Atomic Catalog update; single ARIES txn; update parent dir entries.
- fsync: Flush dirty buffers; force WAL fsync; commit txn; advance Page LSNs.
- xattr (optional): Store editor/LSP metadata as small records in a side table keyed by file_id.

Caching (client-side):
- Page cache: fixed-size blocks (e.g., 64 KiB), read-ahead for sequential scans (e.g., readdir, large reads).
- Attribute cache: TTL for stat results; invalidate on server change events.
- Dentry cache: cache directory listings; invalidate on create/rename/unlink.
- Write-back: buffer writes; flush on fsync/close or timer; coalesce adjacent edits.

Notifications (no polling):
- Server emits change events per object (create/modify/rename/delete).
- FUSE client translates events to inotify/FSEvents so editors pick up changes immediately.
- Debounce/merge bursts to avoid event storms during batch updates.

Concurrency & Locking:
- Per-file leases/oplocks to coordinate multi-writer scenarios; optimistic by default with conflict detection on commit.
- Atomic rename and directory updates; directory-level locks only during short critical sections.
- Snapshot reads: readers see a consistent Version until they release handle.

Security:
- UDS ACLs for local; mutual TLS for remote; per-workspace ACLs (rw/ro).
- Namespace isolation: each mount rooted to a workspace DB; no path escape.
- Audit log of mutating ops (user, time, path, LSN).

Failure Handling:
- Network loss: queue writes locally (bounded); attempt reconnect; expose EIO after grace period.
- Crash recovery: ARIES WAL replay on daemon restart; client revalidates handles (GetVersion).
- Read-only fallback: mount ro if daemon declines rw due to recovery.

Mount Options:
- rw or ro; attr_ttl in milliseconds; entry_ttl in milliseconds; readahead in bytes; writeback on or off; prefetch on or off; stripe count (integer).
- Per-mount limits: max_open, max_dirty_bytes, flush_interval_ms.

Performance Knobs:
- Software striping across NVMe files/partitions for parallel I/O.
- Large I/O sizes for read/write (e.g., 256 KiB); zero-copy where possible.
- Hotset pinning: keep hot PieceTables/ASTs resident in daemon RAM.

Multi-User:
- Shared workspace mounts (multi-session); per-user permissions; conflict policy (last-writer-wins or merge hooks).
- Optional file-level locks (advisory) exposed via flock.

Minimal RPC API (client to daemon):
- Open(path, flags) → fh
- Read(fh, off, len) → bytes
- Write(fh, off, bytes) → n
- Fsync(fh) → ok
- Close(fh) → ok
- Stat(path|fh) → attrs
- List(dir, cookie, max) → entries
- Create(path, mode) → fh
- Mkdir(path, mode) → ok
- Unlink(path) → ok
- Rmdir(path) → ok
- Rename(old, new) → ok
- Watch(path, mask) → stream{event}
- GetVersion(fh|path) → {version_id, lsn}
- BeginTxn() / CommitTxn(lsn) / AbortTxn()

DB Considerations:
- Catalog clustered by directory path for readdir locality.
- PieceTable logs clustered by document_id for didChange/reads.
- Background compaction/defrag tuned to avoid interfering with editor latency.
