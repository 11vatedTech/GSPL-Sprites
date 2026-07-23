# GSPL Studio SDK API Reference

## Namespace Layout

```
gspl::studio    — Core Studio types (Project, Workspace, Document, IPC, etc.)
gspl::ls        — Language service (LsServer, Diagnostic, Completion, etc.)
gspl::plugin    — Plugin system (PluginManager, Manifest, etc.)
gspl::package   — Package management (PackageManager, PackageManifest, etc.)
```

---

## gspl::studio

### Project

Represents a single GSPL project with its `gspl-project.jsonc` manifest.

```cpp
class Project {
  static Result<Project> create_at(const std::filesystem::path& path);
  static Result<Project> load(const std::filesystem::path& path);

  const std::string& name() const noexcept;
  const std::string& version() const noexcept;
  const std::filesystem::path& root_path() const noexcept;

  Result<void> save() const;
  Result<std::string> read_source_file(const std::string& relative_path) const;
};
```

### Workspace

Manages a collection of projects within a workspace directory. Backed by SQLite.

```cpp
class Workspace {
  static Result<Workspace> open(const std::filesystem::path& path);
  static Result<Workspace> create(const std::filesystem::path& path);

  const std::filesystem::path& path() const noexcept;
  std::vector<std::shared_ptr<Project>> projects() const;

  Result<void> add_project(const std::filesystem::path& project_path);
  Result<void> remove_project(const std::string& project_name);
  void close();
};
```

### Document

Base class for all open documents in the editor.

```cpp
class Document {
  enum class Kind { Text, Gene, Morphology, Form, Animation, Behavior, Combat, Graph };

  Kind kind() const noexcept;
  const std::string& name() const noexcept;
  bool dirty() const noexcept;
  const std::filesystem::path& file_path() const noexcept;

  Result<void> load(const std::filesystem::path& path);
  Result<void> save() const;

  using ChangeCallback = std::function<void()>;
  void set_change_callback(ChangeCallback cb);
};
```

### UndoStack

Bounded-depth command-pattern undo/redo stack.

```cpp
class UndoStack {
  explicit UndoStack(size_t max_depth = 256);

  void push(std::unique_ptr<Command> cmd);
  bool can_undo() const noexcept;
  bool can_redo() const noexcept;
  void undo();
  void redo();
  void clear();
  size_t depth() const noexcept;
};
```

### IpcEnvelope

Versioned message envelope for inter-process communication.

```cpp
struct IpcEnvelope {
  int version;
  std::string kind;             // e.g. "request", "response", "event"
  std::string correlation_id;
  std::string payload;          // JSON string
  std::optional<SharedMemRef> shared_memory;

  std::string to_json() const;
  static std::optional<IpcEnvelope> from_json(const std::string& json);
};
```

### IpcChannel

Length-prefixed JSON framing over stdin/stdout.

```cpp
class IpcChannel {
  explicit IpcChannel(QIODevice* device);

  void send(const IpcEnvelope& envelope);
  void send(const std::string& kind, const std::string& payload);

  using MessageCallback = std::function<void(const IpcEnvelope&)>;
  void set_message_callback(MessageCallback cb);
  void start();
  void stop();
};
```

### WorkerProcess

QProcess wrapper with health-check and restart policy.

```cpp
class WorkerProcess {
  using RestartPolicy = /* always, on_crash, never */;

  WorkerProcess(const QString& executable, RestartPolicy policy);
  void start();
  void stop();
  bool is_running() const noexcept;
  void health_check();

  IpcChannel* channel() noexcept;
};
```

### SharedMemory

Double-buffered shared memory for large payload transfer.

```cpp
class SharedMemory {
  explicit SharedMemory(const std::string& name);
  bool write(const void* data, size_t size);
  std::vector<std::byte> read() const;
  void release();
};
```

### ThemeManager

Manages theme loading, switching, and validation.

```cpp
class ThemeManager {
  void load_builtin_themes();
  Result<void> load_theme(const std::filesystem::path& path);
  bool activate_theme(const std::string& name);
  const ThemeDefinition* active_theme() const noexcept;
  std::vector<const ThemeDefinition*> available_themes() const;

  static double contrast_ratio(const std::string& hex1, const std::string& hex2);
};
```

### ThemeDefinition

```cpp
struct ThemeDefinition {
  std::string name;
  std::string author;
  std::string version;
  std::string type;  // "light", "dark", "high-contrast"
  std::unordered_map<std::string, std::string> colors;
};
```

### Logger

Singleton structured logger.

```cpp
class Logger {
  enum Level { Debug, Info, Warning, Error };

  static Logger& instance();
  void log(Level level, const std::string& message);
  void set_level(Level level);
  void set_callback(std::function<void(Level, const std::string&)> cb);
};
```

### SyntaxHighlighter

Tokenizes GSPL source for syntax highlighting.

```cpp
class SyntaxHighlighter {
  struct Token { size_t offset; size_t length; std::string type; };

  std::vector<Token> highlight(const std::string& source) const;
  std::vector<Token> highlight_line(const std::string& line, size_t line_number) const;
};
```

### DocumentSymbolBuilder

Builds a symbol tree from a GSPL document for the graph editor and navigation.

```cpp
class DocumentSymbolBuilder {
  struct Symbol { std::string name; std::string kind; size_t line; size_t column; };

  std::vector<Symbol> build(const std::string& source) const;
};
```

---

## gspl::ls

### LsServer

LSP-compatible language server.

```cpp
class LsServer {
  bool initialize(const std::string& workspace_root);
  std::string handle_request(const std::string& method, const std::string& params);
  std::vector<Diagnostic> diagnostics(const std::string& uri);
  std::vector<CompletionItem> completions(const std::string& uri, size_t line, size_t col);
  std::vector<SymbolInfo> symbols(const std::string& uri);
  std::optional<Location> definition(const std::string& uri, size_t line, size_t col);
  std::vector<Location> references(const std::string& uri, size_t line, size_t col);
  std::optional<HoverInfo> hover(const std::string& uri, size_t line, size_t col);
  void shutdown();
};
```

### Diagnostic

```cpp
struct Diagnostic {
  size_t line_start, col_start, line_end, col_end;
  Severity severity;  // Error, Warning, Info
  std::string message;
  std::string code;
};
```

### CompletionItem

```cpp
struct CompletionItem {
  std::string label;
  std::string kind;       // "keyword", "function", "type", "variable", etc.
  std::string detail;
  std::string insert_text;
  std::string documentation;
};
```

### SymbolInfo / Location / HoverInfo

```cpp
struct Location {
  std::string uri;
  size_t line, column;
};

struct SymbolInfo {
  std::string name;
  std::string kind;
  Location location;
};

struct HoverInfo {
  std::string contents;
  size_t line, column;
};
```

---

## gspl::plugin

### PluginManager

Discovers, loads, activates, and unloads plugins.

```cpp
class PluginManager {
  void discover(const std::vector<std::filesystem::path>& search_dirs);
  Result<void> load(const std::string& plugin_id);
  Result<void> activate(const std::string& plugin_id);
  void deactivate(const std::string& plugin_id);
  void unload(const std::string& plugin_id);
  const PluginManifest* manifest(const std::string& plugin_id) const;
  std::vector<std::string> loaded_plugins() const;
};
```

### PluginManifest

```cpp
struct PluginManifest {
  std::string id;
  std::string version;
  std::string name;
  std::string description;
  std::string author;
  std::vector<std::string> capabilities;
  std::unordered_map<std::string, std::string> dependencies;
  std::string entry;
};
```

---

## gspl::package

### PackageManager

Installs, updates, and removes packages with dependency resolution.

```cpp
class PackageManager {
  Result<void> install(const std::string& package_id, const std::string& version = "latest");
  Result<void> uninstall(const std::string& package_id);
  Result<void> update(const std::string& package_id, const std::string& version);
  Result<std::vector<PackageManifest>> resolve_dependencies(const PackageManifest& root);
  std::vector<const PackageManifest*> installed() const;
};
```

### PackageManifest

```cpp
struct PackageManifest {
  std::string id;
  std::string version;
  std::string name;
  std::string description;
  std::string author;
  std::string license;
  std::string signature;   // Ed25519 signature for verification
  std::unordered_map<std::string, std::string> dependencies;
};
```

---

## gspl::studio (continued)

### PublishingManager

```cpp
class PublishingManager {
  Result<void> add_target(const std::string& name, const std::string& type);
  Result<void> publish(const std::string& target_name, const std::string& channel);
  Result<void> rollback(const std::string& target_name, const std::string& version);
  std::vector<PublishRecord> publish_history(const std::string& target_name) const;
};
```

### TargetAdapterManager

```cpp
class TargetAdapterManager {
  void register_adapter(std::unique_ptr<TargetAdapter> adapter);
  std::vector<std::string> available_profiles() const;
  Result<void> detect_sdk(const std::string& profile);
  void set_active_profile(const std::string& profile);
  const TargetAdapter* active_adapter() const;
};
```

### GitIntegration

```cpp
class GitIntegration {
  std::string status() const;
  std::string diff(const std::string& path = "") const;
  Result<void> stage(const std::vector<std::string>& files);
  Result<void> commit(const std::string& message);
  std::vector<CommitEntry> log(int count = 50) const;
  std::optional<BlameEntry> blame(const std::string& path, size_t line) const;
};
```

---

## API Stability Policy

The Studio SDK follows semantic versioning (`major.minor.patch`):

- **Major** — Breaking API changes (removed or renamed classes/functions, changed signatures)
- **Minor** — New functionality added without breaking existing APIs
- **Patch** — Bug fixes and internal changes with no API surface change

All public headers in `include/gspl/studio/`, `include/gspl/ls/`, `include/gspl/plugin/`, and `include/gspl/package/` are covered by this policy. Headers in `src/` are internal and may change without notice.
