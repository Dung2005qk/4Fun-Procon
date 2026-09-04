param(
    [string]$Probe = "build-research-258-msvc/udonshield_claim_probe.exe",
    [string]$Replay = "artifacts/btc/m-6134-ab3d699-series.jsonl",
    [string]$Output = "research/evidence/ATTR-MULTITEAM-LONG-HORIZON-MEDIUM-SERVING-DEFICIT-269-closed-loop.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedProbe = "B6B05A3603DC4318B04A99F27B7936AEA3DDCF9917A8DE5001D6750214C7CE94"
$expectedReplay = "9A0AADF8A2DCA35C3C3E6B77FD337A3352085D9880A81BE72010DDE9B26D6BB5"

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

$outputParent = Split-Path -Parent $Output
if ($outputParent) {
    [System.IO.Directory]::CreateDirectory($outputParent) | Out-Null
}
if ((Test-Path -LiteralPath $Output) -and -not $Resume) {
    throw "Output already exists; use -Resume after checking atomic completion: $Output"
}

$completed = @{}
if (Test-Path -LiteralPath $Output) {
    foreach ($line in Get-Content -LiteralPath $Output) {
        if ($line -match '^case_complete day=(\d+) exit=0$') {
            $completed[[int]$Matches[1]] = $true
        }
    }
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($day in @(1, 4, 8)) {
    if ($completed.ContainsKey($day)) {
        continue
    }
    $partial = "$Output.day-$day.partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires manual inspection: $partial"
    }

    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Probe $Replay --recorded-sparse-closed-loop $day 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin day=$day started=$started")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
    }
    $payload.Add("case_complete day=$day exit=$exitCode")
    $payload.Add("case_elapsed_ms day=$day value=$($stopwatch.ElapsedMilliseconds)")
    [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)

    if ($exitCode -ne 0) {
        throw "Closed-loop attribution failed for day=$day with exit $exitCode; preserved $partial"
    }

    [System.IO.File]::AppendAllText($Output, ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine), $utf8NoBom)
    Move-Item -LiteralPath $partial -Destination "$partial.complete"
}

$finalCompleted = @(Select-String -LiteralPath $Output -Pattern '^case_complete day=\d+ exit=0$').Count
if ($finalCompleted -ne 3) {
    throw "Expected 3 complete cases, found $finalCompleted"
}
[System.IO.File]::AppendAllText($Output, "run_complete cases=3 days=1,4,8$([Environment]::NewLine)", $utf8NoBom)
