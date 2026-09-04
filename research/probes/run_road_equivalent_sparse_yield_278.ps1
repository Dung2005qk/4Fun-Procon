param(
    [string]$Probe = "build-research-268-msvc/udonshield_claim_probe.exe",
    [string]$ProbeSource = "research/probes/claim_probe.cpp",
    [string]$Manifest = "research/holdouts/ATTR-ROAD-EQUIVALENT-SPARSE-YIELD-278.csv",
    [string]$Output = "research/evidence/ATTR-ROAD-EQUIVALENT-SPARSE-YIELD-278.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedProbe = "51A5881F99309AF134D5CE7F824E5D0DF57B6A25906134A110A99D93D2CA00B1"
$expectedProbeSource = "0CA860C2A5BB7588267A1A8F37FAE9200BA33CA00CE742BC783448B8D0B6FEA2"
$expectedManifest = "41A331B72D197C102A620BB57F8893DEF4A53A69A914D30260D2EFA8611C9683"

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
if ($cases.Count -ne 6) {
    throw "Expected six frozen attribution cases, found $($cases.Count)"
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
    $lines = @(& $Probe $case.replay --recorded-sparse-budget $case.day -1 $case.max_states $case.max_routes 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()
    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin case=$($case.case_id) replay=$($case.replay) day=$($case.day) stratum=$($case.stratum) started=$started max_states=$($case.max_states) max_routes=$($case.max_routes)")
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
if ($finalCompleted -ne 6) {
    throw "Expected six complete cases, found $finalCompleted"
}
if (-not (Select-String -LiteralPath $Output -Pattern '^run_complete cases=6 ' -Quiet)) {
    [System.IO.File]::AppendAllText(
        $Output,
        "run_complete cases=6 max_states=50000 max_routes=64$([Environment]::NewLine)",
        $utf8NoBom)
}
