# CodeForge: Multi-Language Web Compiler

CodeForge is an educational web-based compiler supporting a unified subset of **C, C++, and Java**. It implements a complete front-to-intermediate compiler pipeline, transforming source code through lexical analysis, syntax parsing, semantic checks, and Three-Address Code (TAC) generation.

---

## Features & Supported Subset

* **Data Types**: `int`, `float`, `bool`
* **Literals**: Integer (`42`), Floating-point (`3.14`), Boolean (`true`, `false`)
* **Identifiers**: Standard naming rules (`[a-zA-Z_][a-zA-Z0-9_]*`)
* **Operators**:
  * Arithmetic: `+`, `-`, `*`, `/`, `%`
  * Relational: `<`, `>`, `<=`, `>=`, `==`, `!=`
  * Logical: `&&`, `||`, `!`
* **Statements & Control Flow**:
  * Variable declarations & assignments (`int x = 10;`, `x = x + 1;`)
  * Branching (`if`, `if-else`)
  * Loops (`while`, `for`)
  * Output statements (`print(...)`, `printf(...)`, `std::cout`, `System.out.println(...)`)
  * Arbitrarily nested blocks with lexical scoping (`{ ... }`)
* **Diagnostics & Error Handling**: Line-accurate reporting for lexical anomalies, syntax errors with recovery sync points, duplicate declarations, undeclared variable references, and type mismatches.
* **Intermediate Representation**: Linear Three-Address Code (TAC) using temporaries (`t1`, `t2`) and label-based branching (`L1`, `L2`).

---

## Architecture & SDLC Pipeline

The project follows an 8-stage Software Development Life Cycle (SDLC):

```
Source Code
    │
    ▼
[ 1. Lexical Analyzer (Flex) ] ──────► Token Stream
    │
    ▼
[ 2. Syntax Analyzer (Bison) ] ─────► Abstract Syntax Tree (AST)
    │
    ▼
[ 3. Semantic Analyzer ] ────────────► Scoped Symbol Table & Diagnostics
    │
    ▼
[ 4. Code Generator ] ──────────────► Three-Address Code (TAC)
    │
    ▼
[ 5. Web Interface Bridge ] ────────► Browser UI Visualizer
```

### Team Responsibilities

| Role / Track | Responsibility | Primary Files |
| :--- | :--- | :--- |
| **Track 1: Frontend Engine** | Lexer tokenization, formal CFG, operator precedence, parser error recovery | `src/lexer/`, `src/parser/`, `docs/cfg_grammar.md` |
| **Track 2: Middle-End Engine** | AST hierarchy, scoped symbol table management, type checking, semantic validation | `src/ast/`, `src/symbol_table/`, `src/semantic/`, `src/common/` |
| **Track 3: Backend & Web** | TAC generation, driver pipeline (`main.cpp`), web bridge (`server.php`), UI panels | `src/codegen/`, `src/web/`, `Makefile` |

---

## Directory Structure

```text
CodeForge/
├── docs/                 # CFG specification, report, architecture diagrams
├── examples/             # Sample source programs in C, C++, and Java styles
├── src/
│   ├── common/           # Shared utilities (error reporter, diagnostic types)
│   ├── lexer/            # Flex lexer specification (lexer.l)
│   ├── parser/           # Bison grammar rules (parser.y)
│   ├── ast/              # AST node classes, print utilities, JSON export
│   ├── symbol_table/     # Scoped environment tree (enter/exit/lookup)
│   ├── semantic/         # Type checking and semantic rule verification
│   ├── codegen/          # TAC instructions and generator
│   ├── web/              # Web frontend (HTML/CSS/JS) and server runner
│   └── main.cpp          # CLI driver and JSON serialization flag
├── tests/
│   ├── valid/            # Correct programs demonstrating language features
│   ├── invalid/          # Test cases for syntax, scoping, and type errors
│   └── *.sh              # Automated test runner scripts
├── Makefile              # Build automation
└── README.md
```

---

## Prerequisites

Ensure the following tools are installed on your system:

* **C++ Compiler**: `g++` (supporting C++17 or newer)
* **Lexer Generator**: `flex` (v2.6+)
* **Parser Generator**: `bison` (v3.0+)
* **Build Tool**: `make`
* **Web Environment** (optional, for web UI): A local PHP server (PHP 7.4+ or XAMPP) or any lightweight backend runner.

---

## Build Instructions

### Compile the CLI Engine

Run `make` from the project root directory:

```bash
make
```

This compiles Flex and Bison sources, builds the C++ modules, and produces the native executable:

```bash
./codeforge_compiler
```

### Clean Build Artifacts

To remove generated parser files, object files, and binaries:

```bash
make clean
```

---

## Usage

### Command-Line Interface (CLI)

Run the compiler against a source file with specific inspection flags:

```bash
# Print token stream
./codeforge_compiler --tokens examples/demo_c.c

# Print Abstract Syntax Tree
./codeforge_compiler --ast examples/demo_cpp.cpp

# Inspect Scoped Symbol Table
./codeforge_compiler --symbol-table examples/demo_java.java

# Output generated Three-Address Code (TAC)
./codeforge_compiler --tac examples/demo_c.c

# Emit complete JSON output for all phases (used by the web UI)
./codeforge_compiler --json examples/demo_c.c
```

### Running the Web Interface

1. Start a local server inside `src/web/` (or host the root via XAMPP):
   ```bash
   php -S localhost:8000 -t src/web/
   ```
2. Open your browser and navigate to `http://localhost:8000`.
3. Input or select sample C/C++/Java code, then click **Compile** to view the outputs across the dedicated tabs.

---

## Running Tests

Run the automated test suite across valid and invalid test programs:

```bash
make test
```

Or run individual test scripts:

```bash
bash tests/test_lexer.sh
bash tests/test_parser.sh
bash tests/test_semantic.sh
```

---

## Git Workflow

* Feature development must occur on separate branches:
  * `feature/lexer-parser`
  * `feature/ast-symbol-semantic`
  * `feature/tac-web-integration`
* Merges to `main` require a clean build (`make`) and passing automated test suites.