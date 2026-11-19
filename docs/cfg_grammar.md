# CodeForge Formal Context-Free Grammar (CFG) Specification

## 1. Overview & Convergent Grammar Design

CodeForge implements a **convergent front-to-intermediate compiler architecture** for a unified subset of **C, C++, and Java**. Rather than maintaining three separate parsers, CodeForge defines a single, unambiguous Context-Free Grammar $G = (V, \Sigma, R, S)$ where:

* **$V$ (Non-Terminals):** Syntactic constructs (`program`, `stmt_list`, `stmt`, `var_decl`, `assign_stmt`, `if_stmt`, `while_stmt`, `for_stmt`, `print_stmt`, `block`, `expr`, etc.).
* **$\Sigma$ (Terminals):** Lexical tokens produced by the Flex scanner (`TOK_INT`, `TOK_FLOAT`, `TOK_BOOL`, `TOK_BOOLEAN`, `TOK_ID`, operators, and delimiters).
* **$R$ (Production Rules):** Grammar derivations that construct a unified Abstract Syntax Tree (`src/ast/ast.h`).
* **$S$ (Start Symbol):** `program`.

To support **Strict Language Mode** (`--lang=c|cpp|java`), language-specific surface constructs (such as `bool` vs. `boolean` and `printf` vs. `cout <<` vs. `System.out.println`) are parsed into shared AST nodes tagged with `TypeKeyword` and `PrintFlavor` metadata for validation during semantic analysis.

---

## 2. Terminal Symbols ($\Sigma$)

| Category | Terminal Token | Lexeme / Pattern |
| :--- | :--- | :--- |
| **Type Keywords** | `TOK_INT`, `TOK_FLOAT`, `TOK_BOOL`, `TOK_BOOLEAN` | `int`, `float`, `bool`, `boolean` |
| **Control Keywords** | `TOK_IF`, `TOK_ELSE`, `TOK_WHILE`, `TOK_FOR` | `if`, `else`, `while`, `for` |
| **Output Keywords** | `TOK_PRINT`, `TOK_PRINTF`, `TOK_COUT`, `TOK_STD`, `TOK_SYSTEM`, `TOK_OUT`, `TOK_PRINTLN` | `print`, `printf`, `cout`, `std`, `System`, `out`, `println` |
| **Boolean Literals** | `TOK_TRUE`, `TOK_FALSE` | `true`, `false` |
| **Literals & IDs** | `TOK_INT_LIT`, `TOK_FLOAT_LIT`, `TOK_ID` | `[0-9]+`, `[0-9]+\.[0-9]+`, `[a-zA-Z_][a-zA-Z0-9_]*` |
| **Arithmetic Ops** | `TOK_PLUS`, `TOK_MINUS`, `TOK_MUL`, `TOK_DIV`, `TOK_MOD` | `+`, `-`, `*`, `/`, `%` |
| **Relational Ops** | `TOK_LT`, `TOK_GT`, `TOK_LE`, `TOK_GE`, `TOK_EQ`, `TOK_NEQ` | `<`, `>`, `<=`, `>=`, `==`, `!=` |
| **Logical Ops** | `TOK_AND`, `TOK_OR`, `TOK_NOT` | `&&`, `\|\|`, `!` |
| **Stream / Scope Ops** | `TOK_SHL`, `TOK_SCOPE`, `TOK_DOT`, `TOK_ASSIGN` | `<<`, `::`, `.`, `=` |
| **Delimiters** | `TOK_LPAREN`, `TOK_RPAREN`, `TOK_LBRACE`, `TOK_RBRACE`, `TOK_SEMI`, `TOK_COMMA` | `(`, `)`, `{`, `}`, `;`, `,` |

---

## 3. Formal BNF / EBNF Productions ($R$)

### 3.1 Program & Statement Structure
```bnf
<program>       ::= <stmt_list>

<stmt_list>     ::= <stmt_list> <stmt>
                  | ε

<stmt>          ::= <var_decl>
                  | <assign_stmt>
                  | <print_stmt>
                  | <if_stmt>
                  | <while_stmt>
                  | <for_stmt>
                  | <block>
                  | TOK_SEMI

<block>         ::= TOK_LBRACE <stmt_list> TOK_RBRACE
```

### 3.2 Variable Declarations & Assignments
Factoring out the trailing semicolon into `_core` rules allows `for`-loop headers to reuse declaration and assignment productions without duplicating semicolons.

```bnf
<type_spec>     ::= TOK_INT
                  | TOK_FLOAT
                  | TOK_BOOL
                  | TOK_BOOLEAN

<var_decl>      ::= <var_decl_core> TOK_SEMI

<var_decl_core> ::= <type_spec> TOK_ID
                  | <type_spec> TOK_ID TOK_ASSIGN <expr>

<assign_stmt>   ::= <assign_core> TOK_SEMI

<assign_core>   ::= TOK_ID TOK_ASSIGN <expr>
```

### 3.3 Control Flow Statements
```bnf
<if_stmt>       ::= TOK_IF TOK_LPAREN <expr> TOK_RPAREN <stmt>
                  | TOK_IF TOK_LPAREN <expr> TOK_RPAREN <stmt> TOK_ELSE <stmt>

<while_stmt>    ::= TOK_WHILE TOK_LPAREN <expr> TOK_RPAREN <stmt>

<for_stmt>      ::= TOK_FOR TOK_LPAREN <for_init> TOK_SEMI <for_cond> TOK_SEMI <for_update> TOK_RPAREN <stmt>

<for_init>      ::= <var_decl_core>
                  | <assign_core>
                  | ε

<for_cond>      ::= <expr>
                  | ε

<for_update>    ::= <assign_core>
                  | ε
```

### 3.4 Convergent Multi-Language Output
All four language output idioms converge into a single `PrintNode` carrying its originating `PrintFlavor`.

```bnf
<print_stmt>    ::= TOK_PRINT TOK_LPAREN <expr> TOK_RPAREN TOK_SEMI
                  | TOK_PRINTF TOK_LPAREN <expr> TOK_RPAREN TOK_SEMI
                  | TOK_COUT TOK_SHL <expr> TOK_SEMI
                  | TOK_STD TOK_SCOPE TOK_COUT TOK_SHL <expr> TOK_SEMI
                  | TOK_SYSTEM TOK_DOT TOK_OUT TOK_DOT TOK_PRINTLN TOK_LPAREN <expr> TOK_RPAREN TOK_SEMI
```

### 3.5 Expressions
In the formal grammar, expressions are stratified by precedence (or written flat in Bison with explicit `%left` / `%right` precedence declarations):

```bnf
<expr>          ::= <expr> TOK_OR <expr>
                  | <expr> TOK_AND <expr>
                  | <expr> TOK_EQ <expr>
                  | <expr> TOK_NEQ <expr>
                  | <expr> TOK_LT <expr>
                  | <expr> TOK_GT <expr>
                  | <expr> TOK_LE <expr>
                  | <expr> TOK_GE <expr>
                  | <expr> TOK_PLUS <expr>
                  | <expr> TOK_MINUS <expr>
                  | <expr> TOK_MUL <expr>
                  | <expr> TOK_DIV <expr>
                  | <expr> TOK_MOD <expr>
                  | TOK_NOT <expr>
                  | TOK_MINUS <expr>              %prec UMINUS
                  | TOK_PLUS <expr>               %prec UPLUS
                  | TOK_LPAREN <expr> TOK_RPAREN
                  | TOK_INT_LIT
                  | TOK_FLOAT_LIT
                  | TOK_TRUE
                  | TOK_FALSE
                  | TOK_ID
```

---

## 4. Operator Precedence & Associativity Table

Operators are ordered from **lowest precedence (Level 1)** to **highest precedence (Level 8)**, matching standard C, C++, and Java evaluation rules:

| Precedence Level | Operators | Category | Associativity | Bison Directive |
| :---: | :--- | :--- | :--- | :--- |
| **1 (Lowest)** | `\|\|` | Logical OR | Left-to-right | `%left TOK_OR` |
| **2** | `&&` | Logical AND | Left-to-right | `%left TOK_AND` |
| **3** | `==`, `!=` | Equality | Left-to-right | `%left TOK_EQ TOK_NEQ` |
| **4** | `<`, `>`, `<=`, `>=` | Relational | Left-to-right | `%left TOK_LT TOK_GT TOK_LE TOK_GE` |
| **5** | `+`, `-` | Additive | Left-to-right | `%left TOK_PLUS TOK_MINUS` |
| **6** | `*`, `/`, `%` | Multiplicative | Left-to-right | `%left TOK_MUL TOK_DIV TOK_MOD` |
| **7** | `!`, unary `-`, unary `+` | Unary Prefix | Right-to-left | `%right TOK_NOT UMINUS UPLUS` |
| **8 (Highest)** | `( ... )` | Grouping | Non-associative | Handled by primary rule |

---

## 5. Resolving the Dangling-Else Ambiguity

### The Ambiguity
Consider the nested conditional:
```c
if (a) if (b) x = 1; else x = 2;
```
An unrestricted grammar produces two valid parse trees:
1. **Inner-bind (Standard C/C++/Java rule):** `else x = 2;` attaches to `if (b)`.
2. **Outer-bind:** `else x = 2;` attaches to `if (a)`.

In an LALR(1) parser generator like Bison, when the parser sees `TOK_IF '(' expr ')' stmt` on the stack and `TOK_ELSE` as the lookahead token, this creates a **Shift/Reduce conflict**:
* **Shift:** Consume `TOK_ELSE` and attach it to the inner `if`.
* **Reduce:** Complete the inner `if` without an `else`, leaving `TOK_ELSE` for the outer `if`.

### Bison Precedence Resolution
CodeForge resolves this deterministically in `src/parser/parser.y` without duplicating statement rules (matched vs. unmatched statements) by declaring dummy precedence tokens:

```yacc
%nonassoc LOWER_THAN_ELSE
%nonassoc TOK_ELSE
```

And tagging the single-branch `if` production with `%prec LOWER_THAN_ELSE`:

```yacc
if_stmt
    : TOK_IF TOK_LPAREN expr TOK_RPAREN stmt %prec LOWER_THAN_ELSE
    | TOK_IF TOK_LPAREN expr TOK_RPAREN stmt TOK_ELSE stmt
    ;
```

Because `TOK_ELSE` has higher precedence than `LOWER_THAN_ELSE`, Bison always **shifts** `TOK_ELSE` when encountered, binding every `else` clause to the nearest preceding unmatched `if` while maintaining **zero shift/reduce conflicts**.

---

## 6. Grammar-to-AST Node Mapping

| Production Rule | Instantiated AST Class (`src/ast/ast.h`) | Metadata / Fields Populated |
| :--- | :--- | :--- |
| `<program>` | `ProgramNode` | `statements` vector |
| `<block>` | `BlockNode` | `statements` vector, `line` |
| `<var_decl>` | `VarDeclNode` | `varType`, `rawKeyword` (`KW_BOOL` vs `KW_BOOLEAN`), `name`, `initializer` |
| `<assign_stmt>` | `AssignNode` | `name`, `value` |
| `<if_stmt>` | `IfNode` | `condition`, `thenBranch`, `elseBranch` (nullable) |
| `<while_stmt>` | `WhileNode` | `condition`, `body` |
| `<for_stmt>` | `ForNode` | `init`, `condition`, `update`, `body` |
| `<print_stmt>` | `PrintNode` | `expression`, `flavor` (`GENERIC_PRINT`, `C_PRINTF`, `CPP_COUT`, `JAVA_SYSOUT`) |
| `<expr>` (binary) | `BinaryExprNode` | `op` (`Operator`), `left`, `right` |
| `<expr>` (unary) | `UnaryExprNode` | `op` (`Operator::NOT`, `NEG`, `POS`), `operand` |
| Literals / ID | `IntLiteralNode`, `FloatLiteralNode`, `BoolLiteralNode`, `IdentifierNode` | `value` / `name`, `line` |