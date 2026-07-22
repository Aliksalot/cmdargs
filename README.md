# cmdargs

A small, header-only C++ library for parsing command line arguments.

It supports two usage patterns:

- **`CommandLine`** — parse `argc`/`argv` once, like a typical CLI argument parser (getopt, cxxopts, etc.).
- **`CommandList`** — define a set of named commands and run a REPL-style loop, reading and dispatching one command string at a time (think `psql`, `redis-cli`, or any "type a command, get a result" tool).

Both are built on the same underlying command definition (`SimpleCommand`), so flags, positional args, and usage generation work identically in either mode.

## Quickstart

Header-only, so there's nothing to build or link. Just include it:

```cpp
#include "cmdargs.hpp"
```

### As a CLI parser (`CommandLine`)

```cpp
int main(int argc, const char** argv) {
    cmdargs::CommandArgs args, flags;

    bool ok = cmdargs::fromArgv(argc, argv)
        .describe("Copies a file from one place to another")
        .flag(cmdargs::CommandFlagType::NoValue, "verbose", "v", "Print extra output")
        .flag(cmdargs::CommandFlagType::WithValue, "output", "o", "Output path")
        .arg(cmdargs::CommandArgType::Required, "source", "File to copy")
        .includeHelp()
        .into(args, flags);

    if (!ok) return 1;

    std::string source = args.value("source");
    std::string output = flags.value("output", [] { return "./out"; });

    if (flags.has("verbose")) {
        std::cout << "Copying " << source << " to " << output << "\n";
    }
}
```

Run it with `--help` or `-h` and it exits after printing auto-generated usage.

### As a command loop (`CommandList`)

```cpp
cmdargs::CommandList<2> commands;

commands.add(
    cmdargs::Command("greet")
        .describe("Greets someone by name")
        .arg(cmdargs::CommandArgType::Required, "name")
        .does([](const cmdargs::CommandArgs& args, const cmdargs::CommandArgs&) {
            std::cout << "Hello, " << args.value("name") << "!\n";
        })
);

commands.add(
    cmdargs::Command("quit")
        .describe("Exits the program")
        .does([](const cmdargs::CommandArgs&, const cmdargs::CommandArgs&) {
            std::exit(0);
        })
);

commands.includeHelp(); // adds a "help [command-name]" command

std::string line;
while (std::getline(std::cin, line)) {
    commands.execute(line);
}
```

`CommandList<N>` is templated on the number of commands you plan to add (not counting `help`), and it asserts that you actually add exactly that many before use — this is checked at `execute` time via `assertCommandsExhausted`, so a half-populated list will throw rather than silently misbehave.

## Positional args and flags

**Positional args** (`CommandArg`) are ordered, and matched to a command by position:

- Declared as `CommandArgType::Required` or `CommandArgType::Optional`.
- Optional args must be declared *after* all required ones — there's no support for "gap" optionality.
- Retrieved from the resulting `CommandArgs` by the name you gave them (`args.value("source")`), not by index — the name doubles as both the display label in usage text and the lookup key.

**Flags** (`CommandFlag`) are unordered and matched by name:

- Always have both a long form (`--my-flag`) and a short form (`-mf`) — there's no long-only or short-only flag.
- `CommandFlagType::WithValue` consumes the next token as its value (`--output ./out`); `CommandFlagType::NoValue` is a boolean switch that's simply present or absent.
- Regardless of which form is used on the command line, they're stored under the **long name** in the resulting `CommandArgs`, so `flags.value("output")` works whether the user typed `--output` or `-o`.

Both args and flags are optional to describe — if you skip the description, usage text just prints "No description provided" for that entry.

## `CommandArgs`

Both parsed args and parsed flags come back as a `CommandArgs` — a thin wrapper around `std::unordered_map<std::string, std::string>`. A few things worth knowing:

- `.value(key)` throws if the key is missing; `.valueOpt(key)` returns `std::optional<std::string>` instead.
- `.value(key, fallback)` takes a `std::function<std::string()>` and calls it lazily if the key is absent — useful for defaults that are expensive to compute or that you'd rather not construct unconditionally.
- No-value flags are stored with a sentinel value of `"SET"` internally; check for their presence with `.has(key)`, not `.value(key)`.

## Usage/help generation

Calling `.getUsage()` on any command produces a formatted block:

```
copy - Copies a file from one place to another
  OPTIONAL FLAGS
    --verbose -v - Print extra output
    --output -o <value> - Output path
  POSITIONAL ARGS
    <source> - File to copy
```

Required args are shown as `<name>`, optional ones as `[name]`. This is what gets printed automatically when parsing fails, or when `--help`/`-h` is passed after calling `.includeHelp()` on a `CommandLine`.

For `CommandList`, `.includeHelp()` instead registers a `help [command-name]` command: called with no argument it prints usage for every registered command, called with a name it prints usage for just that one. Note this reserves the name `"help"` — trying to `.add()` your own command called `help` after calling `.includeHelp()` is rejected with a warning to stderr rather than a hard failure.

## Other things worth knowing

- **Quoted tokens**: `SimpleCommand::tokenize` (used internally by `CommandList::execute`) understands double-quoted substrings as single tokens, including backslash-escaping inside quotes — so `run "hello \"world\""` tokenizes as expected. This tokenizer isn't exposed for `CommandLine`, since `argv` is already tokenized by the shell.
- **Chained/fluent API**: nearly every builder method (`.arg`, `.flag`, `.describe`, `.does`, `.includeHelp`, etc.) returns a reference to the object itself (via CRTP in `SimpleCommand<Derived>`), so definitions read top-to-bottom as a single chained expression.
- **No exceptions for ordinary parse failures**: malformed CLI input (unknown flag, wrong arg count, etc.) is reported via `std::cerr` and a `false` return value, not an exception — `into()` and `execute()` return `bool`. Exceptions (`std::logic_error`, `std::invalid_argument`, `std::out_of_range`) are reserved for programmer errors: misusing the library's own API (e.g. exhausting a `CommandList` incorrectly, requesting a missing key via `.value()`) or malformed quoting during tokenization.
- **`CommandLine` has no default constructor** — it always requires `argc`/`argv`, since there's no meaningful "empty" CLI invocation.
