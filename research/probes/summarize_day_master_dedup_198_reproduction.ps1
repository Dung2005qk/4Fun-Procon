param()

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$logPath = Join-Path $repoRoot 'research\evidence\PERF-DAY-MASTER-DEDUP-198-reproduction.log'
if (-not (Test-Path -LiteralPath $logPath)) {
    throw "missing log: $logPath"
}

$cases = @()
$current = $null
foreach ($line in Get-Content -LiteralPath $logPath) {
    if ($line -like 'case_begin,*') {
        if ($null -ne $current) {
            $cases += $current
        }
        $fields = @{}
        foreach ($part in $line.Split(',')) {
            if ($part.Contains('=')) {
                $pair = $part.Split('=', 2)
                $fields[$pair[0]] = $pair[1]
            }
        }
        $current = [pscustomobject]@{
            Label = $fields.label
            Suite = $fields.suite
            Seed = [int64]$fields.seed
            Spots = [int]$fields.spots
            Fuel = $fields.fuel
            Mask = [int]$fields.mask
            Window = $fields.window
            Results = [System.Collections.ArrayList]::new()
        }
        continue
    }
    if ($line -notlike 'result,*' -or $null -eq $current) {
        continue
    }
    $fields = @{}
    foreach ($part in $line.Split(',')) {
        if ($part.Contains('=')) {
            $pair = $part.Split('=', 2)
            $fields[$pair[0]] = $pair[1]
        }
    }
    $mode = if ($fields.version -like '*-candidate') { 'candidate' } else { 'parent' }
    [void]$current.Results.Add([pscustomobject]@{
        Mode = $mode
        Lifetime = [int]$fields.lifetime
        Daily = [int]$fields.daily
        Servings = [int]$fields.servings
        Invalid = [int]$fields.invalid
        Emergency = [int]$fields.emergency
        Combinations = [int64]$fields.combinations
    })
}
if ($null -ne $current) {
    $cases += $current
}

function Compare-Result($candidate, $parent) {
    if ($candidate.Lifetime -ne $parent.Lifetime) {
        return [pscustomobject]@{ Verdict = if ($candidate.Lifetime -gt $parent.Lifetime) { 'W' } else { 'L' }; Tier = 1; Delta = $candidate.Lifetime - $parent.Lifetime }
    }
    if ($candidate.Daily -ne $parent.Daily) {
        return [pscustomobject]@{ Verdict = if ($candidate.Daily -gt $parent.Daily) { 'W' } else { 'L' }; Tier = 2; Delta = $candidate.Daily - $parent.Daily }
    }
    if ($candidate.Servings -ne $parent.Servings) {
        return [pscustomobject]@{ Verdict = if ($candidate.Servings -gt $parent.Servings) { 'W' } else { 'L' }; Tier = 3; Delta = $candidate.Servings - $parent.Servings }
    }
    return [pscustomobject]@{ Verdict = 'T'; Tier = 0; Delta = 0 }
}

$complete = 0
$confirmedGain = 0
$confirmedLoss = 0
$mixed = 0
$bothTie = 0
foreach ($case in $cases) {
    if ($case.Results.Count -ne 4) {
        Write-Output "incomplete,seed=$($case.Seed),mask=$($case.Mask),results=$($case.Results.Count)"
        continue
    }
    ++$complete
    $ab = Compare-Result $case.Results[1] $case.Results[0]
    $ba = Compare-Result $case.Results[2] $case.Results[3]
    if ($ab.Verdict -eq 'W' -and $ba.Verdict -eq 'W') {
        ++$confirmedGain
        $stability = 'confirmed-gain'
    } elseif ($ab.Verdict -eq 'L' -and $ba.Verdict -eq 'L') {
        ++$confirmedLoss
        $stability = 'confirmed-loss'
    } elseif ($ab.Verdict -eq 'T' -and $ba.Verdict -eq 'T') {
        ++$bothTie
        $stability = 'tie'
    } else {
        ++$mixed
        $stability = 'mixed'
    }
    $parentScores = @($case.Results | Where-Object Mode -eq 'parent' | ForEach-Object { "$($_.Lifetime)/$($_.Daily)/$($_.Servings)" }) -join '|'
    $candidateScores = @($case.Results | Where-Object Mode -eq 'candidate' | ForEach-Object { "$($_.Lifetime)/$($_.Daily)/$($_.Servings)" }) -join '|'
    $invalid = ($case.Results | Measure-Object Invalid -Sum).Sum
    $emergency = ($case.Results | Measure-Object Emergency -Sum).Sum
    Write-Output (
        'repro_case,label={0},seed={1},mask={2},window={3},parent={4},candidate={5},ab={6}:{7}:{8},ba={9}:{10}:{11},stability={12},invalid={13},emergency={14}' -f
        $case.Label, $case.Seed, $case.Mask, $case.Window, $parentScores,
        $candidateScores, $ab.Verdict, $ab.Tier, $ab.Delta, $ba.Verdict,
        $ba.Tier, $ba.Delta, $stability, $invalid, $emergency)
}
Write-Output "repro_summary,complete=$complete,confirmed_gain=$confirmedGain,confirmed_loss=$confirmedLoss,mixed=$mixed,tie=$bothTie"
