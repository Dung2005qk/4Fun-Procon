param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-CURRENT-DAY-SPARSE-SIDECAR-250.csv',
    [string]$Binary = 'build-contract-fix/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [string]$BinarySha256 = '81A041C14A2EC73591C521BC2481E8C55608E022E825B76DD23FE3822B8A421C',
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = '7FFEAD5590F017B79C0DDC87DFB4F03EE2420CA2DBD8FA90C79C08A193054D58'

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

if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-CURRENT-DAY-SPARSE-SIDECAR-250-${Phase}.log"
}
$outputPath = if ([IO.Path]::IsPathRooted($Output)) {
    [IO.Path]::GetFullPath($Output)
} else {
    [IO.Path]::GetFullPath((Join-Path (Get-Location) $Output))
}
$outputDirectory = Split-Path -Parent $outputPath
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$allRows = @(Import-Csv -LiteralPath $manifestPath)
$rows = @($allRows | Where-Object { $_.split -eq $Phase })
if ($rows.Count -eq 0) { throw "manifest has no rows for phase ${Phase}" }
$expectedPairs = 0
foreach ($row in $rows) { $expectedPairs += [int]$row.count }
$expectedResults = 2 * $expectedPairs

if ((Test-Path -LiteralPath $outputPath) -and -not $Resume) {
    throw "output already exists; use -Resume only after validating atomic evidence: ${outputPath}"
}
if (-not (Test-Path -LiteralPath $outputPath)) {
    @(
        "run_begin,experiment=SCORE-CURRENT-DAY-SPARSE-SIDECAR-250,phase=${Phase},expected_pairs=${expectedPairs},expected_results=${expectedResults}"
        "frozen,manifest_sha256=${expectedManifestSha256},binary_sha256=$($BinarySha256.ToUpperInvariant())"
    ) | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultKeys = [Collections.Generic.HashSet[string]]::new()
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,label=([^,]+),suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
    } elseif ($line -match '^result,version=([^,]+),track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        [void]$resultKeys.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
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
        throw "run_complete exists with $($completed.Count) results, expected ${expectedResults}"
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
        $labels = if (($seed % 2) -eq 0) {
            @('parent', 'sidecar')
        } else {
            @('sidecar', 'parent')
        }
        foreach ($label in $labels) {
            $key = "${label}|$($row.suite)|${seed}"
            if ($completed.Contains($key)) { continue }
            $sidecarMilliseconds = if ($label -eq 'sidecar') { 5000 } else { 0 }
            $arguments = @(
                '--version', $label,
                '--track', 'current-day-sparse-sidecar-250',
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
                '--midday-chain', 1,
                '--midday-pair', 0,
                '--midday-target-followup', 1,
                '--public-window-probe-ms', 5000,
                '--checkpoint-closed-loop', 1,
                '--current-day-sparse-ms', $sidecarMilliseconds,
                '--day-details'
            )
            $caseOutput = @(& $binaryPath @arguments 2>&1)
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) {
                throw "${label}/$($row.suite)/${seed} exited ${exitCode}: $($caseOutput -join [Environment]::NewLine)"
            }
            if (@($caseOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
                throw "${label}/$($row.suite)/${seed} did not emit exactly one result"
            }
            @(
                "case_begin,label=${label},suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),public_window_ms=5000"
                $caseOutput
                "case_complete,label=${label},suite=$($row.suite),seed=${seed}"
            ) | Add-Content -LiteralPath $outputPath -Encoding utf8
            [void]$completed.Add($key)
        }
    }
}

if ($completed.Count -ne $expectedResults) {
    throw "completed $($completed.Count), expected ${expectedResults}"
}
"run_complete,results=$($completed.Count),pairs=$($completed.Count / 2)" |
    Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) results in ${outputPath}"
