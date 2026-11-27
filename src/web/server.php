<?php
declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');

function sendJsonError(int $statusCode, string $message, array $extra = []): void {
    http_response_code($statusCode);
    echo json_encode(array_merge([
        'success'      => false,
        'file'         => '',
        'language'     => 'auto',
        'tokens'       => [],
        'ast'          => null,
        'symbols'      => [],
        'tac'          => [],
        'output'       => [],
        'runtimeError' => null,
        'errors'       => [
            [
                'phase'   => 'Server Error',
                'line'    => 0,
                'column'  => 0,
                'message' => $message,
            ]
        ],
    ], $extra), JSON_UNESCAPED_SLASHES);
    exit;
}

// Locate compiler binary in repository root (two levels up from src/web/)
$projectRoot = dirname(__DIR__, 2);
$candidates = [
    $projectRoot . DIRECTORY_SEPARATOR . 'codeforge_compiler.exe',
    $projectRoot . DIRECTORY_SEPARATOR . 'codeforge_compiler',
];

$compilerPath = null;
foreach ($candidates as $path) {
    if (is_file($path)) {
        $compilerPath = $path;
        break;
    }
}

// Optional health check: GET src/web/server.php?health=1
if ($_SERVER['REQUEST_METHOD'] === 'GET' && isset($_GET['health'])) {
    echo json_encode([
        'ok'           => $compilerPath !== null,
        'compilerPath' => $compilerPath !== null ? basename($compilerPath) : null,
        'phpVersion'   => PHP_VERSION,
    ]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    sendJsonError(405, 'Method Not Allowed. Send a POST request with JSON { "code": "...", "language": "c|cpp|java|auto" }.');
}

if ($compilerPath === null) {
    sendJsonError(500, 'Compiler binary not found. Run "make" in the project root first.');
}

// Read JSON request body (fallback to form POST)
$rawBody = file_get_contents('php://input');
$payload = null;
if ($rawBody !== false && trim($rawBody) !== '') {
    $decoded = json_decode($rawBody, true);
    if (is_array($decoded)) {
        $payload = $decoded;
    }
}
if ($payload === null) {
    $payload = $_POST;
}

$code = isset($payload['code']) && is_string($payload['code']) ? $payload['code'] : '';
$langRaw = isset($payload['language']) && is_string($payload['language'])
    ? strtolower(trim($payload['language']))
    : 'auto';

// Strict language allowlist -> prevents argument/path injection
$langMap = [
    'auto' => ['flag' => 'auto', 'ext' => '.txt'],
    'c'    => ['flag' => 'c',    'ext' => '.c'],
    'cpp'  => ['flag' => 'cpp',  'ext' => '.cpp'],
    'c++'  => ['flag' => 'cpp',  'ext' => '.cpp'],
    'java' => ['flag' => 'java', 'ext' => '.java'],
];

if (!isset($langMap[$langRaw])) {
    sendJsonError(400, 'Invalid language selection. Allowed values: auto, c, cpp, java.');
}

// Basic payload size limit (64 KB)
if (strlen($code) > 65536) {
    sendJsonError(413, 'Source code exceeds the 64 KB size limit.');
}

$langFlag = $langMap[$langRaw]['flag'];
$fileExt  = $langMap[$langRaw]['ext'];

$tempFile = sys_get_temp_dir() . DIRECTORY_SEPARATOR . 'codeforge_' . bin2hex(random_bytes(8)) . $fileExt;

if (file_put_contents($tempFile, $code) === false) {
    sendJsonError(500, 'Failed to write temporary source file for compilation.');
}

try {
    // Pass argv as an array so PHP invokes the binary directly without a shell
    $cmd = [
        $compilerPath,
        $tempFile,
        '--json',
        '--lang=' . $langFlag,
    ];

    $descriptors = [
        0 => ['pipe', 'r'],
        1 => ['pipe', 'w'],
        2 => ['pipe', 'w'],
    ];

    $process = proc_open($cmd, $descriptors, $pipes, $projectRoot);
    if (!is_resource($process)) {
        sendJsonError(500, 'Failed to launch codeforge_compiler process.');
    }

    fclose($pipes[0]);
    $stdout = stream_get_contents($pipes[1]);
    $stderr = stream_get_contents($pipes[2]);
    fclose($pipes[1]);
    fclose($pipes[2]);

    $exitCode = proc_close($process);

    if ($stdout === false || trim($stdout) === '') {
        $errMsg = trim((string)$stderr);
        sendJsonError(500, $errMsg !== '' ? $errMsg : 'Compiler produced no output (exit code ' . $exitCode . ').');
    }

    // Validate compiler JSON before returning it to the client
    $parsed = json_decode($stdout, true);
    if (!is_array($parsed)) {
        sendJsonError(500, 'Compiler returned invalid JSON.', [
            'rawOutput' => substr($stdout, 0, 500),
        ]);
    }

    // Replace temp file path with clean virtual filename for UI display
    $parsed['file'] = 'main' . $fileExt;

    echo json_encode($parsed, JSON_UNESCAPED_SLASHES);
} finally {
    if (is_file($tempFile)) {
        @unlink($tempFile);
    }
}