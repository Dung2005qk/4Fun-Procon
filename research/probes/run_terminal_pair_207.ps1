param(
    [ValidateSet('development', 'holdout')]
    [string]$Split = 'development',
    [ValidateSet('parent', 'candidate')]
    [string]$Side = 'parent'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = Join-Path $repoRoot ("build-release\historical_tournament_{0}_207.exe" -f $Side)
$logPath = Join-Path $repoRoot "research\evidence\SCORE-TERMINAL-PAIR-EXCHANGE-207-$Split-$Side.log"

if ($Split -eq 'holdout') {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4910000; Count = 16; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4911000; Count = 16; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4912000; Count = 36; Spots = @(18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4913000; Count = 40; Spots = @(18, 24, 30); Agents = 8 }
    )
} else {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4900000; Count = 8; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4901000; Count = 8; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4902000; Count = 20; Spots = @(18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4903000; Count = 24; Spots = @(18, 24, 30); Agents = 8 }
    )
}

$fuels = @('low', 'default', 'high')
$roles = @('fixed', 'native')
Set-Content -LiteralPath $logPath -Value @(
    "experiment=SCORE-TERMINAL-PAIR-EXCHANGE-207",
    "split=$Split",
    "side=$Side",
    "manifest_sha256=D9506BFC73120CB30679271992EDAA71D333E94879AF1A78E47A3CF04B740D79"
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
        Add-Content -LiteralPath $logPath -Value (
            "case_begin,suite={0},seed={1},spots={2},fuel={3},role={4},window={5}" -f
            $tier.Suite, $seed, $spotCount, $fuel, $role, $window)
        $arguments = @(
            '--version', "206-$Split-$Side",
            '--track', 'protected-reserve',
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
        & $binary @arguments 2>&1 | ForEach-Object {
            Add-Content -LiteralPath $logPath -Value $_
        }
        if ($LASTEXITCODE -ne 0) {
            throw "case failed: suite=$($tier.Suite) seed=$seed side=$Side exit=$LASTEXITCODE"
        }
    }
}
Add-Content -LiteralPath $logPath -Value "run_complete,side=$Side,split=$Split"
