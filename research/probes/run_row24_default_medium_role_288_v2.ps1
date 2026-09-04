param(
    [string] $Binary = 'build-release/udonshield_btc.exe',
    [string] $Replay = 'artifacts/btc/m-8615-ab3d699-series.jsonl',
    [string] $Manifest = 'research/holdouts/ATTR-ROW24-DEFAULT-MEDIUM-ROLE-288.csv',
    [string] $Output = 'research/evidence/ATTR-ROW24-DEFAULT-MEDIUM-ROLE-288-v2-development.log',
    [switch] $Resume
)

$ErrorActionPreference = 'Stop'

$expectedBinary = '5FA10472D46E1136E3A2CFCD87FF26DA97C64575E42E2A385F1134CB44826F01'
$expectedReplay = '48C567D6922A732F5D8A055CAEB07B603AE66C04B4B5D909A3504EE29F48F784'
$expectedManifest = 'DFD9A70A701F7B831AFFEC05907D4B267E8EDAB146D4A031EDC5416BBA125473'

function Assert-FrozenHash([string] $Path, [string] $Expected) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing frozen input: $Path"
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "Frozen hash mismatch for ${Path}: expected $Expected actual $actual"
    }
}

Assert-FrozenHash $Binary $expectedBinary
Assert-FrozenHash $Replay $expectedReplay
Assert-FrozenHash $Manifest $expectedManifest

$cases = @(Import-Csv -LiteralPath $Manifest)
if ($cases.Count -ne 12) {
    throw "Expected 12 frozen cases, found $($cases.Count)"
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
        if ($line -match '^case_complete order=(forward|reverse) mask=(\d+) exit=0$') {
            $completed["$($Matches[1]):$($Matches[2])"] = $true
        }
    }
}

$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($case in $cases) {
    $order = [string] $case.order
    $mask = [int] $case.mask
    $maxDays = [int] $case.max_days
    $key = "${order}:$mask"
    if ($completed.ContainsKey($key)) {
        continue
    }

    $partial = "$Output.$order-$mask.partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires manual inspection: $partial"
    }

    $started = [DateTimeOffset]::UtcNow.ToString('O')
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Binary replay-counterfactual --replay $Replay --role-mask $mask --response-ms 5000 --current-floor 1 --max-days $maxDays 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin order=$order mask=$mask started=$started")
    foreach ($line in $lines) {
        $payload.Add([string] $line)
    }
    $payload.Add("case_complete order=$order mask=$mask exit=$exitCode")
    $payload.Add("case_elapsed_ms order=$order mask=$mask value=$($stopwatch.ElapsedMilliseconds)")
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
    Select-String -LiteralPath $Output -Pattern '^case_complete order=(forward|reverse) mask=\d+ exit=0$'
).Count
if ($finalCompleted -ne 12) {
    throw "Expected 12 complete cases, found $finalCompleted"
}
if (-not (Select-String -LiteralPath $Output -Pattern '^run_complete cases=12$' -Quiet)) {
    [System.IO.File]::AppendAllText(
        $Output,
        "run_complete cases=12$([Environment]::NewLine)",
        $utf8NoBom)
}
