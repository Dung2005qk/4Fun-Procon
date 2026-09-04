param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/ATTR-OPTIMALITY-ENVELOPE-PREVALENCE-304.csv',
    [string]$Binary = 'build-research-304-msvc/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [switch]$ValidateOnly,
    [switch]$Resume,
    [switch]$OpenHoldout
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = 'D908B37A1787ACDDF01BB0DF9D34078EFE08EF18808CB4957A7D294CBA3B2DF4'
$expectedBinarySha256 = '33888811F62CB3BF5316A41328315D8077458344CB6C4F71AA2DC3815E134827'
$expectedHarnessSourceSha256 = '469F19D32C3111719BDC85EE10D69F2646C962438F9B0CDD7D837AA7D97FE3EF'
$suiteSides = @{
    'multiteam-12' = 12
    'multiteam-16' = 16
    'multiteam-24' = 24
    'multiteam-32' = 32
}

function Assert-Sha256([string]$PathValue, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $PathValue -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actual -ne $Expected.ToUpperInvariant()) {
        throw "SHA256 mismatch for ${PathValue}: expected ${Expected}, got ${actual}"
    }
}

$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
$harnessSourcePath = (Resolve-Path -LiteralPath 'old/harness/historical_tournament.cpp').Path
Assert-Sha256 $manifestPath $expectedManifestSha256
Assert-Sha256 $binaryPath $expectedBinarySha256
Assert-Sha256 $harnessSourcePath $expectedHarnessSourceSha256

$allRows = @(Import-Csv -LiteralPath $manifestPath)
$seen = [Collections.Generic.HashSet[string]]::new()
$caseCounts = @{development = 0; holdout = 0}
foreach ($row in $allRows) {
    if ($row.experiment_id -ne 'ATTR-OPTIMALITY-ENVELOPE-PREVALENCE-304' -or
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
        $row.fuel_profile -notin @('low', 'default', 'high', 'generated') -or
        $row.scope -ne 'optimality-envelope-prevalence') {
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
if ($allRows.Count -ne 24 -or $caseCounts.development -ne 12 -or
    $caseCounts.holdout -ne 24) {
    throw "expected 24 rows and 12/24 cases, got $($allRows.Count) and $($caseCounts.development)/$($caseCounts.holdout)"
}
if ($Phase -eq 'holdout' -and -not $OpenHoldout) {
    throw 'sealed holdout requires explicit -OpenHoldout after the development gate passes'
}
if ($ValidateOnly) {
    Write-Output 'validated frozen manifest, harness and binary: 12 development cases and 24 sealed holdout cases'
    exit 0
}

$rows = @($allRows | Where-Object { $_.split -eq $Phase })
$expectedCases = [int]$caseCounts[$Phase]
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/ATTR-OPTIMALITY-ENVELOPE-PREVALENCE-304-${Phase}.log"
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
        "run_begin,experiment=ATTR-OPTIMALITY-ENVELOPE-PREVALENCE-304,phase=${Phase},expected_cases=${expectedCases},expected_results=${expectedCases}"
        "frozen,manifest_sha256=${expectedManifestSha256},binary_sha256=${expectedBinarySha256},harness_source_sha256=${expectedHarnessSourceSha256}"
    ) | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultCounts = @{}
$detailCounts = @{}
$runComplete = $false
$currentKey = ''
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_begin,suite=([^,]+),seed=([0-9]+),') {
        $currentKey = "$($Matches[1])|$($Matches[2])"
    } elseif ($line -match '^result,version=canonical-304,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        $key = "$($Matches[1])|$($Matches[2])"
        $resultCounts[$key] = 1 + [int]($resultCounts[$key] ?? 0)
    } elseif ($line -match '^day_detail,' -and -not [string]::IsNullOrWhiteSpace($currentKey)) {
        $detailCounts[$currentKey] = 1 + [int]($detailCounts[$currentKey] ?? 0)
    } elseif ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
        $currentKey = ''
    } elseif ($line -match '^run_complete,') {
        $runComplete = $true
    }
}
foreach ($key in $resultCounts.Keys) {
    if (-not $completed.Contains($key) -or [int]$resultCounts[$key] -ne 1 -or
        -not $detailCounts.ContainsKey($key) -or [int]$detailCounts[$key] -le 0) {
        throw "ambiguous atomic evidence for ${key}"
    }
}
foreach ($key in $completed) {
    if (-not $resultCounts.ContainsKey($key) -or [int]$resultCounts[$key] -ne 1 -or
        -not $detailCounts.ContainsKey($key) -or [int]$detailCounts[$key] -le 0) {
        throw "completion without one result and day telemetry: ${key}"
    }
}
if ($runComplete) {
    if ($completed.Count -ne $expectedCases) {
        throw "run_complete has $($completed.Count) cases, expected ${expectedCases}"
    }
    Write-Output "already complete: $($completed.Count) cases in ${outputPath}"
    exit 0
}
if ($Resume) {
    "resume_begin,completed=$($completed.Count),expected=${expectedCases}" |
        Add-Content -LiteralPath $outputPath -Encoding utf8
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($completed.Contains($key)) { continue }
        $arguments = @(
            '--version', 'canonical-304',
            '--track', 'optimality-envelope-prevalence-304',
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
            '--midday-chain', 1,
            '--midday-pair', 0,
            '--midday-target-followup', 1,
            '--public-window-probe-ms', $row.public_window_ms,
            '--checkpoint-closed-loop', 1,
            '--day-details'
        )
        $caseOutput = @(& $binaryPath @arguments 2>&1)
        $exitCode = $LASTEXITCODE
        $resultCount = @($caseOutput | Where-Object { $_ -match '^result,' }).Count
        $detailCount = @($caseOutput | Where-Object { $_ -match '^day_detail,' }).Count
        if ($exitCode -ne 0 -or $resultCount -ne 1 -or $detailCount -le 0) {
            throw "$($row.suite)/${seed} failed (${exitCode}, result=${resultCount}, detail=${detailCount}): $($caseOutput -join [Environment]::NewLine)"
        }
        @(
            "case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),fuel=$($row.fuel_profile),public_window_ms=$($row.public_window_ms)"
            $caseOutput
            "case_complete,suite=$($row.suite),seed=${seed}"
        ) | Add-Content -LiteralPath $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}

if ($completed.Count -ne $expectedCases) {
    throw "completed $($completed.Count), expected ${expectedCases}"
}
"run_complete,cases=$($completed.Count),results=$($completed.Count)" |
    Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) cases in ${outputPath}"
