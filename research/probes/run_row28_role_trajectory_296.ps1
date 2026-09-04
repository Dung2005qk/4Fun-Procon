param(
    [string]$Binary = "build-release/udonshield_btc.exe",
    [string]$Replay = "artifacts/btc/m-9594-ab3d699-series.jsonl",
    [string]$Manifest = "research/holdouts/ATTR-ROW28-ROLE-TRAJECTORY-296.csv",
    [string]$Output = "research/evidence/ATTR-ROW28-ROLE-TRAJECTORY-296-development.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"

$expectedBinary = "B64C7EBDBB96AA02AA0EC52620C12C9F6937FEE546470549B8F5C777A8E3CC52"
$expectedReplay = "F5F0C8CDE7D1EF3BBE584D138E7E1BB6BD127388F139B6C869913A10EADDA933"
$expectedManifest = "3466045698B36B7493F3A6A6CEFB59DA4BB4D09ED7ABA4BF07DE8DE60525211A"

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
    $order = [string]$case.order
    $mask = [int]$case.mask
    $maxDays = [int]$case.max_days
    $key = "${order}:$mask"
    if ($completed.ContainsKey($key)) {
        continue
    }

    $partial = "$Output.$order-$mask.partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires manual inspection: $partial"
    }

    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Binary replay-counterfactual --replay $Replay --role-mask $mask --response-ms 5000 --max-days $maxDays 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin order=$order mask=$mask started=$started")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
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
[System.IO.File]::AppendAllText(
    $Output,
    "run_complete cases=12$([Environment]::NewLine)",
    $utf8NoBom)
