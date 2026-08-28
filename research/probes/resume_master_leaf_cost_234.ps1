param(
    [string]$Binary = 'build-release/udonshield_historical_tournament.exe',
    [string]$Manifest = 'research/holdouts/ATTR-MASTER-LEAF-COST-234.csv',
    [string]$Output = 'research/evidence/ATTR-MASTER-LEAF-COST-234-development.log'
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '240E75931C297A5A22AFC40B1201BCDE115A1A40CC7FF75568B93BB2C0F13A9B'
$expectedBinaryHash = '84647559F62C295A69A7FD156A7D6BD9A3E82DB19A59D160482641DFE3975B7D'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
$binaryHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $binaryPath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($binaryHash -ne $expectedBinaryHash) { throw "binary hash mismatch: $binaryHash" }
if (-not (Test-Path -LiteralPath $Output)) { throw "missing evidence log: $Output" }

$lines = @(Get-Content -LiteralPath $Output)
$resultSeeds = @{}
$completeSeeds = @{}
foreach ($line in $lines) {
    if ($line -match '^result,.*(?:^|,)seed=([0-9]+)(?:,|$)') {
        $seed = [int64]$Matches[1]
        if ($resultSeeds.ContainsKey($seed)) { throw "duplicate result seed $seed" }
        $resultSeeds[$seed] = $true
    }
    if ($line -match '^case_complete,.*(?:^|,)seed=([0-9]+)(?:,|$)') {
        $seed = [int64]$Matches[1]
        if ($completeSeeds.ContainsKey($seed)) { throw "duplicate case_complete seed $seed" }
        $completeSeeds[$seed] = $true
    }
}
if ($resultSeeds.Count -ne 15 -or $completeSeeds.Count -ne 15) {
    throw "expected exactly 15 atomic completed cases; results=$($resultSeeds.Count) completed=$($completeSeeds.Count)"
}
foreach ($seed in $resultSeeds.Keys) {
    if (-not $completeSeeds.ContainsKey($seed)) { throw "ambiguous result without case_complete: $seed" }
}
if ($lines[-1] -ne 'case_begin,suite=stratified-very-hard,seed=9423101,role=deadline,public_window_ms=15000') {
    throw "last line is not the registered unfinished case_begin"
}

$matchingRows = @(Import-Csv -LiteralPath $Manifest |
    Where-Object { [int64]$_.first_seed -eq 9423100 -and $_.role_mode -eq 'deadline' })
if ($matchingRows.Count -ne 1 -or [int]$matchingRows[0].count -ne 2) {
    throw 'registered final manifest row missing'
}
$row = $matchingRows[0]
$arguments = @(
    '--version', 'instrumented',
    '--track', 'master-leaf-cost',
    '--suite', $row.suite,
    '--first-seed', 9423101,
    '--seeds', 1,
    '--budget-ms', $row.budget_ms,
    '--role-ms', $row.role_ms,
    '--role-mode', $row.role_mode,
    '--role-mask', 1,
    '--fuel-profile', $row.fuel_profile,
    '--spot-count', $row.spot_count,
    '--short-role-fallback', 1,
    '--protected-wait-closed-loop',
    '--protected-wait-ms', 1600,
    '--terminal-sparse-ms', 5000,
    '--terminal-pair', 1,
    '--midday-chain', 1,
    '--midday-pair', 0,
    '--midday-target-followup', 1,
    '--public-window-probe-ms', $row.public_window_ms,
    '--checkpoint-closed-loop', 1,
    '--day-details'
)
& $binaryPath @arguments 2>&1 | Add-Content -LiteralPath $Output -Encoding utf8
if ($LASTEXITCODE -ne 0) { throw "final case exited $LASTEXITCODE" }
'case_complete,suite=stratified-very-hard,seed=9423101' |
    Add-Content -LiteralPath $Output -Encoding utf8
'run_complete,results=16' | Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed final case in $Output"
