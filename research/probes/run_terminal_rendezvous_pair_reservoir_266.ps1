param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-TERMINAL-RENDEZVOUS-PAIR-RESERVOIR-266.csv',
    [string]$Binary = 'build-research-266-msvc/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [string]$BinarySha256 = 'D2165C65507078A6C11F6A68366D96329569DA115FB63597E6542DFA9A1DB392',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = '6B00471EF05DD3611FA34A90B6C923067ECD64C266E5B6BA07C325E2DDF66A82'
$suiteSides = @{
    'multiteam-12' = 12
    'multiteam-16' = 16
    'multiteam-24' = 24
    'multiteam-32' = 32
}
function Resolve-ExistingPath([string]$PathValue) {
    return (Resolve-Path -LiteralPath $PathValue).Path
}

function Assert-Sha256([string]$PathValue, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $PathValue -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actual -ne $Expected.ToUpperInvariant()) {
        throw "SHA256 mismatch for ${PathValue}: expected ${Expected}, got ${actual}"
    }
}

$manifestPath = Resolve-ExistingPath $Manifest
$binaryPath = Resolve-ExistingPath $Binary
Assert-Sha256 $manifestPath $expectedManifestSha256
Assert-Sha256 $binaryPath $BinarySha256

$allRows = @(Import-Csv -LiteralPath $manifestPath)
$seen = [Collections.Generic.HashSet[string]]::new()
$caseCounts = @{development = 0; holdout = 0}
foreach ($row in $allRows) {
    if ($row.experiment_id -ne 'SCORE-TERMINAL-RENDEZVOUS-PAIR-RESERVOIR-266' -or
        $row.split -notin @('development', 'holdout')) {
        throw "invalid experiment/split: $($row.experiment_id)/$($row.split)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spots = [int]$row.spot_count
    $window = [int]$row.public_window_ms
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or
        [int]$row.role_ms -ne 5000 -or $players -notin @(8, 9, 10) -or
        $window -notin @(5000, 10000, 15000) -or
        $row.role_mode -notin @('fixed', 'deadline') -or
        $row.fuel_profile -notin @('low', 'default', 'high', 'generated')) {
        throw "invalid frozen stratum: $($row.suite)/$($row.first_seed)"
    }
    if ($row.suite -eq 'general') {
        if ($spots -ne 0) { throw 'general suite must derive spot count' }
    } elseif ($suiteSides.ContainsKey($row.suite)) {
        $side = [int]$suiteSides[$row.suite]
        if ($spots -le 0 -or $spots -gt ($side * $side - $players)) {
            throw "invalid spot count: $($row.suite)/${spots}"
        }
    } else {
        throw "unknown suite: $($row.suite)"
    }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.split)|$($row.suite)|${seed}"
        if (-not $seen.Add($key)) { throw "duplicate manifest case: ${key}" }
    }
    $caseCounts[$row.split] += $count
}
if ($allRows.Count -ne 24 -or $caseCounts.development -ne 30 -or
    $caseCounts.holdout -ne 54) {
    throw "expected 24 rows and 30/54 cases, got $($allRows.Count) and $($caseCounts.development)/$($caseCounts.holdout)"
}
if ($ValidateOnly) {
    Write-Output 'validated 24 rows, 30 development cases and 54 sealed holdout cases'
    exit 0
}

$rows = @($allRows | Where-Object { $_.split -eq $Phase })
$expectedResults = [int]$caseCounts[$Phase]
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-TERMINAL-RENDEZVOUS-PAIR-RESERVOIR-266-${Phase}.log"
}
$outputPath = if ([IO.Path]::IsPathRooted($Output)) {
    [IO.Path]::GetFullPath($Output)
} else {
    [IO.Path]::GetFullPath((Join-Path (Get-Location) $Output))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null
if ((Test-Path -LiteralPath $outputPath) -and -not $Resume) {
    throw "output already exists; use -Resume only after atomic-evidence validation: ${outputPath}"
}
if (-not (Test-Path -LiteralPath $outputPath)) {
    @(
        "run_begin,experiment=SCORE-TERMINAL-RENDEZVOUS-PAIR-RESERVOIR-266,phase=${Phase},expected_results=${expectedResults}"
        "frozen,manifest_sha256=${expectedManifestSha256},binary_sha256=$($BinarySha256.ToUpperInvariant())"
    ) | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultKeys = [Collections.Generic.HashSet[string]]::new()
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^result,version=causal-266,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        [void]$resultKeys.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^run_complete,') {
        $runComplete = $true
    }
}
foreach ($key in $resultKeys) {
    if (-not $completed.Contains($key)) { throw "ambiguous partial result: ${key}" }
}
foreach ($key in $completed) {
    if (-not $resultKeys.Contains($key)) { throw "completion without result: ${key}" }
}
if ($runComplete) {
    if ($completed.Count -ne $expectedResults) {
        throw "run_complete has $($completed.Count) results, expected ${expectedResults}"
    }
    Write-Output "already complete: $($completed.Count) results in ${outputPath}"
    exit 0
}
if ($Resume) {
    "resume_begin,completed=$($completed.Count),expected=${expectedResults}" |
        Add-Content -LiteralPath $outputPath -Encoding utf8
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($completed.Contains($key)) { continue }
        $arguments = @(
            '--version', 'causal-266',
            '--track', 'terminal-rendezvous-pair-reservoir-266',
            '--suite', $row.suite,
            '--first-seed', $seed,
            '--seeds', 1,
            '--budget-ms', $row.budget_ms,
            '--role-ms', $row.role_ms,
            '--role-mode', $row.role_mode,
            '--role-mask', 1,
            '--fuel-profile', $row.fuel_profile,
            '--spot-count', $row.spot_count,
            '--players', $row.players,
            '--short-role-fallback', 1,
            '--protected-wait-closed-loop',
            '--protected-wait-ms', 1600,
            '--terminal-sparse-ms', 5000,
            '--terminal-pair', 1,
            '--terminal-marginal-reservoir', 1,
            '--terminal-rendezvous-pair-reservoir', 1,
            '--midday-chain', 1,
            '--midday-pair', 0,
            '--midday-target-followup', 1,
            '--public-window-probe-ms', $row.public_window_ms,
            '--checkpoint-closed-loop', 1,
            '--day-details'
        )
        $caseOutput = @(& $binaryPath @arguments 2>&1)
        $exitCode = $LASTEXITCODE
        if ($exitCode -ne 0) {
            throw "$($row.suite)/${seed} exited ${exitCode}: $($caseOutput -join [Environment]::NewLine)"
        }
        if (@($caseOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
            throw "$($row.suite)/${seed} did not emit exactly one result"
        }
        @(
            "case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),public_window_ms=$($row.public_window_ms)"
            $caseOutput
            "case_complete,suite=$($row.suite),seed=${seed}"
        ) | Add-Content -LiteralPath $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}

if ($completed.Count -ne $expectedResults) {
    throw "completed $($completed.Count), expected ${expectedResults}"
}
"run_complete,results=$($completed.Count)" |
    Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) results in ${outputPath}"
