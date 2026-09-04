param(
    [string]$Probe = "build-research-268-msvc/udonshield_claim_probe.exe",
    [string]$ProbeSource = "research/probes/claim_probe.cpp",
    [string]$Replay269 = "artifacts/btc/m-6134-ab3d699-series.jsonl",
    [string]$Replay270 = "artifacts/btc/m-6213-ab3d699-series.jsonl",
    [string]$Output = "research/evidence/ATTR-OFFROAD-SEGMENT-SUBSTITUTION-272.log",
    [switch]$Resume
)

$ErrorActionPreference = "Stop"
$expectedProbe = "C4AAD641DDB102C39FB002862F212CA91277300AD9159BA81A0EF975AC72093B"
$expectedProbeSource = "8158F5517102DACF5F45608B6659118218AAFCFF974A2F9C8D17DF304D3EE7EC"
$expectedReplay269 = "9A0AADF8A2DCA35C3C3E6B77FD337A3352085D9880A81BE72010DDE9B26D6BB5"
$expectedReplay270 = "FC3D31AC086D2F7ABE8FCBB00802648EAB9675C8C7F86891AC45A35C7AF0F5DA"

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
Assert-Hash $Replay269 $expectedReplay269
Assert-Hash $Replay270 $expectedReplay270

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
        if ($line -match '^case_complete replay=(\S+) day=(\d+) exit=0$') {
            $completed["$($Matches[1]):$($Matches[2])"] = $true
        }
    }
}

$cases = @(
    [pscustomobject]@{ Name = "m-6134"; Replay = $Replay269; Day = 1; Stratum = "early" },
    [pscustomobject]@{ Name = "m-6134"; Replay = $Replay269; Day = 4; Stratum = "middle" },
    [pscustomobject]@{ Name = "m-6134"; Replay = $Replay269; Day = 8; Stratum = "late" },
    [pscustomobject]@{ Name = "m-6213"; Replay = $Replay270; Day = 1; Stratum = "early" },
    [pscustomobject]@{ Name = "m-6213"; Replay = $Replay270; Day = 5; Stratum = "middle" },
    [pscustomobject]@{ Name = "m-6213"; Replay = $Replay270; Day = 8; Stratum = "late" }
)
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
foreach ($case in $cases) {
    $key = "$($case.Name):$($case.Day)"
    if ($completed.ContainsKey($key)) {
        continue
    }
    $partial = "$Output.$($case.Name)-day-$($case.Day).partial"
    if (Test-Path -LiteralPath $partial) {
        throw "Ambiguous partial case requires manual inspection: $partial"
    }

    $started = [DateTimeOffset]::UtcNow.ToString("O")
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    $lines = @(& $Probe $case.Replay --recorded-offroad-segments $case.Day 2>&1)
    $exitCode = $LASTEXITCODE
    $stopwatch.Stop()

    $payload = [System.Collections.Generic.List[string]]::new()
    $payload.Add("case_begin replay=$($case.Name) day=$($case.Day) stratum=$($case.Stratum) started=$started max_actions=8 max_paths=4 labels=64")
    foreach ($line in $lines) {
        $payload.Add([string]$line)
    }
    $payload.Add("case_complete replay=$($case.Name) day=$($case.Day) exit=$exitCode")
    $payload.Add("case_elapsed_ms replay=$($case.Name) day=$($case.Day) value=$($stopwatch.ElapsedMilliseconds)")
    [System.IO.File]::WriteAllLines($partial, $payload, $utf8NoBom)

    if ($exitCode -ne 0) {
        throw "Off-road segment attribution failed for $key with exit $exitCode; preserved $partial"
    }
    [System.IO.File]::AppendAllText(
        $Output,
        ([System.IO.File]::ReadAllText($partial) + [Environment]::NewLine),
        $utf8NoBom)
    Move-Item -LiteralPath $partial -Destination "$partial.complete"
}

$finalCompleted = @(
    Select-String -LiteralPath $Output -Pattern '^case_complete replay=\S+ day=\d+ exit=0$'
).Count
if ($finalCompleted -ne 6) {
    throw "Expected 6 complete cases, found $finalCompleted"
}
if (-not (Select-String -LiteralPath $Output -Pattern '^run_complete cases=6 ' -Quiet)) {
    [System.IO.File]::AppendAllText(
        $Output,
        "run_complete cases=6 max_actions=8 max_paths=4 labels=64$([Environment]::NewLine)",
        $utf8NoBom)
}
