(() => {
  "use strict";

  const PRESETS = {
    cpp_demo: {
      lang: "cpp",
      code:
`// CodeForge C++ Demo: Scoping & Conditionals
int score = 85;
bool passed = false;

if (score >= 50) {
    passed = true;
    int bonus = 10;
    score = score + bonus;
    cout << "Final Score:";
    cout << score;
}

cout << passed;
`
    },
    c_demo: {
      lang: "c",
      code:
`// CodeForge C Demo: While Loop & Branching
int base = 10;
float rate = 2.5;
int iterations = 4;
int counter = 0;

while (counter < iterations) {
    base = base + 5;
    counter = counter + 1;
}

if (base > 20) {
    printf("Result exceeded threshold:");
    printf(base);
} else {
    printf(counter);
}
`
    },
    java_demo: {
      lang: "java",
      code:
`// CodeForge Java Demo: For Loop & Float Widening
int sum = 0;
for (int i = 1; i <= 5; i = i + 1) {
    sum = sum + i;
}

float avg = sum / 2.0;
boolean ok = !(sum < 10);

System.out.println("Sum & Average:");
System.out.println(sum);
System.out.println(avg);
System.out.println(ok);
`
    },
    strings_shadow: {
      lang: "c",
      code:
`// Lexical Shadowing & String Literals
int x = 10;
printf("Outer before block:");
printf(x);

{
    int x = 99;
    printf("Inner shadowed x:");
    printf(x);
}

printf("Outer after block:");
printf(x);
`
    },
    error_demo: {
      lang: "c",
      code:
`// Strict C Mode & Semantic Type Error Demo
int whole = "not_an_int";
boolean wrongKeyword = true;
if ("not_a_bool") {
    cout << whole;
}
`
    },
    runtime_guard: {
      lang: "c",
      code:
`// Runtime Infinite-Loop Step Cap Demo
int i = 0;
while (i < 5) {
    i = 0;
}
printf(i);
`
    }
  };

  const langSelect = document.getElementById("langSelect");
  const presetSelect = document.getElementById("presetSelect");
  const codeEditor = document.getElementById("codeEditor");
  const lineNumbers = document.getElementById("lineNumbers");
  const fileLabel = document.getElementById("fileLabel");
  const runBtn = document.getElementById("runBtn");
  const clearBtn = document.getElementById("clearBtn");
  const statusBadge = document.getElementById("statusBadge");
  const metaStats = document.getElementById("metaStats");

  const tabs = document.querySelectorAll(".tab");
  const panes = {
    output: document.getElementById("pane-output"),
    tac: document.getElementById("pane-tac"),
    ast: document.getElementById("pane-ast"),
    symbols: document.getElementById("pane-symbols"),
    tokens: document.getElementById("pane-tokens"),
    errors: document.getElementById("pane-errors"),
  };

  const counts = {
    output: document.getElementById("count-output"),
    tac: document.getElementById("count-tac"),
    symbols: document.getElementById("count-symbols"),
    tokens: document.getElementById("count-tokens"),
    errors: document.getElementById("count-errors"),
  };

  function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined && text !== null) node.textContent = String(text);
    return node;
  }

  function updateLineNumbers() {
    const lines = codeEditor.value.split("\n").length;
    const nums = [];
    for (let i = 1; i <= lines; i++) {
      nums.push(i);
    }
    lineNumbers.textContent = nums.join("\n");
  }

  function updateFileLabel() {
    const extMap = { c: ".c", cpp: ".cpp", java: ".java", auto: ".txt" };
    fileLabel.textContent = "main" + (extMap[langSelect.value] || ".txt");
  }

  function activateTab(tabName) {
    tabs.forEach((btn) => {
      btn.classList.toggle("active", btn.dataset.tab === tabName);
    });
    Object.entries(panes).forEach(([name, pane]) => {
      pane.classList.toggle("active", name === tabName);
    });
  }

  function setStatus(type, text) {
    statusBadge.className = "badge badge-" + type;
    statusBadge.textContent = text;
  }

  function renderOutput(data) {
    const pane = panes.output;
    pane.innerHTML = "";

    const outputs = Array.isArray(data.output) ? data.output : [];
    const runtimeError = data.runtimeError || null;
    const errors = Array.isArray(data.errors) ? data.errors : [];

    if (errors.length > 0 && outputs.length === 0 && !runtimeError) {
      pane.appendChild(
        el("div", "empty-state", "Compilation failed. Check the Errors tab for diagnostics.")
      );
      return;
    }

    const box = el("div", "terminal-box");
    if (outputs.length === 0 && !runtimeError) {
      box.appendChild(el("div", "empty-state", "(program exited with no output)"));
    } else {
      outputs.forEach((line) => {
        const row = el("div", "terminal-line");
        row.appendChild(el("span", "terminal-prefix", ">"));
        row.appendChild(document.createTextNode(line));
        box.appendChild(row);
      });
    }
    pane.appendChild(box);

    if (runtimeError) {
      pane.appendChild(el("div", "runtime-alert", runtimeError));
    }
  }

  function renderTable(pane, headers, rows, emptyMessage, rowClassFn) {
    pane.innerHTML = "";
    if (!rows || rows.length === 0) {
      pane.appendChild(el("div", "empty-state", emptyMessage));
      return;
    }

    const table = el("table", "data-table");
    const thead = el("thead");
    const headRow = el("tr");
    headers.forEach((h) => headRow.appendChild(el("th", "", h)));
    thead.appendChild(headRow);
    table.appendChild(thead);

    const tbody = el("tbody");
    rows.forEach((cells, idx) => {
      const tr = el("tr", rowClassFn ? rowClassFn(idx) : "");
      cells.forEach((cell) => tr.appendChild(el("td", "", cell)));
      tbody.appendChild(tr);
    });
    table.appendChild(tbody);
    pane.appendChild(table);
  }

  function renderTAC(data) {
    const tac = Array.isArray(data.tac) ? data.tac : [];
    const rows = tac.map((instr) => [
      instr.index,
      instr.op,
      instr.text,
      instr.line,
    ]);
    renderTable(
      panes.tac,
      ["#", "Op", "Instruction", "Line"],
      rows,
      "No TAC instructions generated.",
      (idx) => (tac[idx].op === "LABEL" ? "tac-label" : "")
    );
  }

  function renderSymbols(data) {
    const symbols = Array.isArray(data.symbols) ? data.symbols : [];
    const rows = symbols.map((s) => [
      s.name,
      s.type,
      `${s.scope}#${s.scopeId}`,
      s.scopeDepth,
      s.line,
      s.initialized ? "yes" : "no",
      s.used ? "yes" : "no",
    ]);
    renderTable(
      panes.symbols,
      ["Name", "Type", "Scope", "Depth", "Line", "Init", "Used"],
      rows,
      "No symbols declared."
    );
  }

  function renderTokens(data) {
    const tokens = Array.isArray(data.tokens) ? data.tokens : [];
    const rows = tokens.map((t, idx) => [
      idx + 1,
      t.type,
      t.lexeme,
      `${t.line}:${t.column}`,
    ]);
    renderTable(
      panes.tokens,
      ["#", "Token Type", "Lexeme", "Pos"],
      rows,
      "No tokens produced."
    );
  }

  function buildAstElement(node, label, isRoot = false) {
    const container = el("div", isRoot ? "ast-node ast-root" : "ast-node");
    if (!node) {
      const line = el("div");
      if (label) line.appendChild(el("span", "ast-meta", label + ": "));
      line.appendChild(el("span", "ast-meta", "null"));
      container.appendChild(line);
      return container;
    }

    const details = [];
    if (node.name !== undefined) details.push(node.name);
    if (node.varType !== undefined) details.push(node.varType);
    if (node.op !== undefined) details.push(`'${node.op}'`);
    if (node.value !== undefined) details.push(JSON.stringify(node.value));
    if (node.flavor !== undefined) details.push(node.flavor);

    const children = [];
    if (Array.isArray(node.statements)) {
      node.statements.forEach((stmt, i) => children.push([`stmt[${i}]`, stmt]));
    }
    ["init", "condition", "update", "thenBranch", "elseBranch", "body", "initializer", "value", "expression", "left", "right", "operand"].forEach((key) => {
      if (node[key] !== undefined && typeof node[key] === "object") {
        if (node[key] !== null || key === "elseBranch") {
          if (node[key] !== null) {
            children.push([key, node[key]]);
          }
        }
      }
    });

    const headerSpan = el("span");
    if (label) headerSpan.appendChild(el("span", "ast-meta", label + ": "));
    headerSpan.appendChild(el("span", "ast-type", node.nodeType || "Node"));
    if (details.length > 0) {
      headerSpan.appendChild(el("span", "ast-detail", ` (${details.join(", ")})`));
    }
    const metaParts = [];
    if (node.inferredType) metaParts.push(`: ${node.inferredType}`);
    if (node.resolvedScopeId !== undefined && node.resolvedScopeId >= 0) {
      metaParts.push(`scope#${node.resolvedScopeId}`);
    } else if (node.scopeId !== undefined && node.scopeId >= 0) {
      metaParts.push(`scope#${node.scopeId}`);
    }
    if (node.line !== undefined) metaParts.push(`line ${node.line}`);
    if (metaParts.length > 0) {
      headerSpan.appendChild(el("span", "ast-meta", ` [${metaParts.join(" | ")}]`));
    }

    if (children.length > 0) {
      const detailsEl = document.createElement("details");
      detailsEl.open = true;
      const summary = el("summary", "ast-summary");
      summary.appendChild(headerSpan);
      detailsEl.appendChild(summary);
      children.forEach(([childLabel, childNode]) => {
        detailsEl.appendChild(buildAstElement(childNode, childLabel, false));
      });
      container.appendChild(detailsEl);
    } else {
      const leaf = el("div", "ast-summary");
      leaf.appendChild(headerSpan);
      container.appendChild(leaf);
    }

    return container;
  }

  function renderAST(data) {
    const pane = panes.ast;
    pane.innerHTML = "";
    if (!data.ast) {
      pane.appendChild(el("div", "empty-state", "No AST available."));
      return;
    }
    const tree = el("div", "ast-tree");
    tree.appendChild(buildAstElement(data.ast, "", true));
    pane.appendChild(tree);
  }

  function renderErrors(data) {
    const pane = panes.errors;
    pane.innerHTML = "";
    const errors = Array.isArray(data.errors) ? data.errors : [];
    const runtimeError = data.runtimeError || null;

    if (errors.length === 0 && !runtimeError) {
      pane.appendChild(el("div", "empty-state", "0 errors — clean compilation and execution."));
      return;
    }

    const list = el("div", "error-list");
    errors.forEach((err) => {
      const card = el("div", "error-card");
      const phase = err.phase || "Compiler Error";
      const loc = err.line ? ` (Line ${err.line}${err.column ? `, Col ${err.column}` : ""})` : "";
      card.appendChild(el("div", "error-phase", `${phase}${loc}`));
      card.appendChild(el("div", "", err.message || "Unknown error"));
      list.appendChild(card);
    });

    if (runtimeError) {
      const card = el("div", "runtime-alert", runtimeError);
      list.appendChild(card);
    }

    pane.appendChild(list);
  }

  async function compileAndRun() {
    runBtn.disabled = true;
    setStatus("idle", "Compiling...");

    const started = performance.now();
    try {
      const response = await fetch("server.php", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          code: codeEditor.value,
          language: langSelect.value,
        }),
      });

      const data = await response.json();
      const elapsed = Math.round(performance.now() - started);

      const errCount = (Array.isArray(data.errors) ? data.errors.length : 0) + (data.runtimeError ? 1 : 0);
      const outCount = Array.isArray(data.output) ? data.output.length : 0;
      const tacCount = Array.isArray(data.tac) ? data.tac.length : 0;
      const symCount = Array.isArray(data.symbols) ? data.symbols.length : 0;
      const tokCount = Array.isArray(data.tokens) ? data.tokens.length : 0;

      counts.output.textContent = String(outCount);
      counts.tac.textContent = String(tacCount);
      counts.symbols.textContent = String(symCount);
      counts.tokens.textContent = String(tokCount);
      counts.errors.textContent = String(errCount);
      counts.errors.classList.toggle("has-errors", errCount > 0);

      metaStats.textContent = `Tokens: ${tokCount} | TAC: ${tacCount} | Time: ${elapsed} ms`;

      renderOutput(data);
      renderTAC(data);
      renderAST(data);
      renderSymbols(data);
      renderTokens(data);
      renderErrors(data);

      if (data.success) {
        setStatus("success", `Success (${data.language || langSelect.value})`);
        activateTab("output");
      } else if (data.runtimeError) {
        setStatus("runtime", "Runtime Error");
        activateTab("output");
      } else {
        setStatus("error", `Failed (${errCount} error${errCount === 1 ? "" : "s"})`);
        activateTab("errors");
      }
    } catch (err) {
      setStatus("error", "Server Unreachable");
      renderErrors({
        errors: [
          {
            phase: "Network Error",
            line: 0,
            column: 0,
            message: "Could not reach server.php. Run PHP server or XAMPP from src/web/.",
          },
        ],
      });
      activateTab("errors");
    } finally {
      runBtn.disabled = false;
    }
  }

  // Event Wiring
  tabs.forEach((btn) => {
    btn.addEventListener("click", () => activateTab(btn.dataset.tab));
  });

  codeEditor.addEventListener("input", updateLineNumbers);
  codeEditor.addEventListener("scroll", () => {
    lineNumbers.scrollTop = codeEditor.scrollTop;
  });

  codeEditor.addEventListener("keydown", (e) => {
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
      e.preventDefault();
      compileAndRun();
      return;
    }
    if (e.key === "Tab") {
      e.preventDefault();
      const start = codeEditor.selectionStart;
      const end = codeEditor.selectionEnd;
      codeEditor.value =
        codeEditor.value.substring(0, start) + "    " + codeEditor.value.substring(end);
      codeEditor.selectionStart = codeEditor.selectionEnd = start + 4;
      updateLineNumbers();
    }
  });

  langSelect.addEventListener("change", updateFileLabel);

  presetSelect.addEventListener("change", () => {
    const preset = PRESETS[presetSelect.value];
    if (!preset) return;
    langSelect.value = preset.lang;
    codeEditor.value = preset.code;
    updateFileLabel();
    updateLineNumbers();
  });

  clearBtn.addEventListener("click", () => {
    codeEditor.value = "";
    updateLineNumbers();
    codeEditor.focus();
  });

  runBtn.addEventListener("click", compileAndRun);

  // Load initial preset
  const initial = PRESETS.cpp_demo;
  langSelect.value = initial.lang;
  codeEditor.value = initial.code;
  updateFileLabel();
  updateLineNumbers();
})();