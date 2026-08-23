param(
    [ValidateSet('development', 'holdout')]
    [string]$Split = 'development'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = Join-Path $repoRoot 'build-release\udonshield_historical_tournament.exe'
$logPath = Join-Path $repoRoot "research\evidence\PERF-DAY-MASTER-DEDUP-198-$Split.log"

if ($Split -eq 'holdout') {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4870000; Count = 24; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4871000; Count = 24; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4872000; Count = 30; Spots = @(14, 18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4873000; Count = 30; Spots = @(18, 24, 30); Agents = 8 }
    )
} else {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4860000; Count = 12; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4861000; Count = 12; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4862000; Count = 18; Spots = @(14, 18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4863000; Count = 18; Spots = @(18, 24, 30); Agents = 8 }
    )
}

$fuels = @('low', 'default', 'high')
$roles = @('fixed', 'native')
Set-Content -LiteralPath $logPath -Value @(
    'experiment=PERF-DAY-MASTER-DEDUP-198',
    "split=$Split",
    'parent=690728a',
    'manifest_sha256=BA23588A49AC6E431D401F63BB8538E919421844FB07484645F881780D87C34C'
)

foreach ($tier in $tiers) {
    for ($offset = 0; $offset -lt $tier.Count; ++$offset) {
        $seed = $tier.First + $offset
        $spotCount = $tier.Spots[$offset % $tier.Spots.Count]
        $fuel = $fuels[([math]::Floor($offset / $tier.Spots.Count)) % $fuels.Count]
        $role = $roles[([math]::Floor($offset / ($tier.Spots.Count * $fuels.Count))) % $roles.Count]
        if ($fuel -eq 'low') {
            $roleMask = (1 -shl 0) -bor (1 -shl ($tier.Agents - 1))
        } elseif ($fuel -eq 'high') {
            $roleMask = 2
        } else {
            $roleMask = 1
        }
        $longWindow = ($offset % 2) -eq 1
        $waitBudget = if ($longWindow) { 1600 } else { 500 }
        $terminalBudget = if ($longWindow) { 5000 } else { 3875 }
        $window = if ($longWindow) { 'long' } else { 'short' }
        $modes = if ($offset % 2 -eq 0) { @('parent', 'candidate') } else { @('candidate', 'parent') }
        Add-Content -LiteralPath $logPath -Value (
            "case_begin,suite={0},seed={1},spots={2},fuel={3},role={4},window={5},order={6}" -f
            $tier.Suite, $seed, $spotCount, $fuel, $role, $window, ($modes -join '|'))
        foreach ($mode in $modes) {
            $arguments = @(
                '--version', "198-$Split-$mode",
                '--track', 'day-master-dedup',
                '--suite', $tier.Suite,
                '--first-seed', $seed,
                '--seeds', 1,
                '--budget-ms', 3375,
                '--role-ms', 3375,
                '--role-mode', $role,
                '--role-mask', $roleMask,
                '--spot-count', $spotCount,
                '--fuel-profile', $fuel,
                '--protected-wait-closed-loop',
                '--protected-wait-ms', $waitBudget,
                '--terminal-sparse-ms', $terminalBudget
            )
            if ($mode -eq 'candidate') {
                $arguments += '--day-master-flat-key'
            }
            Add-Content -LiteralPath $logPath -Value "mode_begin,seed=$seed,mode=$mode"
            & $binary @arguments 2>&1 | ForEach-Object {
                Add-Content -LiteralPath $logPath -Value $_
                Write-Output $_
            }
            if ($LASTEXITCODE -ne 0) {
                throw "case failed: suite=$($tier.Suite) seed=$seed mode=$mode exit=$LASTEXITCODE"
            }
        }
    }
}
