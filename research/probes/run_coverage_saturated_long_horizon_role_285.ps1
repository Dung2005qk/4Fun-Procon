param(
    [string]$Binary = "build-release/udonshield_btc.exe",
    [string]$Replay = "artifacts/btc/m-8385-ab3d699-series.jsonl",
    [string]$Manifest = "research/holdouts/ATTR-COVERAGE-SATURATED-LONG-HORIZON-ROLE-285.csv",
    [string]$Output = "research/evidence/ATTR-COVERAGE-SATURATED-LONG-HORIZON-ROLE-285-development.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"

$expectedBinary = "85816C2D02AE86CFC9E9F746C0EE37BF742204C5CD73A778BC6450B9250E2978"
$expectedReplay = "D55BF7F372CBF8A6215651E43854DFE329C5BDA64C755A5E819773BC61115734"
$expectedManifest = "D815FFE9F5EB0BEB939A74B0FACB45C6E8C060A25754879E7D37E360B0D59BF7"

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
if ($cases.Count -ne 14) {
    throw "Expected 14 frozen cases, found $($cases.Count)"
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
if ($finalCompleted -ne 14) {
    throw "Expected 14 complete cases, found $finalCompleted"
}
if (-not (Select-String -LiteralPath $Output -Pattern '^run_complete cases=14$' -Quiet)) {
    [System.IO.File]::AppendAllText(
        $Output,
        "run_complete cases=14$([Environment]::NewLine)",
        $utf8NoBom)
}
