param(
    [ValidateSet('development', 'holdout')]
    [string]$Split = 'development',
    [ValidateSet('parent', 'candidate')]
    [string]$Side = 'parent'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = Join-Path $repoRoot ("build-release\historical_tournament_{0}_206.exe" -f $Side)
$logPath = Join-Path $repoRoot "research\evidence\SCORE-REFUEL-NOWAIT-206-$Split-$Side.log"

if ($Split -eq 'holdout') {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4890000; Count = 24; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4891000; Count = 24; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4892000; Count = 30; Spots = @(14, 18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4893000; Count = 30; Spots = @(18, 24, 30); Agents = 8 }
    )
} else {
    $tiers = @(
        @{ Suite = 'stratified-easy'; First = 4880000; Count = 12; Spots = @(12, 14); Agents = 4 },
        @{ Suite = 'stratified-medium'; First = 4881000; Count = 12; Spots = @(12, 18); Agents = 4 },
        @{ Suite = 'stratified-hard'; First = 4882000; Count = 18; Spots = @(14, 18, 24); Agents = 6 },
        @{ Suite = 'stratified-very-hard'; First = 4883000; Count = 18; Spots = @(18, 24, 30); Agents = 8 }
    )
}

$fuels = @('low', 'default', 'high')
$roles = @('fixed', 'native')
Set-Content -LiteralPath $logPath -Value @(
    "experiment=SCORE-REFUEL-NOWAIT-206",
    "split=$Split",
    "side=$Side",
    "manifest_sha256=22E3E21819E34B1ABFEA46258117D519ABA8984124AA031EA74B5C588C2E73D4"
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
