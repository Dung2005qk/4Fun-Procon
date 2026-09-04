param(
    [string]$Probe = "build-release/udonshield_claim_probe.exe",
    [string]$Replay = "artifacts/btc/m-8615-ab3d699-series.jsonl",
    [string]$Manifest = "research/holdouts/ATTR-ROW24-ROUTE-FRONTIER-289.csv",
    [string]$Output = "research/evidence/ATTR-ROW24-ROUTE-FRONTIER-289-development.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedProbe = "964AC9D999284E3751B2C902D265E97C6F751F7DB2F20E222868088FB28DC5B8"
$expectedReplay = "48C567D6922A732F5D8A055CAEB07B603AE66C04B4B5D909A3504EE29F48F784"
$expectedManifest = "C9F53460FBF188497C0747492B1A18DC09E23D1BD4634971C592743506EF91CD"

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
Assert-Hash $Replay $expectedReplay
Assert-Hash $Manifest $expectedManifest

$cases = @(Import-Csv -LiteralPath $Manifest)
if ($cases.Count -ne 24) {
    throw "Expected 24 frozen cases, found $($cases.Count)"
}
$outputParent = Split-Path -Parent $Output
if ($outputParent) {
    [System.IO.Directory]::CreateDirectory($outputParent) | Out-Null
}
if ((Test-Path -LiteralPath $Output) -and -not $Resume) {
    throw "Output already exists; use -Resume only after checking atomic completion: $Output"
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
    $caseId = [string]$case.case_id
    if ($completed.ContainsKey($caseId)) {
        continue
    }
    $partial = "$Output.$caseId.partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires inspection: $partial"
    }
    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Probe $Replay --recorded-sparse-budget ([int]$case.day) ([int]$case.agent) 50000 64 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin case=$caseId day=$($case.day) agent=$($case.agent) started=$started max_states=50000 max_routes=64")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
    }
    $payload.Add("case_complete case=$caseId exit=$exitCode")
    $payload.Add("case_elapsed_ms case=$caseId value=$($stopwatch.ElapsedMilliseconds)")
    [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)
    if ($exitCode -ne 0) {
        throw "Attribution failed for $caseId with exit $exitCode; preserved $partial"
    }
    [System.IO.File]::AppendAllText(
        $Output,
        ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine),
        $utf8NoBom)
    Move-Item -LiteralPath $partial -Destination "$partial.complete"
}

$finalCompleted = @(Select-String -LiteralPath $Output -Pattern '^case_complete case=\S+ exit=0$').Count
if ($finalCompleted -ne 24) {
    throw "Expected 24 complete cases, found $finalCompleted"
}
[System.IO.File]::AppendAllText(
    $Output,
    "run_complete cases=24 max_states=50000 max_routes=64$([Environment]::NewLine)",
    $utf8NoBom)
