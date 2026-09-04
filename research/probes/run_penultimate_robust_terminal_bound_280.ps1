param(
    [string]$Probe = "build-research-268-msvc/udonshield_claim_probe.exe",
    [string]$ProbeSource = "research/probes/claim_probe.cpp",
    [string]$Manifest = "research/holdouts/ATTR-PENULTIMATE-ROBUST-TERMINAL-BOUND-280.csv",
    [string]$Output = "research/evidence/ATTR-PENULTIMATE-ROBUST-TERMINAL-BOUND-280.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedProbe = "CAE28CFABB1E942A20865308A5B1DF8E7ACCFC8AB45CD307693538A6B17D0BBE"
$expectedProbeSource = "126AABACD8AF60AE5627C6782D81AD13B60014B35CB1E40D946C3EEC1E0536F5"
$expectedManifest = "A7894FF1EEA050CD867AF0511702785A7F112CB9ADABAED43C98D91599E03F3E"

function Assert-Hash([string]$Path, [string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing frozen input: $Path"
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "Frozen hash mismatch for ${Path}: expected $Expected actual $actual"
    }
}

Assert-Hash $Probe $expectedProbe
Assert-Hash $ProbeSource $expectedProbeSource
Assert-Hash $Manifest $expectedManifest

$cases = @(Import-Csv -LiteralPath $Manifest)
if ($cases.Count -ne 2) {
    throw "Expected two frozen attribution cases, found $($cases.Count)"
}
foreach ($case in $cases) {
    Assert-Hash $case.replay $case.replay_sha256
}

if ((Test-Path -LiteralPath $Output) -and -not $Resume) {
    throw "Output already exists; use -Resume after checking atomic completion: $Output"
}
$completed = @{}
if (Test-Path -LiteralPath $Output) {
    foreach ($line in Get-Content -LiteralPath $Output) {
        if ($line -match '^case_complete case=(\S+) exit=0$') {
            $completed[$Matches[1]] = $true
        }
    }
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($case in $cases) {
    if ($completed.ContainsKey($case.case_id)) {
        continue
    }
    $partial = "$Output.$($case.case_id).partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires manual inspection: $partial"
    }
    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Probe $case.replay --recorded-penultimate-robust-bound $case.day $case.max_states $case.max_routes 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()
    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin case=$($case.case_id) replay=$($case.replay) day=$($case.day) started=$started max_states=$($case.max_states) max_routes=$($case.max_routes)")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
    }
    $payload.Add("case_complete case=$($case.case_id) exit=$exitCode")
    $payload.Add("case_elapsed_ms case=$($case.case_id) value=$($stopwatch.ElapsedMilliseconds)")
    [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)
    if ($exitCode -ne 0) {
        throw "Attribution failed for $($case.case_id) with exit $exitCode; preserved $partial"
    }
    [System.IO.File]::AppendAllText(
        $Output,
        ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine),
        $utf8NoBom)
    Move-Item -LiteralPath $partial -Destination "$partial.complete"
}

$finalCompleted = @(
    Select-String -LiteralPath $Output -Pattern '^case_complete case=\S+ exit=0$'
).Count
if ($finalCompleted -ne 2) {
    throw "Expected two complete cases, found $finalCompleted"
}
if (-not (Select-String -LiteralPath $Output -Pattern '^run_complete cases=2 ' -Quiet)) {
    [System.IO.File]::AppendAllText(
        $Output,
        "run_complete cases=2 max_states=50000 max_routes=64$([Environment]::NewLine)",
        $utf8NoBom)
}
