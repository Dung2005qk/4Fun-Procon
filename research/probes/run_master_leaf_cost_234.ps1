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
if ($manifestHash -ne $expectedManifestHash) {
    throw "manifest hash mismatch: $manifestHash"
}
if ($binaryHash -ne $expectedBinaryHash) {
    throw "binary hash mismatch: $binaryHash"
}
if (Test-Path -LiteralPath $Output) {
    throw "refusing to overwrite existing evidence: $Output"
}

$rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.split -eq 'development' })
@(
    'experiment=ATTR-MASTER-LEAF-COST-234'
    "manifest_sha256=$manifestHash"
    "binary_sha256=$binaryHash"
) | Set-Content -LiteralPath $Output -Encoding utf8

$completed = 0
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $arguments = @(
            '--version', 'instrumented',
            '--track', 'master-leaf-cost',
            '--suite', $row.suite,
            '--first-seed', $seed,
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
        "case_begin,suite=$($row.suite),seed=$seed,role=$($row.role_mode),public_window_ms=$($row.public_window_ms)" |
            Add-Content -LiteralPath $Output -Encoding utf8
        & $binaryPath @arguments 2>&1 | Add-Content -LiteralPath $Output -Encoding utf8
        if ($LASTEXITCODE -ne 0) {
            throw "$($row.suite)/$seed exited $LASTEXITCODE"
        }
        "case_complete,suite=$($row.suite),seed=$seed" |
            Add-Content -LiteralPath $Output -Encoding utf8
        ++$completed
    }
}

"run_complete,results=$completed" | Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $completed results in $Output"
