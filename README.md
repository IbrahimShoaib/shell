# mosh - My Own Shell

A lightweight, custom Unix shell implementation in C. This project demonstrates core Operating Systems concepts including process creation, execution, signal handling, and inter-process communication.

## Features

- **Command Execution**: Supports standard Unix commands (e.g., `ls`, `grep`, `cat`).
- **Pipelines**: Supports piping between multiple commands (e.g., `ls -l | grep .c | wc -l`).
- **Built-in Commands**:
  - `cd <directory>`: Change current working directory.
  - `history`: View command history.
  - `exit`: Terminate the shell.
  - `help`: Display available commands.
  - `watch <command>`: (Linux only) Monitor process statistics like CPU time and memory usage.
- **Signal Handling**: Gracefully handles `SIGINT` (Ctrl+C).

## Project Structure

```
shell/
├── Makefile              # Build script
├── include/
│   └── shell.h           # Function prototypes and definitions
└── src/
    ├── main.c            # Main REPL loop
    ├── builtins.c        # Built-in command implementations
    ├── executor.c        # Process execution logic (fork/exec/pipe)
    ├── parser.c          # Command parsing logic
    └── utils.c           # Helper functions and signal handlers
```

## Building and Running

### Prerequisites

- GCC Compiler
- Make
- Linux (for `watch` command features) or macOS

### Compilation

To build the shell, run:

```bash
make
```

### Usage

Start the shell:

```bash
./shell
```

You will see the prompt `## mosh >>`. You can now type commands.

### Examples

```bash
## mosh >> ls -l
## mosh >> cd ..
## mosh >> history
## mosh >> ls | grep .c
```

## Development

To clean build artifacts:

```bash
make clean
```

## License

This project is open source and available under the [MIT License](LICENSE).
