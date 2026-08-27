param(
    [Parameter(Mandatory = $true)]
    [string]$Binary,
    [Parameter(Mandatory = $true)]
    [string]$Label,
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/PERF-ROLE-DIAGNOSTIC-ISOLATION-227.csv',
    [string]$OutputDirectory = 'research/evidence'
)

$ErrorActionPreference = 'Stop'
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
$rows = Import-Csv -LiteralPath $Manifest |
    Where-Object { $_.phase -eq $Phase }
if (@($rows).Count -eq 0) {
    throw "manifest has no rows for phase $Phase"
}

foreach ($row in $rows) {
    $outputPath = Join-Path $OutputDirectory (
        "PERF-ROLE-DIAGNOSTIC-ISOLATION-227-$Phase-$Label-$($row.suite).log")
    if (Test-Path -LiteralPath $outputPath) {
        throw "refusing to overwrite existing evidence: $outputPath"
    }
    $arguments = @(
        '--version', $Label,
        '--track', 'role-diagnostic-isolation',
        '--suite', $row.suite,
        '--first-seed', $row.first_seed,
        '--seeds', $row.seed_count,
        '--budget-ms', $row.budget_ms,
        '--role-ms', $row.role_ms,
        '--role-mode', $row.role_mode,
        '--fuel-profile', $row.fuel_profile,
        '--spot-count', $row.spot_count,
        '--short-role-fallback', '1'
    )
    & $binaryPath @arguments | Set-Content -LiteralPath $outputPath -Encoding utf8
    if ($LASTEXITCODE -ne 0) {
        throw "$Label/$($row.suite) exited $LASTEXITCODE"
    }
    $completed = (Select-String -LiteralPath $outputPath -Pattern '^result,').Count
    if ($completed -ne [int]$row.seed_count) {
        throw "$Label/$($row.suite) produced $completed/$($row.seed_count) results"
    }
    Write-Output "$Label/$($row.suite): $completed results"
}
