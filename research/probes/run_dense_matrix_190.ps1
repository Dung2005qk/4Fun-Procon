param(
    [ValidateSet('development', 'holdout')]
    [string]$Split = 'development',
    [int]$BudgetMs = 5000,
    [int]$RoleMs = 5000,
    [int]$TerminalSparseMs = 5000,
    [string]$OutputPath = 'research/evidence/SCORE-DENSE-SPARSE-FRONTIER-190-development.log'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$manifest = Join-Path $root 'research\holdouts\SCORE-DENSE-SPARSE-FRONTIER-190.csv'
$executable = Join-Path $root 'build-release\udonshield_historical_tournament.exe'
if (-not (Test-Path -LiteralPath $executable)) {
    throw "missing tournament executable: $executable"
}

$output = if ([System.IO.Path]::IsPathRooted($OutputPath)) {
    $OutputPath
} else {
    Join-Path $root $OutputPath
}
$outputDirectory = Split-Path -Parent $output
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
[System.IO.File]::WriteAllText($output, '')

$rows = Import-Csv -LiteralPath $manifest |
    Where-Object { $_.split -eq $Split }
if (-not $rows) {
    throw "manifest has no rows for split $Split"
}

foreach ($row in $rows) {
    $mapSide = [int]$row.map_side
    $spots = @(
        $row.spot_counts -split '\|' |
            ForEach-Object { [int]$_ } |
            Where-Object { $_ -le $mapSide }
    )
    if (-not $spots) {
        throw "manifest row has no protocol-valid spot count for side $mapSide"
    }
    $count = [int]$row.count
    $firstSeed = [uint64]$row.first_seed
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $spotIndex = $offset % $spots.Count
        $phase = [math]::Floor($offset / $spots.Count)
        $fuel = if ((($phase + $spotIndex) % 2) -eq 0) {
            'default'
        } else {
            'high'
        }
        $role = if ((($phase + [math]::Floor($spotIndex / 2)) % 2) -eq 0) {
            'fixed'
        } else {
            'native'
        }
        $arguments = @(
            '--version', 'dev190-terminal-sidecar',
            '--track', $Split,
            '--suite', "stratified-$($row.tier)",
            '--first-seed', ([string]($firstSeed + [uint64]$offset)),
            '--seeds', '1',
            '--budget-ms', ([string]$BudgetMs),
            '--role-ms', ([string]$RoleMs),
            '--role-mode', $role,
            '--role-mask', '4',
            '--spot-count', ([string]$spots[$spotIndex]),
            '--fuel-profile', $fuel,
            '--terminal-sparse-ms', ([string]$TerminalSparseMs),
            '--protected-wait-closed-loop',
            '--protected-wait-ms', '0'
        )
        $lines = & $executable @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "matrix fixture failed: tier=$($row.tier) offset=$offset"
        }
        foreach ($line in $lines) {
            [System.IO.File]::AppendAllText($output, $line + [Environment]::NewLine)
            Write-Output $line
        }
    }
}
