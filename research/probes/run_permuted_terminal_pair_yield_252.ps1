param(
    [string]$Manifest = 'research/holdouts/ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252.csv',
    [string]$Binary = 'build-contract-fix/udonshield_historical_tournament.exe',
    [string]$Output = 'research/evidence/ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-development.log',
    [string]$BinarySha256 = 'A98E228D390EFA8D3359052B31EC482B1A4E2136DFCB33A9E3433BA81B589804',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$expectedManifestSha256 = '865C0A45DAE080E96EB2397634FF7714E3FABA70480AA033D89B955755CE19D9'

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

$rows = @(Import-Csv -LiteralPath $manifestPath)
$suiteSides = @{
    'multiteam-12' = 12
    'multiteam-16' = 16
    'multiteam-24' = 24
    'multiteam-32' = 32
}
$seen = @{}
$expectedResults = 0
foreach ($row in $rows) {
    if ($row.experiment_id -ne 'ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252') {
        throw "wrong experiment id: $($row.experiment_id)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spotCount = [int]$row.spot_count
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or
        [int]$row.role_ms -ne 5000 -or $players -notin @(8, 9, 10) -or
        $row.role_mode -notin @('fixed', 'deadline') -or
        $row.fuel_profile -notin @('low', 'default', 'high', 'generated')) {
        throw "invalid frozen public stratum: $($row.suite)/$($row.first_seed)"
    }
    if ($row.suite -eq 'general') {
        if ($spotCount -ne 0) { throw 'general suite must derive its spot count' }
    } elseif ($suiteSides.ContainsKey($row.suite)) {
        $side = [int]$suiteSides[$row.suite]
        if ($spotCount -le 0 -or $spotCount -gt ($side * $side - 8)) {
            throw "spot count cannot fit eligible cells: $($row.suite)/${spotCount}"
        }
    } else {
        throw "unknown frozen suite: $($row.suite)"
    }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($seen.ContainsKey($key)) { throw "duplicate manifest case: ${key}" }
        $seen[$key] = $true
    }
    $expectedResults += $count
}
if ($rows.Count -ne 12 -or $expectedResults -ne 24) {
    throw "expected 12 rows and 24 results, got $($rows.Count)/${expectedResults}"
}
if ($ValidateOnly) {
    Write-Output 'validated 12 rows and 24 fresh official-domain attribution cases'
    exit 0
}

$outputPath = if ([IO.Path]::IsPathRooted($Output)) {
    [IO.Path]::GetFullPath($Output)
} else {
    [IO.Path]::GetFullPath((Join-Path (Get-Location) $Output))
}
$outputDirectory = Split-Path -Parent $outputPath
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
$header = @(
    'experiment=ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252'
    "manifest_sha256=${expectedManifestSha256}"
    "binary_sha256=$($BinarySha256.ToUpperInvariant())"
)
if (Test-Path -LiteralPath $outputPath) {
    if (-not $Resume) { throw "refusing to overwrite evidence: ${outputPath}" }
    if (((Get-Content -LiteralPath $outputPath -TotalCount 3) -join "`n") -ne
        ($header -join "`n")) {
        throw 'evidence header mismatch'
    }
} else {
    $header | Set-Content -LiteralPath $outputPath -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultKeys = [Collections.Generic.HashSet[string]]::new()
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^result,version=probe,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
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
        throw "run_complete has $($completed.Count) cases, expected ${expectedResults}"
    }
    Write-Output "already complete: $($completed.Count) results"
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
            '--version', 'probe',
            '--track', 'permuted-terminal-pair-yield-252',
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
            '--permuted-terminal-probe-states', 50000,
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
            "case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players)"
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
