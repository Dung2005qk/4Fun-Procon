param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-REQUIRED-TERMINAL-LOWER-BOUND-301.csv',
    [string]$ParentBinary = 'build-research-301-parent-msvc/udonshield_historical_tournament.exe',
    [string]$CandidateBinary = 'build-research-301-candidate-msvc/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [string]$CandidateSha256 = '',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = 'B5BEED7027A7CE32CAA8CF878F2B573425F636E004974A765E26B08A9A3BF103'
$expectedParentSha256 = '03DD83C9B31BFFFF5D935D245693F7D260DFCBF5CB8607A3BAA2B1D5846E6314'
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
Assert-Sha256 $manifestPath $expectedManifestSha256
$allRows = @(Import-Csv -LiteralPath $manifestPath)
$seen = [Collections.Generic.HashSet[string]]::new()
$caseCounts = @{development = 0; holdout = 0}
foreach ($row in $allRows) {
    if ($row.experiment_id -ne 'SCORE-REQUIRED-TERMINAL-LOWER-BOUND-301' -or
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
        $row.scope -ne 'required-terminal-lower-bound') {
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
if ([string]::IsNullOrWhiteSpace($CandidateSha256)) {
    throw '-CandidateSha256 is required for an evidence run'
}
$parentPath = (Resolve-Path -LiteralPath $ParentBinary).Path
$candidatePath = (Resolve-Path -LiteralPath $CandidateBinary).Path
Assert-Sha256 $parentPath $expectedParentSha256
Assert-Sha256 $candidatePath $CandidateSha256

$rows = @($allRows | Where-Object { $_.split -eq $Phase })
$expectedCases = [int]$caseCounts[$Phase]
$expectedResults = 2 * $expectedCases
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-REQUIRED-TERMINAL-LOWER-BOUND-301-${Phase}.log"
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
        "run_begin,experiment=SCORE-REQUIRED-TERMINAL-LOWER-BOUND-301,phase=${Phase},expected_cases=${expectedCases},expected_results=${expectedResults}"
        "frozen,manifest_sha256=${expectedManifestSha256},parent_sha256=${expectedParentSha256},candidate_sha256=$($CandidateSha256.ToUpperInvariant())"
    ) | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultCounts = @{}
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^result,version=(parent|candidate)-301,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        $key = "$($Matches[2])|$($Matches[3])"
        $resultCounts[$key] = 1 + [int]($resultCounts[$key] ?? 0)
    } elseif ($line -match '^run_complete,') {
        $runComplete = $true
    }
}
foreach ($key in $resultCounts.Keys) {
    if (-not $completed.Contains($key) -or [int]$resultCounts[$key] -ne 2) {
        throw "ambiguous paired evidence: ${key} has $($resultCounts[$key]) results"
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
    "resume_begin,completed=$($completed.Count),expected=${expectedCases}" |
        Add-Content -LiteralPath $outputPath -Encoding utf8
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($completed.Contains($key)) { continue }
        $common = @(
            '--track', 'required-terminal-lower-bound-301',
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
        $arms = if (($seed -band 1) -eq 0) {
            @(@('parent', $parentPath), @('candidate', $candidatePath))
        } else {
            @(@('candidate', $candidatePath), @('parent', $parentPath))
        }
        $caseOutput = [Collections.Generic.List[string]]::new()
        foreach ($arm in $arms) {
            $arguments = @('--version', "$($arm[0])-301") + $common
            $armOutput = @(& $arm[1] @arguments 2>&1)
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0 -or
                @($armOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
                throw "$($row.suite)/${seed}/$($arm[0]) failed (${exitCode}): $($armOutput -join [Environment]::NewLine)"
            }
            foreach ($line in $armOutput) { $caseOutput.Add([string]$line) }
        }
        @(
            "case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),fuel=$($row.fuel_profile),public_window_ms=$($row.public_window_ms),order=$($arms[0][0])-$($arms[1][0])"
            $caseOutput
            "case_complete,suite=$($row.suite),seed=${seed}"
        ) | Add-Content -LiteralPath $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}

if ($completed.Count -ne $expectedCases) {
    throw "completed $($completed.Count), expected ${expectedCases}"
}
"run_complete,cases=$($completed.Count),results=${expectedResults}" |
    Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) paired cases in ${outputPath}"
