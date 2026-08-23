param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$binary = Join-Path $repoRoot 'build-release\udonshield_historical_tournament.exe'
$logPath = Join-Path $repoRoot 'research\evidence\PERF-DAY-MASTER-DEDUP-198-reproduction.log'

# Every run below fixes the observed role assignment so this gate measures only
# the post-role day-master key. Both role masks are retained for the one native
# development pair whose independently timed role searches disagreed.
$cases = @(
    @{ Label = 'loss'; Suite = 'stratified-medium'; Seed = 4861003; Spots = 18; Fuel = 'default'; Mask = 1; Window = 'long' },
    @{ Label = 'loss-role-parent'; Suite = 'stratified-medium'; Seed = 4861009; Spots = 18; Fuel = 'default'; Mask = 8; Window = 'long' },
    @{ Label = 'loss-role-candidate'; Suite = 'stratified-medium'; Seed = 4861009; Spots = 18; Fuel = 'default'; Mask = 2; Window = 'long' },
    @{ Label = 'loss'; Suite = 'stratified-hard'; Seed = 4862000; Spots = 14; Fuel = 'low'; Mask = 33; Window = 'short' },
    @{ Label = 'loss'; Suite = 'stratified-very-hard'; Seed = 4863000; Spots = 18; Fuel = 'low'; Mask = 129; Window = 'short' },
    @{ Label = 'loss'; Suite = 'stratified-very-hard'; Seed = 4863002; Spots = 30; Fuel = 'low'; Mask = 129; Window = 'short' },
    @{ Label = 'loss'; Suite = 'stratified-very-hard'; Seed = 4863004; Spots = 24; Fuel = 'default'; Mask = 1; Window = 'short' },
    @{ Label = 'loss-role-parent'; Suite = 'stratified-very-hard'; Seed = 4863009; Spots = 18; Fuel = 'low'; Mask = 8; Window = 'long' },
    @{ Label = 'loss-role-candidate'; Suite = 'stratified-very-hard'; Seed = 4863009; Spots = 18; Fuel = 'low'; Mask = 2; Window = 'long' },
    @{ Label = 'loss'; Suite = 'stratified-very-hard'; Seed = 4863010; Spots = 24; Fuel = 'low'; Mask = 1; Window = 'short' },
    @{ Label = 'gain'; Suite = 'stratified-hard'; Seed = 4862002; Spots = 24; Fuel = 'low'; Mask = 33; Window = 'short' },
    @{ Label = 'gain'; Suite = 'stratified-hard'; Seed = 4862011; Spots = 24; Fuel = 'low'; Mask = 1; Window = 'long' },
    @{ Label = 'gain'; Suite = 'stratified-very-hard'; Seed = 4863003; Spots = 18; Fuel = 'default'; Mask = 1; Window = 'long' },
    @{ Label = 'gain'; Suite = 'stratified-very-hard'; Seed = 4863011; Spots = 30; Fuel = 'low'; Mask = 1; Window = 'long' },
    @{ Label = 'gain'; Suite = 'stratified-very-hard'; Seed = 4863012; Spots = 18; Fuel = 'default'; Mask = 32; Window = 'short' }
)

Set-Content -LiteralPath $logPath -Value @(
    'experiment=PERF-DAY-MASTER-DEDUP-198',
    'gate=fixed-role-abba-reproduction',
    'parent=690728a',
    'development_sha256=54642CB2E387B03067D766FA361943A4F4F7F1A30FCF7EFDA6B7C60897B779A0',
    'manifest_sha256=BA23588A49AC6E431D401F63BB8538E919421844FB07484645F881780D87C34C'
)

foreach ($case in $cases) {
    $waitBudget = if ($case.Window -eq 'long') { 1600 } else { 500 }
    $terminalBudget = if ($case.Window -eq 'long') { 5000 } else { 3875 }
    Add-Content -LiteralPath $logPath -Value (
        'case_begin,label={0},suite={1},seed={2},spots={3},fuel={4},mask={5},window={6}' -f
        $case.Label, $case.Suite, $case.Seed, $case.Spots, $case.Fuel,
        $case.Mask, $case.Window)
    foreach ($mode in @('parent', 'candidate', 'candidate', 'parent')) {
        $arguments = @(
            '--version', "198-reproduction-$mode",
            '--track', 'day-master-dedup',
            '--suite', $case.Suite,
            '--first-seed', $case.Seed,
            '--seeds', 1,
            '--budget-ms', 3375,
            '--role-ms', 3375,
            '--role-mode', 'fixed',
            '--role-mask', $case.Mask,
            '--spot-count', $case.Spots,
            '--fuel-profile', $case.Fuel,
            '--protected-wait-closed-loop',
            '--protected-wait-ms', $waitBudget,
            '--terminal-sparse-ms', $terminalBudget
        )
        if ($mode -eq 'candidate') {
            $arguments += '--day-master-flat-key'
        }
        Add-Content -LiteralPath $logPath -Value "mode_begin,seed=$($case.Seed),mask=$($case.Mask),mode=$mode"
        & $binary @arguments 2>&1 | ForEach-Object {
            Add-Content -LiteralPath $logPath -Value $_
            Write-Output $_
        }
        if ($LASTEXITCODE -ne 0) {
            throw "case failed: seed=$($case.Seed) mask=$($case.Mask) mode=$mode exit=$LASTEXITCODE"
        }
    }
}
