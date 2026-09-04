param(
    [string]$Log = "research/evidence/ATTR-COVERAGE-SATURATED-HARVEST-RETENTION-283-development.log"
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) {
    throw "Missing completed log: $Log"
}
$lines = @(Get-Content -LiteralPath $Log)
$complete = @($lines | Where-Object { $_ -match '^case_complete order=(forward|reverse) mode=\d+ exit=0$' })
$runComplete = @($lines | Where-Object { $_ -eq 'run_complete cases=8' })
if ($complete.Count -ne 8 -or $runComplete.Count -ne 1) {
    throw "Incomplete evidence: cases=$($complete.Count) run_complete=$($runComplete.Count)"
}

$caseOrder = @()
foreach ($line in $lines) {
    if ($line -match '^case_begin order=(forward|reverse) mode=(\d+) ') {
        $caseOrder += [pscustomobject]@{ order=$Matches[1]; mode=[int]$Matches[2] }
    }
}
foreach ($case in $caseOrder) {
    $begin = [Array]::FindIndex($lines, [Predicate[string]]{ param($x) $x -match "^case_begin order=$($case.order) mode=$($case.mode) " })
    $end = [Array]::FindIndex($lines, $begin + 1, [Predicate[string]]{ param($x) $x -eq "case_complete order=$($case.order) mode=$($case.mode) exit=0" })
    if ($begin -lt 0 -or $end -le $begin) {
        throw "Missing atomic block for $($case.order):$($case.mode)"
    }
    $summary = @($lines[$begin..$end] | Where-Object { $_ -match '^summary role_mask=0 days=5 score=(\d+/\d+/\d+)$' })
    if ($summary.Count -ne 1) {
        throw "Missing unique summary for $($case.order):$($case.mode)"
    }
    $score = [regex]::Match($summary[0], 'score=(\d+/\d+/\d+)$').Groups[1].Value
    $dump = "$Log.$($case.order)-mode$($case.mode).decisions.jsonl"
    if (-not (Test-Path -LiteralPath $dump -PathType Leaf)) {
        throw "Missing decision dump: $dump"
    }
    $dayEvidence = @()
    foreach ($jsonLine in Get-Content -LiteralPath $dump) {
        $record = $jsonLine | ConvertFrom-Json
        $decision = $record.decision
        $dayEvidence += "d$($decision.dayNumber):max=$($decision.audit.portfolioMaximumServingsByAgent -join '/')|ext=$($decision.audit.portfolioHarvestExtensionsByAgent -join '/')|upper=$($decision.masterDiagnostics.optimisticUpperBound.lifetimeDistinct)/$($decision.masterDiagnostics.optimisticUpperBound.totalDailyDistinct)/$($decision.masterDiagnostics.optimisticUpperBound.totalServings)"
    }
    "order=$($case.order) mode=$($case.mode) score=$score days=$($dayEvidence -join ';')"
}
"log_sha256=$((Get-FileHash -LiteralPath $Log -Algorithm SHA256).Hash)"
