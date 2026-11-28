(() => {
  "use strict";

  /* =====================================================================
     Data
     ===================================================================== */

  const PRESETS = {
    cpp_demo: {
      name: "Scoping & conditions",
      desc: "Block scope, an if statement and cout",
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
      name: "While loop & arithmetic",
      desc: "A counter loop, branching and printf",
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
      name: "For loop & float math",
      desc: "Widening int to float, println",
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
      name: "Strings & shadowing",
      desc: "The same name in nested blocks",
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
      name: "Error diagnostics",
      desc: "Type errors that strict mode rejects",
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
      name: "Infinite-loop guard",
      desc: "The runtime step cap stops a loop",
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

  const LANGS = {
    c:    { label: "C",    ext: ".c" },
    cpp:  { label: "C++",  ext: ".cpp" },
    java: { label: "Java", ext: ".java" },
    auto: { label: "Auto", ext: ".txt" },
  };

  const STAGES = [
    { id: "lexer",    view: "tokens",  label: "Lexer" },
    { id: "parser",   view: "ast",     label: "Parser" },
    { id: "semantic", view: "symbols", label: "Semantics" },
    { id: "tac",      view: "tac",     label: "TAC" },
    { id: "run",      view: "output",  label: "Run" },
  ];

  const VIEW_INFO = {
    tokens:  { title: "Tokens",              hint: "The lexer splits the source text into tokens." },
    ast:     { title: "Syntax tree",         hint: "The parser arranges tokens into statements and expressions." },
    symbols: { title: "Symbol table",        hint: "Semantic analysis records each declaration and its scope." },
    tac:     { title: "Three-address code",  hint: "Intermediate instructions generated from the tree." },
    output:  { title: "Program output",      hint: "What the program printed when it ran." },
  };

  const KEYWORDS = new Set(["if", "else", "while", "for", "do", "return", "break", "continue", "true", "false", "null",
    "class", "public", "private", "static", "final", "const", "new", "using", "namespace", "include", "switch", "case", "default"]);
  const TYPES = new Set(["int", "float", "double", "char", "bool", "boolean", "void", "long", "short", "string", "String", "auto"]);
  const BUILTINS = new Set(["cout", "cin", "endl", "printf", "scanf", "println", "print", "System", "out", "std"]);

  /* =====================================================================
     Helpers
     ===================================================================== */

  const $ = (id) => document.getElementById(id);
  const root = document.documentElement;

  const store = {
    get(key, fallback) {
      try { const v = localStorage.getItem(key); return v === null ? fallback : v; } catch (e) { return fallback; }
    },
    set(key, value) {
      try { localStorage.setItem(key, value); } catch (e) { /* storage unavailable */ }
    },
  };

  function el(tag, className, text) {
    const node = document.createElement(tag);
    if (className) node.className = className;
    if (text !== undefined && text !== null) node.textContent = String(text);
    return node;
  }

  function esc(s) {
    return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
  }

  function arr(v) { return Array.isArray(v) ? v : []; }

  let toastTimer = null;
  function toast(message) {
    let t = document.querySelector(".toast");
    if (!t) { t = el("div", "toast"); t.setAttribute("role", "status"); document.body.appendChild(t); }
    t.textContent = message;
    void t.offsetWidth;
    t.classList.add("show");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => t.classList.remove("show"), 1600);
  }

  async function copyText(text, message) {
    try {
      await navigator.clipboard.writeText(text);
    } catch (e) {
      const ta = document.createElement("textarea");
      ta.value = text;
      ta.style.position = "fixed";
      ta.style.opacity = "0";
      document.body.appendChild(ta);
      ta.select();
      try { document.execCommand("copy"); } catch (err) { /* ignore */ }
      ta.remove();
    }
    toast(message || "Copied");
  }

  /* =====================================================================
     Elements
     ===================================================================== */

  const shell = $("shell");
  const langSeg = $("langSeg");
  const autoRunBox = $("autoRun");
  const themeBtn = $("themeBtn");
  const clearBtn = $("clearBtn");
  const runBtn = $("runBtn");
  const runLabel = $("runLabel");
  const sideToggle = $("sideToggle");
  const presetList = $("presetList");
  const historyList = $("historyList");
  const clearHistoryBtn = $("clearHistory");
  const fileLabel = $("fileLabel");
  const staleChip = $("staleChip");
  const copyCodeBtn = $("copyCodeBtn");
  const downloadBtn = $("downloadBtn");
  const editorBody = $("editorBody");
  const gutter = $("gutter");
  const gutterInner = $("gutterInner");
  const highlight = $("highlight");
  const hlInner = $("hlInner");
  const codeEditor = $("codeEditor");
  const problems = $("problems");
  const problemsToggle = $("problemsToggle");
  const problemsCount = $("problemsCount");
  const problemsHint = $("problemsHint");
  const problemsList = $("problemsList");
  const splitter = $("splitter");
  const inspector = $("inspector");
  const stepper = $("stepper");
  const viewTitle = $("viewTitle");
  const viewHint = $("viewHint");
  const tokenFilter = $("tokenFilter");
  const tokChips = $("tokChips");
  const tokTable = $("tokTable");
  const astExpand = $("astExpand");
  const astCollapse = $("astCollapse");
  const copyTacBtn = $("copyTacBtn");
  const copyOutBtn = $("copyOutBtn");
  const statusBadge = $("statusBadge");
  const statStats = $("statStats");
  const statCursor = $("statCursor");
  const statLang = $("statLang");

  const panes = {
    output:  $("pane-output"),
    tokens:  $("pane-tokens"),
    ast:     $("pane-ast"),
    symbols: $("pane-symbols"),
    tac:     $("pane-tac"),
  };

  /* =====================================================================
     State
     ===================================================================== */

  let lang = "cpp";
  let activeView = "output";
  let activePreset = null;
  let lastData = null;
  let edited = false;
  let running = false;
  let rerunQueued = false;
  let autoTimer = null;
  let tokenMode = "chips";
  let errorLines = new Map();
  let hlLines = [];
  let glLines = [];
  let focusedLine = 0;
  let history = [];

  /* =====================================================================
     Syntax highlighting (line by line, tracks block comments)
     ===================================================================== */

  const TOKEN_RE = /(\/\/.*)|(\/\*)|("(?:\\.|[^"\\])*"?)|('(?:\\.|[^'\\])*'?)|(\b\d+(?:\.\d+)?[fFlL]?\b)|([A-Za-z_]\w*)|(<<|>>|<=|>=|==|!=|&&|\|\||[-+*\/%=<>!&|^~?:]+)|([{}()\[\];,.])/g;

  function span(cls, text) {
    return '<span class="' + cls + '">' + esc(text) + "</span>";
  }

  function highlightLine(line, st) {
    let html = "";
    let i = 0;

    if (st.block) {
      const end = line.indexOf("*/");
      if (end === -1) return span("t-com", line);
      html += span("t-com", line.slice(0, end + 2));
      i = end + 2;
      st.block = false;
    }

    const re = new RegExp(TOKEN_RE.source, "g");
    re.lastIndex = i;
    let m;
    while ((m = re.exec(line))) {
      if (m.index > i) html += esc(line.slice(i, m.index));
      i = m.index + m[0].length;

      if (m[1]) {
        html += span("t-com", m[1]);
      } else if (m[2]) {
        const end = line.indexOf("*/", m.index + 2);
        if (end === -1) {
          html += span("t-com", line.slice(m.index));
          st.block = true;
          i = line.length;
          break;
        }
        html += span("t-com", line.slice(m.index, end + 2));
        i = end + 2;
        re.lastIndex = i;
      } else if (m[3] || m[4]) {
        html += span("t-str", m[0]);
      } else if (m[5]) {
        html += span("t-num", m[0]);
      } else if (m[6]) {
        const w = m[6];
        if (TYPES.has(w)) html += span("t-ty", w);
        else if (KEYWORDS.has(w)) html += span("t-kw", w);
        else if (BUILTINS.has(w)) html += span("t-bi", w);
        else if (/^\s*\(/.test(line.slice(i))) html += span("t-fn", w);
        else html += esc(w);
      } else if (m[7]) {
        html += span("t-op", m[0]);
      } else if (m[8]) {
        html += span("t-pun", m[0]);
      }
    }
    if (i < line.length) html += esc(line.slice(i));
    return html;
  }

  /* =====================================================================
     Editor rendering and behaviour
     ===================================================================== */

  function renderEditor() {
    const lines = codeEditor.value.split("\n");
    const st = { block: false };
    let code = "";
    let nums = "";
    for (let n = 0; n < lines.length; n++) {
      const isErr = errorLines.has(n + 1);
      code += '<div class="hl-line' + (isErr ? " err" : "") + '">' + (highlightLine(lines[n], st) || "&#8203;") + "</div>";
      nums += '<div class="gl' + (isErr ? " err" : "") + '" data-line="' + (n + 1) + '"' +
              (isErr ? ' title="' + esc(errorLines.get(n + 1)).replace(/"/g, "&quot;") + '"' : "") + ">" + (n + 1) + "</div>";
    }
    hlInner.innerHTML = code;
    gutterInner.innerHTML = nums;
    hlLines = Array.from(hlInner.children);
    glLines = Array.from(gutterInner.children);
    focusedLine = 0;
    syncScroll();
  }

  function syncScroll() {
    highlight.scrollTop = codeEditor.scrollTop;
    highlight.scrollLeft = codeEditor.scrollLeft;
    gutter.scrollTop = codeEditor.scrollTop;
    editorBody.style.setProperty("--sy", codeEditor.scrollTop + "px");
  }

  function setFocusLine(n) {
    if (focusedLine && hlLines[focusedLine - 1]) {
      hlLines[focusedLine - 1].classList.remove("focus");
      glLines[focusedLine - 1].classList.remove("focus");
    }
    focusedLine = n > 0 && n <= hlLines.length ? n : 0;
    if (focusedLine) {
      hlLines[focusedLine - 1].classList.add("focus");
      glLines[focusedLine - 1].classList.add("focus");
    }
  }

  function revealLine(line) {
    const top = 8 + (line - 1) * 22;
    const h = codeEditor.clientHeight;
    if (top < codeEditor.scrollTop + 22 || top > codeEditor.scrollTop + h - 66) {
      codeEditor.scrollTop = Math.max(0, top - h / 3);
    }
  }

  function jumpTo(line, col) {
    if (!line || line < 1) return;
    const lines = codeEditor.value.split("\n");
    line = Math.min(line, lines.length);
    let offset = 0;
    for (let i = 0; i < line - 1; i++) offset += lines[i].length + 1;
    const c = Math.min(Math.max((col || 1) - 1, 0), lines[line - 1].length);
    codeEditor.focus();
    codeEditor.setSelectionRange(offset + c, offset + c);
    revealLine(line);
    syncScroll();
    updateCursor();
    const target = hlLines[line - 1];
    if (target) {
      target.classList.remove("flash");
      void target.offsetWidth;
      target.classList.add("flash");
    }
  }

  function updateCursor() {
    const before = codeEditor.value.slice(0, codeEditor.selectionStart);
    const line = before.split("\n").length;
    const col = codeEditor.selectionStart - before.lastIndexOf("\n");
    statCursor.textContent = "Ln " + line + ", Col " + col;
  }

  /* Edit through execCommand when possible so Ctrl+Z keeps working */
  function insertText(text) {
    codeEditor.focus();
    let ok = false;
    try { ok = document.execCommand("insertText", false, text); } catch (e) { ok = false; }
    if (!ok) {
      codeEditor.setRangeText(text, codeEditor.selectionStart, codeEditor.selectionEnd, "end");
      codeEditor.dispatchEvent(new Event("input", { bubbles: true }));
    }
  }

  function lineBounds() {
    const v = codeEditor.value;
    const s = v.lastIndexOf("\n", codeEditor.selectionStart - 1) + 1;
    let e = v.indexOf("\n", codeEditor.selectionEnd);
    if (e === -1) e = v.length;
    // A selection that ends at the very start of a line should not include that line
    if (codeEditor.selectionEnd > codeEditor.selectionStart && v[codeEditor.selectionEnd - 1] === "\n") {
      e = codeEditor.selectionEnd - 1;
    }
    return [s, e];
  }

  function transformLines(fn) {
    const [s, e] = lineBounds();
    const block = codeEditor.value.slice(s, e);
    const next = block.split("\n").map(fn).join("\n");
    codeEditor.setSelectionRange(s, e);
    insertText(next);
    codeEditor.setSelectionRange(s, s + next.length);
  }

  function onEditorKey(e) {
    if ((e.ctrlKey || e.metaKey) && e.key === "Enter") {
      e.preventDefault();
      run();
      return;
    }

    if ((e.ctrlKey || e.metaKey) && e.key === "/") {
      e.preventDefault();
      const [s, e2] = lineBounds();
      const lines = codeEditor.value.slice(s, e2).split("\n");
      const allCommented = lines.every((l) => /^\s*\/\//.test(l) || l.trim() === "");
      transformLines((l) => (allCommented ? l.replace(/^(\s*)\/\/ ?/, "$1") : l.replace(/^(\s*)/, "$1// ")));
      return;
    }

    if (e.key === "Tab") {
      e.preventDefault();
      const multi = codeEditor.value.slice(codeEditor.selectionStart, codeEditor.selectionEnd).includes("\n");
      if (e.shiftKey) {
        transformLines((l) => l.replace(/^( {1,4}|\t)/, ""));
      } else if (multi) {
        transformLines((l) => "    " + l);
      } else {
        insertText("    ");
      }
      return;
    }

    if (e.key === "Enter" && !e.shiftKey && !e.altKey) {
      const v = codeEditor.value;
      const start = codeEditor.selectionStart;
      const end = codeEditor.selectionEnd;
      const lineStart = v.lastIndexOf("\n", start - 1) + 1;
      const indent = (v.slice(lineStart, start).match(/^[ \t]*/) || [""])[0];
      const prev = v[start - 1];
      const next = v[end];
      e.preventDefault();
      if (prev === "{" && next === "}") {
        insertText("\n" + indent + "    " + "\n" + indent);
        const pos = start + 1 + indent.length + 4;
        codeEditor.setSelectionRange(pos, pos);
      } else if (prev === "{") {
        insertText("\n" + indent + "    ");
      } else {
        insertText("\n" + indent);
      }
      return;
    }
  }

  function onEditorInput() {
    errorLines = new Map(); // old error markers no longer match the text
    renderEditor();
    updateCursor();
    store.set("cf.code", codeEditor.value);
    if (activePreset) { activePreset = null; renderPresets(); }
    if (lastData && !edited) { edited = true; staleChip.hidden = false; }

    clearTimeout(autoTimer);
    if (autoRunBox.checked) autoTimer = setTimeout(run, 800);
  }

  /* =====================================================================
     Language, theme, sidebar
     ===================================================================== */

  function setLang(next, persist = true) {
    if (!LANGS[next]) next = "cpp";
    lang = next;
    langSeg.querySelectorAll(".seg-btn").forEach((b) => {
      const on = b.dataset.lang === lang;
      b.classList.toggle("active", on);
      b.setAttribute("aria-checked", on ? "true" : "false");
    });
    fileLabel.textContent = "main" + LANGS[lang].ext;
    statLang.textContent = LANGS[lang].label + " · 4 spaces";
    if (persist) store.set("cf.lang", lang);
  }

  function setTheme(theme) {
    root.setAttribute("data-theme", theme);
    store.set("cf.theme", theme);
  }

  function setSidebar(collapsed) {
    shell.classList.toggle("side-collapsed", collapsed);
    store.set("cf.side", collapsed ? "0" : "1");
  }

  /* =====================================================================
     Sidebar: presets and history
     ===================================================================== */

  function renderPresets() {
    presetList.replaceChildren();
    Object.entries(PRESETS).forEach(([key, p]) => {
      const btn = el("button", "preset" + (key === activePreset ? " active" : ""));
      btn.type = "button";
      const name = el("span", "preset-name");
      name.appendChild(el("span", "preset-lang", LANGS[p.lang].label));
      name.appendChild(document.createTextNode(p.name));
      btn.appendChild(name);
      btn.appendChild(el("span", "preset-desc", p.desc));
      btn.addEventListener("click", () => loadPreset(key));
      presetList.appendChild(btn);
    });
  }

  function loadPreset(key) {
    const p = PRESETS[key];
    if (!p) return;
    setLang(p.lang);
    codeEditor.value = p.code;
    errorLines = new Map();
    renderEditor();
    updateCursor();
    store.set("cf.code", p.code);
    activePreset = key;
    renderPresets();
    markFresh();
  }

  function markFresh() {
    edited = false;
    staleChip.hidden = true;
  }

  function loadHistory() {
    try { history = JSON.parse(store.get("cf.history", "[]")) || []; } catch (e) { history = []; }
    if (!Array.isArray(history)) history = [];
  }

  function saveHistory() { store.set("cf.history", JSON.stringify(history.slice(0, 8))); }

  function pushHistory(ok) {
    const code = codeEditor.value;
    if (!code.trim()) return;
    if (history[0] && history[0].code === code && history[0].lang === lang) {
      history[0].ok = ok;
      history[0].ts = Date.now();
    } else {
      history.unshift({ code, lang, ok, ts: Date.now() });
    }
    history = history.slice(0, 8);
    saveHistory();
    renderHistory();
  }

  function historyTitle(code) {
    const lines = code.split("\n").map((l) => l.trim()).filter(Boolean);
    const first = lines.find((l) => !l.startsWith("//")) || lines[0] || "(empty)";
    return first.length > 34 ? first.slice(0, 33) + "…" : first;
  }

  function renderHistory() {
    historyList.replaceChildren();
    if (history.length === 0) {
      historyList.appendChild(el("div", "side-empty", "Nothing yet. Your last eight runs appear here."));
      return;
    }
    history.forEach((h) => {
      const btn = el("button", "hist");
      btn.type = "button";
      btn.appendChild(el("span", "hist-dot " + (h.ok ? "ok" : "bad")));
      btn.appendChild(el("span", "hist-title", historyTitle(h.code)));
      const d = new Date(h.ts);
      const time = String(d.getHours()).padStart(2, "0") + ":" + String(d.getMinutes()).padStart(2, "0");
      btn.appendChild(el("span", "hist-meta", (LANGS[h.lang] ? LANGS[h.lang].label : "") + " · " + time));
      btn.title = h.ok ? "Ran without errors" : "Had errors";
      btn.addEventListener("click", () => {
        setLang(h.lang);
        codeEditor.value = h.code;
        errorLines = new Map();
        renderEditor();
        updateCursor();
        store.set("cf.code", h.code);
        activePreset = null;
        renderPresets();
        markFresh();
      });
      historyList.appendChild(btn);
    });
  }

  /* =====================================================================
     Views
     ===================================================================== */

  function empty(title, text, actionLabel, action) {
    const box = el("div", "empty");
    box.appendChild(el("b", "", title));
    box.appendChild(el("p", "", text));
    if (actionLabel) {
      const b = el("button", "link-btn", actionLabel);
      b.type = "button";
      b.addEventListener("click", action);
      box.appendChild(b);
    }
    return box;
  }

  function activateView(view) {
    activeView = view;
    inspector.dataset.view = view;
    Object.entries(panes).forEach(([name, pane]) => pane.classList.toggle("active", name === view));
    stepper.querySelectorAll(".step").forEach((s) => s.classList.toggle("active", s.dataset.view === view));
    viewTitle.textContent = VIEW_INFO[view].title;
    viewHint.textContent = VIEW_INFO[view].hint;
  }

  function resetViews() {
    const waiting = {
      output:  ["Nothing has run yet", "Press Compile & run (or Ctrl + Enter) to execute the program in the editor."],
      tokens:  ["No tokens yet", "Compile the program to see how the lexer splits it into tokens."],
      ast:     ["No syntax tree yet", "Compile the program to see the tree the parser builds."],
      symbols: ["No symbols yet", "Compile the program to see every declared name and its scope."],
      tac:     ["No intermediate code yet", "Compile the program to see its three-address code."],
    };
    Object.entries(waiting).forEach(([view, [t, p]]) => {
      panes[view].replaceChildren(empty(t, p));
    });
  }

  function table(headers, rows, opts) {
    const t = el("table", "tbl");
    const thead = el("thead");
    const hr = el("tr");
    headers.forEach((h) => hr.appendChild(el("th", "", h)));
    thead.appendChild(hr);
    t.appendChild(thead);
    const tb = el("tbody");
    rows.forEach((cells, i) => {
      const tr = el("tr");
      cells.forEach((c, ci) => {
        const td = c instanceof Node ? el("td") : el("td", opts && opts.numCols && opts.numCols.includes(ci) ? "num" : "", c);
        if (c instanceof Node) td.appendChild(c);
        tr.appendChild(td);
      });
      if (opts && opts.lineOf) {
        const ln = opts.lineOf(i);
        if (ln > 0) bindLine(tr, ln, opts.colOf ? opts.colOf(i) : 1);
      }
      tb.appendChild(tr);
    });
    t.appendChild(tb);
    return t;
  }

  function bindLine(node, line, col) {
    node.dataset.line = line;
    node.addEventListener("mouseenter", () => setFocusLine(line));
    node.addEventListener("mouseleave", () => setFocusLine(0));
    node.addEventListener("click", () => jumpTo(line, col));
  }

  /* ---- Output ---- */
  function renderOutput(data) {
    const pane = panes.output;
    pane.replaceChildren();
    const outputs = arr(data.output);
    const errors = arr(data.errors);
    const runtimeError = data.runtimeError || null;

    if (errors.length > 0 && outputs.length === 0 && !runtimeError) {
      pane.appendChild(empty(
        "The program did not compile",
        errors.length + (errors.length === 1 ? " problem was" : " problems were") + " found, so nothing ran. Each one is listed under the editor with its line.",
        "Show problems",
        () => { setProblemsOpen(true); problemsToggle.focus(); }
      ));
      return;
    }

    const sheet = el("div", "out-sheet");
    if (outputs.length === 0) {
      sheet.appendChild(el("div", "out-empty", "The program finished without printing anything."));
    } else {
      outputs.forEach((line, i) => {
        const row = el("div", "out-line");
        row.appendChild(el("span", "out-no", i + 1));
        row.appendChild(el("span", "out-text", line));
        sheet.appendChild(row);
      });
    }
    pane.appendChild(sheet);

    if (runtimeError) {
      const a = el("div", "alert");
      a.appendChild(el("b", "", "Stopped at run time"));
      a.appendChild(document.createTextNode(runtimeError));
      pane.appendChild(a);
    }
  }

  /* ---- Tokens ---- */
  function tokenClass(t) {
    const lx = String(t.lexeme == null ? "" : t.lexeme);
    if (/^[A-Za-z_]/.test(lx)) {
      if (TYPES.has(lx)) return "tk-ty";
      if (KEYWORDS.has(lx) || BUILTINS.has(lx)) return "tk-kw";
      return "tk-id";
    }
    if (/^\d/.test(lx)) return "tk-num";
    if (/^["']/.test(lx)) return "tk-str";
    if (/^[{}()\[\];,.]$/.test(lx)) return "tk-pun";
    return "tk-op";
  }

  function renderTokens() {
    const pane = panes.tokens;
    pane.replaceChildren();
    if (!lastData) { pane.appendChild(empty("No tokens yet", "Compile the program to see how the lexer splits it into tokens.")); return; }
    const all = arr(lastData.tokens);
    if (all.length === 0) {
      pane.appendChild(empty("No tokens were produced", "The lexer stopped before it found any tokens. Check the Problems list for the first error."));
      return;
    }
    const q = tokenFilter.value.trim().toLowerCase();
    const list = all.map((t, i) => ({ t, i })).filter(({ t }) =>
      !q || String(t.lexeme).toLowerCase().includes(q) || String(t.type).toLowerCase().includes(q));

    pane.appendChild(el("div", "tok-count", q ? list.length + " of " + all.length + " tokens match" : all.length + " tokens"));
    if (list.length === 0) { pane.appendChild(empty("No tokens match", "Try a different filter.")); return; }

    if (tokenMode === "table") {
      pane.appendChild(table(
        ["#", "Type", "Lexeme", "Position"],
        list.map(({ t, i }) => [i + 1, t.type, t.lexeme, t.line + ":" + t.column]),
        { numCols: [0], lineOf: (k) => list[k].t.line, colOf: (k) => list[k].t.column }
      ));
      return;
    }

    const wrap = el("div", "tok-wrap");
    list.forEach(({ t }) => {
      const chip = el("button", "tk " + tokenClass(t));
      chip.type = "button";
      chip.title = t.type + " at line " + t.line + ", column " + t.column;
      chip.appendChild(el("span", "tk-lex", t.lexeme === "" || t.lexeme == null ? "∅" : t.lexeme));
      chip.appendChild(el("span", "tk-type", t.type));
      if (t.line > 0) bindLine(chip, t.line, t.column);
      wrap.appendChild(chip);
    });
    pane.appendChild(wrap);
  }

  /* ---- AST ---- */
  const AST_KEYS = ["init", "condition", "update", "thenBranch", "elseBranch", "body", "initializer", "value", "expression", "left", "right", "operand"];

  function astChildren(node) {
    const out = [];
    if (Array.isArray(node.statements)) node.statements.forEach((s, i) => out.push(["stmt[" + i + "]", s]));
    AST_KEYS.forEach((k) => {
      const v = node[k];
      if (v && typeof v === "object") out.push([k, v]);
    });
    return out;
  }

  function countNodes(node) {
    if (!node || typeof node !== "object") return 0;
    return 1 + astChildren(node).reduce((n, [, c]) => n + countNodes(c), 0);
  }

  function astTone(type) {
    const t = String(type || "");
    if (/Literal|Number|String|Bool|Const/i.test(t)) return "tone-lit";
    if (/Ident|VarRef|Variable|Name/i.test(t)) return "tone-ref";
    if (/Decl|Define|Param/i.test(t)) return "tone-decl";
    if (/If|While|For|Block|Program|Compound|Loop|Print|Return/i.test(t)) return "tone-ctl";
    if (/Binary|Unary|Assign|Expr|Op|Cast/i.test(t)) return "tone-expr";
    return "tone-def";
  }

  function buildAst(node, label, isRoot) {
    const wrap = el("div", isRoot ? "ast-node ast-root" : "ast-node");
    const head = el("span");
    if (label) head.appendChild(el("span", "ast-label", label));
    head.appendChild(el("span", "ast-type " + astTone(node.nodeType), node.nodeType || "Node"));

    const details = [];
    if (node.name !== undefined) details.push(node.name);
    if (node.varType !== undefined) details.push(node.varType);
    if (node.op !== undefined) details.push("'" + node.op + "'");
    if (node.value !== undefined && typeof node.value !== "object") details.push(JSON.stringify(node.value));
    if (node.flavor !== undefined) details.push(node.flavor);
    if (details.length) head.appendChild(el("span", "ast-detail", " (" + details.join(", ") + ")"));

    const meta = [];
    if (node.inferredType) meta.push(": " + node.inferredType);
    if (node.resolvedScopeId !== undefined && node.resolvedScopeId >= 0) meta.push("scope " + node.resolvedScopeId);
    else if (node.scopeId !== undefined && node.scopeId >= 0) meta.push("scope " + node.scopeId);
    if (node.line !== undefined) meta.push("line " + node.line);
    if (meta.length) head.appendChild(el("span", "ast-meta", "  [" + meta.join(" | ") + "]"));

    const kids = astChildren(node);
    let row;
    if (kids.length) {
      const d = document.createElement("details");
      d.open = true;
      row = el("summary", "ast-row");
      row.appendChild(head);
      d.appendChild(row);
      kids.forEach(([l, c]) => d.appendChild(buildAst(c, l, false)));
      wrap.appendChild(d);
    } else {
      row = el("div", "ast-row leaf");
      row.appendChild(head);
      wrap.appendChild(row);
    }
    if (typeof node.line === "number" && node.line > 0) {
      row.addEventListener("mouseenter", () => setFocusLine(node.line));
      row.addEventListener("mouseleave", () => setFocusLine(0));
    }
    return wrap;
  }

  function renderAst(data) {
    const pane = panes.ast;
    pane.replaceChildren();
    if (!data.ast) {
      pane.appendChild(empty("No syntax tree", "The parser could not build a tree, usually because of a syntax error. Check the Problems list."));
      return;
    }
    const tree = el("div", "ast-tree");
    tree.appendChild(buildAst(data.ast, "", true));
    pane.appendChild(tree);
  }

  /* ---- Symbols ---- */
  function renderSymbols(data) {
    const pane = panes.symbols;
    pane.replaceChildren();
    const symbols = arr(data.symbols);
    if (symbols.length === 0) {
      pane.appendChild(empty("No symbols declared", "The program did not declare any variables, or analysis stopped before it recorded them."));
      return;
    }
    const groups = new Map();
    symbols.forEach((s) => {
      if (!groups.has(s.scopeId)) groups.set(s.scopeId, { scope: s.scope, depth: s.scopeDepth || 0, items: [] });
      groups.get(s.scopeId).items.push(s);
    });
    Array.from(groups.entries()).sort((a, b) => a[0] - b[0]).forEach(([id, g]) => {
      const block = el("section", "scope");
      block.style.marginLeft = Math.min(g.depth, 6) * 14 + "px";
      const head = el("div", "scope-head");
      head.appendChild(el("span", "scope-name", (g.scope ? g.scope : "scope") + " #" + id));
      head.appendChild(el("span", "scope-meta", "depth " + g.depth + " · " + g.items.length + (g.items.length === 1 ? " symbol" : " symbols")));
      block.appendChild(head);

      const rows = g.items.map((s) => [
        el("span", "sym-name", s.name),
        el("span", "sym-type", s.type),
        s.line,
        pill(s.initialized ? "yes" : "no", s.initialized ? "yes" : "no"),
        s.used ? pill("yes", "yes") : pill("never used", "warn"),
      ]);
      const tbl = table(["Name", "Type", "Line", "Initialised", "Used"], rows, {
        numCols: [2],
        lineOf: (i) => g.items[i].line,
      });
      // table() puts plain nodes inside cells as-is; fix the name/type cells that arrive as spans
      block.appendChild(tbl);
      pane.appendChild(block);
    });
  }

  function pill(text, kind) { return el("span", "pill " + kind, text); }

  /* ---- TAC ---- */
  function tacKind(op) {
    const o = String(op || "").toUpperCase();
    if (o === "LABEL") return "label";
    if (/JUMP|GOTO|IF|BRANCH|JMP|BR/.test(o)) return "control";
    if (/PRINT|OUT|CALL|RETURN|WRITE|PARAM/.test(o)) return "io";
    return "";
  }

  function renderTac(data) {
    const pane = panes.tac;
    pane.replaceChildren();
    const tac = arr(data.tac);
    if (tac.length === 0) {
      pane.appendChild(empty("No intermediate code", "Code generation did not run, or the program has no statements that produce instructions."));
      return;
    }
    const box = el("div", "tac");
    tac.forEach((ins) => {
      const row = el("div", "tac-row " + tacKind(ins.op));
      row.appendChild(el("span", "tac-idx", ins.index));
      row.appendChild(el("span", "tac-op", ins.op));
      row.appendChild(el("span", "tac-text", ins.text));
      row.appendChild(el("span", "tac-line", ins.line > 0 ? "L" + ins.line : ""));
      if (ins.line > 0) bindLine(row, ins.line, 1);
      box.appendChild(row);
    });
    pane.appendChild(box);
  }

  /* =====================================================================
     Stepper
     ===================================================================== */

  const STAGE_PATTERNS = [
    ["lexer", /lex|token|scan/i],
    ["parser", /pars|syntax/i],
    ["semantic", /semant|type|scope|symbol|declar/i],
    ["tac", /tac|codegen|code gen|intermediate/i],
    ["run", /runtime|run time|execut/i],
  ];

  function failedStage(data) {
    const errors = arr(data.errors);
    if (errors.length > 0) {
      let best = -1;
      errors.forEach((e) => {
        const phase = String(e.phase || "");
        STAGE_PATTERNS.forEach(([id, re], idx) => {
          if (re.test(phase) && (best === -1 || idx < best)) best = idx;
        });
      });
      if (best !== -1) return STAGES[best].id;
      // Unknown phase names: infer from what was produced
      if (arr(data.tokens).length === 0) return "lexer";
      if (!data.ast) return "parser";
      if (arr(data.tac).length === 0) return "semantic";
      return "tac";
    }
    if (data.runtimeError || data.success === false) return "run";
    return null;
  }

  function stageCount(id, data) {
    switch (id) {
      case "lexer":    return arr(data.tokens).length + " tokens";
      case "parser":   return data.ast ? countNodes(data.ast) + " nodes" : "no tree";
      case "semantic": return arr(data.symbols).length + " symbols";
      case "tac":      return arr(data.tac).length + " steps";
      case "run":      return arr(data.output).length + (arr(data.output).length === 1 ? " line" : " lines");
    }
    return "";
  }

  function setStep(id, status, sub) {
    const btn = stepper.querySelector('[data-stage="' + id + '"]');
    if (!btn) return;
    btn.classList.remove("idle", "ok", "fail", "skip", "running");
    btn.classList.add(status);
    const dot = btn.querySelector(".step-dot");
    const idx = STAGES.findIndex((s) => s.id === id) + 1;
    dot.textContent = status === "ok" ? "✓" : status === "fail" ? "✕" : status === "skip" ? "–" : String(idx);
    $("sub-" + id).textContent = sub;
  }

  function updateStepper(data) {
    const failed = failedStage(data);
    const failIdx = failed ? STAGES.findIndex((s) => s.id === failed) : -1;
    STAGES.forEach((s, i) => {
      if (failIdx === -1 || i < failIdx) setStep(s.id, "ok", stageCount(s.id, data));
      else if (i === failIdx) setStep(s.id, "fail", "failed");
      else setStep(s.id, "skip", "skipped");
    });
    return failed;
  }

  function resetStepper() {
    const subs = { lexer: "tokens", parser: "syntax tree", semantic: "symbols", tac: "3-address code", run: "output" };
    STAGES.forEach((s) => setStep(s.id, "idle", subs[s.id]));
  }

  function setScanning() {
    STAGES.forEach((s) => {
      const btn = stepper.querySelector('[data-stage="' + s.id + '"]');
      btn.classList.remove("ok", "fail", "skip", "idle");
      btn.classList.add("running");
    });
  }

  /* =====================================================================
     Problems
     ===================================================================== */

  function setProblemsOpen(open) {
    problems.dataset.open = open ? "true" : "false";
    problemsToggle.setAttribute("aria-expanded", open ? "true" : "false");
  }

  function renderProblems(data) {
    problemsList.replaceChildren();
    const errors = arr(data.errors);
    const runtimeError = data.runtimeError || null;
    const total = errors.length + (runtimeError ? 1 : 0);

    problemsCount.textContent = String(total);
    problemsCount.classList.toggle("bad", total > 0);

    if (total === 0) {
      problemsHint.textContent = "No problems found.";
      problemsList.appendChild(el("div", "problems-ok", "The program compiled and ran cleanly."));
      setProblemsOpen(false);
      return;
    }

    problemsHint.textContent = errors.length > 0
      ? (errors.length === 1 ? "1 compile error" : errors.length + " compile errors") + (runtimeError ? " and a run-time stop" : "")
      : "Stopped at run time";

    errors.forEach((err) => {
      const hasLine = err.line > 0;
      const btn = el("button", "problem");
      btn.type = "button";
      btn.disabled = !hasLine;
      btn.appendChild(el("span", "problem-mark", "✕"));
      const msg = el("span", "problem-msg");
      msg.appendChild(el("span", "problem-phase", err.phase || "Compiler error"));
      msg.appendChild(document.createTextNode(err.message || "Unknown error"));
      btn.appendChild(msg);
      btn.appendChild(el("span", "problem-loc", hasLine ? "Line " + err.line + (err.column ? ":" + err.column : "") : ""));
      if (hasLine) {
        btn.addEventListener("click", () => jumpTo(err.line, err.column));
        btn.addEventListener("mouseenter", () => setFocusLine(err.line));
        btn.addEventListener("mouseleave", () => setFocusLine(0));
      }
      problemsList.appendChild(btn);
    });

    if (runtimeError) {
      const row = el("div", "problem warn");
      row.appendChild(el("span", "problem-mark", "!"));
      const msg = el("span", "problem-msg");
      msg.appendChild(el("span", "problem-phase", "Run-time stop"));
      msg.appendChild(document.createTextNode(runtimeError));
      row.appendChild(msg);
      row.appendChild(el("span"));
      problemsList.appendChild(row);
    }
    setProblemsOpen(true);
  }

  /* =====================================================================
     Status
     ===================================================================== */

  function setStatus(type, text) {
    statusBadge.className = "badge badge-" + type;
    statusBadge.textContent = text;
  }

  /* =====================================================================
     Compile and run
     ===================================================================== */

  function applyResult(data, elapsed) {
    lastData = data;
    markFresh();

    const errors = arr(data.errors);
    const errCount = errors.length + (data.runtimeError ? 1 : 0);
    const tokCount = arr(data.tokens).length;
    const tacCount = arr(data.tac).length;

    statStats.textContent = "Tokens " + tokCount + " · TAC " + tacCount + " · Time " + elapsed + " ms";

    errorLines = new Map();
    errors.forEach((e) => { if (e.line > 0 && !errorLines.has(e.line)) errorLines.set(e.line, e.message || "Error"); });
    renderEditor();

    renderOutput(data);
    renderTokens();
    renderAst(data);
    renderSymbols(data);
    renderTac(data);
    renderProblems(data);
    const failed = updateStepper(data);

    if (data.success) {
      setStatus("success", "Success (" + (data.language || LANGS[lang].label) + ")");
      activateView("output");
    } else if (data.runtimeError && errors.length === 0) {
      setStatus("runtime", "Runtime error");
      activateView("output");
    } else {
      setStatus("error", "Failed (" + errCount + (errCount === 1 ? " error)" : " errors)"));
      const stage = STAGES.find((s) => s.id === failed) || STAGES[0];
      activateView(stage.view);
    }

    pushHistory(Boolean(data.success));
  }

  function showNetworkError(message) {
    lastData = null;
    setStatus("error", "Server unreachable");
    resetViews();
    resetStepper();
    problemsList.replaceChildren();
    const btn = el("div", "problem");
    btn.appendChild(el("span", "problem-mark", "✕"));
    const msg = el("span", "problem-msg");
    msg.appendChild(el("span", "problem-phase", "Network error"));
    msg.appendChild(document.createTextNode(message));
    btn.appendChild(msg);
    btn.appendChild(el("span"));
    problemsList.appendChild(btn);
    problemsCount.textContent = "1";
    problemsCount.classList.add("bad");
    problemsHint.textContent = "Could not reach the server.";
    setProblemsOpen(true);
    panes.output.replaceChildren(empty(
      "Can't reach the compiler",
      "The page could not talk to server.php. Start PHP's built-in server (or XAMPP) from the src/web folder, then try again."
    ));
    activateView("output");
  }

  async function run() {
    if (running) { rerunQueued = true; return; }
    running = true;
    clearTimeout(autoTimer);
    runBtn.disabled = true;
    runBtn.classList.add("busy");
    runLabel.textContent = "Compiling…";
    setStatus("busy", "Compiling…");
    setScanning();

    const started = performance.now();
    try {
      const response = await fetch("server.php", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ code: codeEditor.value, language: lang }),
      });
      let data;
      try {
        data = await response.json();
      } catch (e) {
        throw new Error("server.php answered with something that is not JSON (HTTP " + response.status + "). Check the PHP error log.");
      }
      applyResult(data, Math.round(performance.now() - started));
    } catch (err) {
      const offline = err instanceof TypeError;
      showNetworkError(offline
        ? "Could not reach server.php. Run the PHP server or XAMPP from src/web/."
        : err.message);
    } finally {
      running = false;
      runBtn.disabled = false;
      runBtn.classList.remove("busy");
      runLabel.textContent = "Compile & run";
      if (rerunQueued) { rerunQueued = false; run(); }
    }
  }

  /* =====================================================================
     Splitter
     ===================================================================== */

  function setSplit(ratio) {
    ratio = Math.min(0.75, Math.max(0.25, ratio));
    shell.style.setProperty("--ed-fr", (ratio * 100).toFixed(1) + "fr");
    shell.style.setProperty("--insp-fr", ((1 - ratio) * 100).toFixed(1) + "fr");
    store.set("cf.split", String(ratio));
    return ratio;
  }

  let splitRatio = 0.55;

  function initSplitter() {
    const saved = parseFloat(store.get("cf.split", ""));
    if (!isNaN(saved)) splitRatio = setSplit(saved);

    splitter.addEventListener("pointerdown", (e) => {
      e.preventDefault();
      splitter.setPointerCapture(e.pointerId);
      splitter.classList.add("dragging");
      document.body.style.userSelect = "none";
    });
    splitter.addEventListener("pointermove", (e) => {
      if (!splitter.classList.contains("dragging")) return;
      const left = $("editorCard").getBoundingClientRect().left;
      const right = $("inspector").getBoundingClientRect().right;
      splitRatio = setSplit((e.clientX - left) / (right - left));
    });
    const stop = (e) => {
      if (!splitter.classList.contains("dragging")) return;
      splitter.classList.remove("dragging");
      document.body.style.userSelect = "";
      try { splitter.releasePointerCapture(e.pointerId); } catch (err) { /* ignore */ }
    };
    splitter.addEventListener("pointerup", stop);
    splitter.addEventListener("pointercancel", stop);
    splitter.addEventListener("dblclick", () => { splitRatio = setSplit(0.55); });
    splitter.addEventListener("keydown", (e) => {
      if (e.key === "ArrowLeft") { e.preventDefault(); splitRatio = setSplit(splitRatio - 0.03); }
      if (e.key === "ArrowRight") { e.preventDefault(); splitRatio = setSplit(splitRatio + 0.03); }
    });
  }

  /* =====================================================================
     Wiring
     ===================================================================== */

  function wire() {
    codeEditor.addEventListener("input", onEditorInput);
    codeEditor.addEventListener("keydown", onEditorKey);
    codeEditor.addEventListener("scroll", syncScroll);
    ["keyup", "click", "focus", "select"].forEach((evt) => codeEditor.addEventListener(evt, updateCursor));

    gutterInner.addEventListener("click", (e) => {
      const g = e.target.closest(".gl.err");
      if (g) jumpTo(Number(g.dataset.line), 1);
    });

    langSeg.addEventListener("click", (e) => {
      const b = e.target.closest(".seg-btn");
      if (b) setLang(b.dataset.lang);
    });

    autoRunBox.addEventListener("change", () => {
      store.set("cf.auto", autoRunBox.checked ? "1" : "0");
      if (autoRunBox.checked) run();
    });

    themeBtn.addEventListener("click", () => setTheme(root.getAttribute("data-theme") === "dark" ? "light" : "dark"));
    sideToggle.addEventListener("click", () => setSidebar(!shell.classList.contains("side-collapsed")));

    clearBtn.addEventListener("click", () => {
      codeEditor.value = "";
      errorLines = new Map();
      renderEditor();
      updateCursor();
      store.set("cf.code", "");
      activePreset = null;
      renderPresets();
      codeEditor.focus();
    });

    runBtn.addEventListener("click", run);

    copyCodeBtn.addEventListener("click", () => copyText(codeEditor.value, "Source copied"));
    downloadBtn.addEventListener("click", () => {
      const blob = new Blob([codeEditor.value], { type: "text/plain" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = "main" + LANGS[lang].ext;
      document.body.appendChild(a);
      a.click();
      a.remove();
      setTimeout(() => URL.revokeObjectURL(url), 500);
    });

    clearHistoryBtn.addEventListener("click", () => { history = []; saveHistory(); renderHistory(); });

    stepper.addEventListener("click", (e) => {
      const s = e.target.closest(".step");
      if (s) activateView(s.dataset.view);
    });

    problemsToggle.addEventListener("click", () => setProblemsOpen(problems.dataset.open !== "true"));

    tokenFilter.addEventListener("input", renderTokens);
    tokChips.addEventListener("click", () => { tokenMode = "chips"; tokChips.classList.add("active"); tokTable.classList.remove("active"); renderTokens(); });
    tokTable.addEventListener("click", () => { tokenMode = "table"; tokTable.classList.add("active"); tokChips.classList.remove("active"); renderTokens(); });

    astExpand.addEventListener("click", () => panes.ast.querySelectorAll("details").forEach((d) => { d.open = true; }));
    astCollapse.addEventListener("click", () => panes.ast.querySelectorAll("details").forEach((d, i) => { d.open = i === 0; }));

    copyTacBtn.addEventListener("click", () => {
      if (!lastData || arr(lastData.tac).length === 0) return toast("No code to copy yet");
      copyText(arr(lastData.tac).map((t) => t.index + ": " + t.text).join("\n"), "TAC copied");
    });
    copyOutBtn.addEventListener("click", () => {
      if (!lastData || arr(lastData.output).length === 0) return toast("No output to copy yet");
      copyText(arr(lastData.output).join("\n"), "Output copied");
    });

    document.addEventListener("keydown", (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key === "Enter" && e.target !== codeEditor) {
        e.preventDefault();
        run();
      }
    });
  }

  /* =====================================================================
     Init
     ===================================================================== */

  function init() {
    wire();
    initSplitter();
    loadHistory();

    const collapsedPref = store.get("cf.side", null);
    setSidebar(collapsedPref === null ? window.innerWidth < 1100 : collapsedPref === "0");

    autoRunBox.checked = store.get("cf.auto", "0") === "1";

    const savedCode = store.get("cf.code", null);
    const savedLang = store.get("cf.lang", null);
    if (savedCode !== null && savedCode !== "") {
      codeEditor.value = savedCode;
      setLang(savedLang || "cpp", false);
      activePreset = Object.keys(PRESETS).find((k) => PRESETS[k].code === savedCode) || null;
    } else {
      codeEditor.value = PRESETS.cpp_demo.code;
      setLang(PRESETS.cpp_demo.lang, false);
      activePreset = "cpp_demo";
    }

    renderPresets();
    renderHistory();
    resetViews();
    resetStepper();
    activateView("output");
    renderEditor();
    updateCursor();
    setProblemsOpen(false);
  }

  init();
})();