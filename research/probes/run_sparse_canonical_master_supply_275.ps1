param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-SPARSE-CANONICAL-MASTER-SUPPLY-275.csv',
    [string]$Binary = 'build-research-275-msvc/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [string]$BinarySha256 = '2406AF7D707B889689F27950B21A13FBFE3C7546376799417C9E96756A89DFE0',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = '51AC32FC032F10122FB4F0C98997CA5A892DEB845AC653AA192B0F568720A021'
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
Assert-Sha256 $manifestPath $expectedManifestSha256
$allRows = @(Import-Csv -LiteralPath $manifestPath)
$seen = [Collections.Generic.HashSet[string]]::new()
$caseCounts = @{development = 0; holdout = 0}
foreach ($row in $allRows) {
    if ($row.experiment_id -ne 'SCORE-SPARSE-CANONICAL-MASTER-SUPPLY-275' -or
        $row.split -notin @('development', 'holdout')) {
        throw "invalid experiment/split: $($row.experiment_id)/$($row.split)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spots = [int]$row.spot_count
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or
        [int]$row.role_ms -ne 5000 -or $players -notin @(8, 9, 10) -or
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
if ([string]::IsNullOrWhiteSpace($BinarySha256)) {
    throw '-BinarySha256 is required for an evidence run'
}
$binaryPath = Resolve-ExistingPath $Binary
Assert-Sha256 $binaryPath $BinarySha256

$rows = @($allRows | Where-Object { $_.split -eq $Phase })
$expectedCases = [int]$caseCounts[$Phase]
$expectedResults = 2 * $expectedCases
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-SPARSE-CANONICAL-MASTER-SUPPLY-275-${Phase}.log"
}
$outputPath = if ([IO.Path]::IsPathRooted($Output)) {
    [IO.Path]::GetFullPath($Output)
} else {
    [IO.Path]::GetFullPath((Join-Path (Get-Location) $Output))
}
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null
if ((Test-Path -LiteralPath $outputPath) -and -not $Resume) {
    throw "output already exists; use -Resume only after atomic validation: ${outputPath}"
}
if (-not (Test-Path -LiteralPath $outputPath)) {
    @(
        "run_begin,experiment=SCORE-SPARSE-CANONICAL-MASTER-SUPPLY-275,phase=${Phase},expected_cases=${expectedCases},expected_results=${expectedResults}"
        "frozen,manifest_sha256=${expectedManifestSha256},binary_sha256=$($BinarySha256.ToUpperInvariant())"
    ) | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultCounts = @{}
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^result,version=(parent|candidate)-275,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        $key = "$($Matches[2])|$($Matches[3])"
        if (-not $resultCounts.ContainsKey($key)) { $resultCounts[$key] = 0 }
        $resultCounts[$key] = [int]$resultCounts[$key] + 1
    } elseif ($line -match '^run_complete,') {
        $runComplete = $true
    }
}
foreach ($key in $resultCounts.Keys) {
    if (-not $completed.Contains($key) -or [int]$resultCounts[$key] -ne 2) {
        throw "ambiguous partial paired result: ${key} count=$($resultCounts[$key])"
    }
}
foreach ($key in $completed) {
    if (-not $resultCounts.ContainsKey($key) -or [int]$resultCounts[$key] -ne 2) {
        throw "completion without exactly two results: ${key}"
    }
}
if ($runComplete) {
    if ($completed.Count -ne $expectedCases) {
        throw "run_complete has $($completed.Count) cases, expected ${expectedCases}"
    }
    Write-Output "already complete: $($completed.Count) paired cases in ${outputPath}"
    exit 0
}
if ($Resume) {
    "resume_begin,completed_cases=$($completed.Count),expected_cases=${expectedCases}" |
        Add-Content -LiteralPath $outputPath -Encoding utf8
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($completed.Contains($key)) { continue }
        $common = @(
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
            '--day-details'
        )
        $sideOrder = if (($seed % 2) -eq 0) { @('off', 'on') } else { @('on', 'off') }
        $caseLines = @("case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),fuel=$($row.fuel_profile),spot_count=$($row.spot_count),side_order=$($sideOrder -join '-')")
        foreach ($side in $sideOrder) {
            $enabled = if ($side -eq 'on') { 1 } else { 0 }
            $version = if ($side -eq 'on') { 'candidate-275' } else { 'parent-275' }
            $track = if ($side -eq 'on') { 'sparse-canonical-master-supply-275-on' } else { 'sparse-canonical-master-supply-275-off' }
            $arguments = @(
                '--version', $version,
                '--track', $track,
                '--sparse-canonical-master-supply', $enabled
            ) + $common
            $sideOutput = @(& $binaryPath @arguments 2>&1)
            $sideExit = $LASTEXITCODE
            if ($sideExit -ne 0 -or
                @($sideOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
                throw "$($row.suite)/${seed}/${side} failed (${sideExit}): $($sideOutput -join [Environment]::NewLine)"
            }
            $caseLines += "side_begin,side=${side}"
            $caseLines += $sideOutput
            $caseLines += "side_complete,side=${side}"
        }
        $caseLines += "case_complete,suite=$($row.suite),seed=${seed}"
        $caseLines | Add-Content -LiteralPath $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}

if ($completed.Count -ne $expectedCases) {
    throw "completed $($completed.Count), expected ${expectedCases}"
}
"run_complete,cases=$($completed.Count),results=${expectedResults}" |
    Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) paired cases in ${outputPath}"
