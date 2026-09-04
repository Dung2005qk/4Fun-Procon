param(
    [string]$Binary = "build-release/udonshield_btc.exe",
    [string]$Replay = "artifacts/btc/m-6213-ab3d699-series.jsonl",
    [string]$Output = "research/evidence/ATTR-MULTITEAM-HIGH-FUEL-LONG-HORIZON-THROUGHPUT-270-role.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedBinary = "0653B59DB009E3797C850EFFA3865BC21DF377120404FCBB751BD227F462F607"
$expectedReplay = "FC3D31AC086D2F7ABE8FCBB00802648EAB9675C8C7F86891AC45A35C7AF0F5DA"

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

$orders = @(
    [pscustomobject]@{ Name = "forward"; Masks = @(0, 32) },
    [pscustomobject]@{ Name = "reverse"; Masks = @(32, 0) }
)
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)

foreach ($order in $orders) {
    foreach ($mask in $order.Masks) {
        $key = "$($order.Name):$mask"
        if ($completed.ContainsKey($key)) {
            continue
        }
        $partial = "$Output.$($order.Name)-$mask.partial"
        if (Test-Path -LiteralPath $partial) {
            throw "Ambiguous partial case requires manual inspection: $partial"
        }

        $started = [DateTimeOffset]::UtcNow.ToString("O")
        $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
        $lines = @(& $Binary replay-counterfactual --replay $Replay --role-mask $mask --response-ms 5000 --max-days 10 2>&1)
        $exitCode = $LASTEXITCODE
        $stopwatch.Stop()

        $payload = [System.Collections.Generic.List[string]]::new()
        $payload.Add("case_begin order=$($order.Name) mask=$mask started=$started")
        foreach ($line in $lines) {
            $payload.Add([string]$line)
        }
        $payload.Add("case_complete order=$($order.Name) mask=$mask exit=$exitCode")
        $payload.Add("case_elapsed_ms order=$($order.Name) mask=$mask value=$($stopwatch.ElapsedMilliseconds)")
        [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)

        if ($exitCode -ne 0) {
            throw "Counterfactual failed for $key with exit $exitCode; preserved $partial"
        }
        [System.IO.File]::AppendAllText($Output, ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine), $utf8NoBom)
        Move-Item -LiteralPath $partial -Destination "$partial.complete"
    }
}

$finalCompleted = @(Select-String -LiteralPath $Output -Pattern '^case_complete order=(forward|reverse) mask=\d+ exit=0$').Count
if ($finalCompleted -ne 4) {
    throw "Expected 4 complete cases, found $finalCompleted"
}
[System.IO.File]::AppendAllText($Output, "run_complete cases=4$([Environment]::NewLine)", $utf8NoBom)
