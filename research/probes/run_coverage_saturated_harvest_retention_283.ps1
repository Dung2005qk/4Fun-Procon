param(
    [string]$Binary = "build-release/udonshield_btc.exe",
    [string]$Replay = "artifacts/btc/m-8135-ab3d699-series.jsonl",
    [string]$Manifest = "research/holdouts/ATTR-COVERAGE-SATURATED-HARVEST-RETENTION-283.csv",
    [string]$Output = "research/evidence/ATTR-COVERAGE-SATURATED-HARVEST-RETENTION-283-development.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"

$expectedBinary = "85816C2D02AE86CFC9E9F746C0EE37BF742204C5CD73A778BC6450B9250E2978"
$expectedReplay = "0EFC3363DBBCE1868A5195FE41677D358C250718D6FC91B11C602A8E33C5CBB3"
$expectedManifest = "4A89F354A42256193A24ACA1F6A412F1D3F3EFC2C85C1E6C5F07703F0D263C4E"

function Assert-Hash([string]$Path, [string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing frozen input: $Path"
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "Frozen hash mismatch for ${Path}: expected $Expected actual $actual"
    }
}

Assert-Hash $Binary $expectedBinary
Assert-Hash $Replay $expectedReplay
Assert-Hash $Manifest $expectedManifest

$cases = @(Import-Csv -LiteralPath $Manifest)
if ($cases.Count -ne 8) {
    throw "Expected 8 frozen cases, found $($cases.Count)"
}

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
        if ($line -match '^case_complete order=(forward|reverse) mode=(\d+) exit=0$') {
            $completed["$($Matches[1]):$($Matches[2])"] = $true
        }
    }
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($case in $cases) {
    $order = [string]$case.order
    $mode = [int]$case.mode
    $maxDays = [int]$case.max_days
    $key = "${order}:$mode"
    if ($completed.ContainsKey($key)) {
        continue
    }

    $partial = "$Output.$order-mode$mode.partial"
    $decisionDump = "$Output.$order-mode$mode.decisions.jsonl"
    if ((Test-Path -LiteralPath $partial) -or (Test-Path -LiteralPath $decisionDump)) {
        throw "Ambiguous partial case requires manual inspection: $key"
    }

    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Binary replay-counterfactual --replay $Replay --role-mask 0 --response-ms 5000 --harvest-extensions $mode --future-harvest-extensions $mode --max-days $maxDays --decision-dump $decisionDump 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin order=$order mode=$mode started=$started")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
    }
    $payload.Add("case_complete order=$order mode=$mode exit=$exitCode")
    $payload.Add("case_elapsed_ms order=$order mode=$mode value=$($stopwatch.ElapsedMilliseconds)")
    [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)

    if ($exitCode -ne 0) {
        throw "Counterfactual failed for $key with exit $exitCode; preserved $partial"
    }

    [System.IO.File]::AppendAllText(
        $Output,
        ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine),
        $utf8NoBom)
    Move-Item -LiteralPath $partial -Destination "$partial.complete"
}

$finalCompleted = @(
    Select-String -LiteralPath $Output -Pattern '^case_complete order=(forward|reverse) mode=\d+ exit=0$'
).Count
if ($finalCompleted -ne 8) {
    throw "Expected 8 complete cases, found $finalCompleted"
}
[System.IO.File]::AppendAllText(
    $Output,
    "run_complete cases=8$([Environment]::NewLine)",
    $utf8NoBom)
