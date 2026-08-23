param(
    [ValidateSet('development', 'holdout')]
    [string]$Split = 'development'
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$logPath = Join-Path $repoRoot "research\evidence\PERF-DAY-MASTER-DEDUP-198-$Split.log"
if (-not (Test-Path -LiteralPath $logPath)) {
    throw "missing log: $logPath"
}

$records = @()
foreach ($line in Get-Content -LiteralPath $logPath) {
    if ($line -notlike 'result,*') {
        continue
    }
    $fields = @{}
    foreach ($part in $line.Split(',')) {
        if (-not $part.Contains('=')) {
            continue
        }
        $pair = $part.Split('=', 2)
        $fields[$pair[0]] = $pair[1]
    }
    $mode = if ($fields.version -like '*-candidate') { 'candidate' } else { 'parent' }
    $records += [pscustomobject]@{
        Seed = [int64]$fields.seed
        Mode = $mode
        Suite = $fields.suite
        Family = $fields.family
        Fuel = $fields.fuel_profile
        RoleMode = $fields.role_mode
        SpotCount = [int]$fields.spot_count
        RoleMask = [int]$fields.role_mask
        Lifetime = [int]$fields.lifetime
        Daily = [int]$fields.daily
        Servings = [int]$fields.servings
        Invalid = [int]$fields.invalid
        Emergency = [int]$fields.emergency
        Combinations = [int64]$fields.combinations
        WaitDeadline = [int]$fields.protected_wait_deadline_days
        TerminalDeadline = [int]$fields.terminal_sparse_deadline
        TerminalFailure = [int]$fields.terminal_sparse_failure
    }
}

$pairs = @()
foreach ($group in $records | Group-Object Seed) {
    $parent = @($group.Group | Where-Object Mode -eq 'parent')
    $candidate = @($group.Group | Where-Object Mode -eq 'candidate')
    if ($parent.Count -ne 1 -or $candidate.Count -ne 1) {
        continue
    }
    $parent = $parent[0]
    $candidate = $candidate[0]
    $firstTier = 0
    $delta = 0
    if ($candidate.Lifetime -ne $parent.Lifetime) {
        $firstTier = 1
        $delta = $candidate.Lifetime - $parent.Lifetime
    } elseif ($candidate.Daily -ne $parent.Daily) {
        $firstTier = 2
        $delta = $candidate.Daily - $parent.Daily
    } elseif ($candidate.Servings -ne $parent.Servings) {
        $firstTier = 3
        $delta = $candidate.Servings - $parent.Servings
    }
    $verdict = if ($delta -gt 0) { 'W' } elseif ($delta -lt 0) { 'L' } else { 'T' }
    $pairs += [pscustomobject]@{
        Seed = $parent.Seed
        Suite = $parent.Suite
        Family = $parent.Family
        Fuel = $parent.Fuel
        RoleMode = $parent.RoleMode
        SpotCount = $parent.SpotCount
        Verdict = $verdict
        FirstTier = $firstTier
        Delta = $delta
        ServingDelta = $candidate.Servings - $parent.Servings
        CombinationDelta = $candidate.Combinations - $parent.Combinations
        RoleMismatch = [int]($candidate.RoleMask -ne $parent.RoleMask)
        Invalid = $parent.Invalid + $candidate.Invalid
        Emergency = $parent.Emergency + $candidate.Emergency
        WaitDeadline = $parent.WaitDeadline + $candidate.WaitDeadline
        TerminalDeadline = $parent.TerminalDeadline + $candidate.TerminalDeadline
        TerminalFailure = $parent.TerminalFailure + $candidate.TerminalFailure
    }
}

function Write-Summary([string]$label, [object[]]$items) {
    $wins = @($items | Where-Object Verdict -eq 'W').Count
    $ties = @($items | Where-Object Verdict -eq 'T').Count
    $losses = @($items | Where-Object Verdict -eq 'L').Count
    $gain = ($items | Where-Object ServingDelta -gt 0 | Measure-Object ServingDelta -Sum).Sum
    $loss = -($items | Where-Object ServingDelta -lt 0 | Measure-Object ServingDelta -Sum).Sum
    if ($null -eq $gain) { $gain = 0 }
    if ($null -eq $loss) { $loss = 0 }
    $minDelta = ($items | Measure-Object ServingDelta -Minimum).Minimum
    $maxDelta = ($items | Measure-Object ServingDelta -Maximum).Maximum
    $combinationDelta = ($items | Measure-Object CombinationDelta -Sum).Sum
    if ($null -eq $combinationDelta) { $combinationDelta = 0 }
    Write-Output (
        "summary,label={0},pairs={1},wtl={2}/{3}/{4},serving_gain={5},serving_loss={6},tail={7}/{8},combination_delta={9}" -f
        $label, $items.Count, $wins, $ties, $losses, $gain, $loss, $minDelta, $maxDelta, $combinationDelta)
}

Write-Summary 'all' $pairs
foreach ($field in @('Suite', 'Fuel', 'RoleMode', 'Family', 'SpotCount')) {
    foreach ($group in $pairs | Group-Object $field) {
        Write-Summary "$field=$($group.Name)" @($group.Group)
    }
}
Write-Output (
    "integrity,results={0},pairs={1},role_mismatch={2},invalid={3},emergency={4},wait_deadline={5},terminal_deadline={6},terminal_failure={7}" -f
    $records.Count,
    $pairs.Count,
    (($pairs | Measure-Object RoleMismatch -Sum).Sum),
    (($pairs | Measure-Object Invalid -Sum).Sum),
    (($pairs | Measure-Object Emergency -Sum).Sum),
    (($pairs | Measure-Object WaitDeadline -Sum).Sum),
    (($pairs | Measure-Object TerminalDeadline -Sum).Sum),
    (($pairs | Measure-Object TerminalFailure -Sum).Sum))
foreach ($loss in $pairs | Where-Object Verdict -eq 'L') {
    Write-Output (
        "loss,seed={0},suite={1},family={2},fuel={3},role={4},spots={5},tier={6},delta={7},serving_delta={8},combination_delta={9}" -f
        $loss.Seed, $loss.Suite, $loss.Family, $loss.Fuel, $loss.RoleMode,
        $loss.SpotCount, $loss.FirstTier, $loss.Delta, $loss.ServingDelta,
        $loss.CombinationDelta)
}
