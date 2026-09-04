param(
    [string] $LogPath = 'research/evidence/ATTR-ROW24-DEFAULT-MEDIUM-ROLE-288-development.log'
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
    throw "Missing complete evidence: $LogPath"
}
if (-not (Select-String -LiteralPath $LogPath -Pattern '^run_complete cases=12$' -Quiet)) {
    throw 'Missing run_complete cases=12'
}

$caseCount = @(Select-String -LiteralPath $LogPath -Pattern '^case_complete order=(forward|reverse) mask=\d+ exit=0$').Count
if ($caseCount -ne 12) {
    throw "Expected 12 complete cases, found $caseCount"
}

$currentOrder = $null
$rows = [System.Collections.Generic.List[object]]::new()
foreach ($line in Get-Content -LiteralPath $LogPath) {
    if ($line -match '^case_begin order=(forward|reverse) mask=(\d+) ') {
        $currentOrder = $Matches[1]
        continue
    }
    if ($line -match '^summary role_mask=(\d+) days=(\d+) score=(\d+)/(\d+)/(\d+)$') {
        $rows.Add([pscustomobject]@{
            order = $currentOrder
            mask = [int] $Matches[1]
            days = [int] $Matches[2]
            lifetime = [int] $Matches[3]
            daily = [int] $Matches[4]
            servings = [int] $Matches[5]
        })
    }
}
if ($rows.Count -ne 12) {
    throw "Expected 12 summary rows, found $($rows.Count)"
}

function Compare-Official($Left, $Right) {
    foreach ($field in 'lifetime', 'daily', 'servings') {
        if ($Left.$field -gt $Right.$field) { return 1 }
        if ($Left.$field -lt $Right.$field) { return -1 }
    }
    return 0
}

$allPatrol = @{}
foreach ($order in 'forward', 'reverse') {
    $row = @($rows | Where-Object { $_.order -eq $order -and $_.mask -eq 0 })
    if ($row.Count -ne 1) { throw "Missing all-Patrol row for $order" }
    $allPatrol[$order] = $row[0]
}

$pairs = foreach ($mask in 0,1,2,4,8,16) {
    $forward = @($rows | Where-Object { $_.mask -eq $mask -and $_.order -eq 'forward' })
    $reverse = @($rows | Where-Object { $_.mask -eq $mask -and $_.order -eq 'reverse' })
    if ($forward.Count -ne 1 -or $reverse.Count -ne 1) {
        throw "Missing unique forward/reverse pair for mask $mask"
    }
    $forwardVsAllPatrol = if ($mask -eq 0) { 0 } else { Compare-Official $forward[0] $allPatrol.forward }
    $reverseVsAllPatrol = if ($mask -eq 0) { 0 } else { Compare-Official $reverse[0] $allPatrol.reverse }
    [pscustomobject]@{
        mask = $mask
        forward = "$($forward[0].lifetime)/$($forward[0].daily)/$($forward[0].servings)"
        reverse = "$($reverse[0].lifetime)/$($reverse[0].daily)/$($reverse[0].servings)"
        forward_vs_all_patrol = $forwardVsAllPatrol
        reverse_vs_all_patrol = $reverseVsAllPatrol
        direction_stable = ($forwardVsAllPatrol -eq $reverseVsAllPatrol)
        lower_servings = [Math]::Min($forward[0].servings, $reverse[0].servings)
    }
}

$oneTankerPairs = @($pairs | Where-Object { $_.mask -ne 0 })
$classWins = @($oneTankerPairs | Where-Object { $_.forward_vs_all_patrol -gt 0 -and $_.reverse_vs_all_patrol -gt 0 }).Count
$classLosses = @($oneTankerPairs | Where-Object { $_.forward_vs_all_patrol -lt 0 -and $_.reverse_vs_all_patrol -lt 0 }).Count
$directionReversals = @($oneTankerPairs | Where-Object { -not $_.direction_stable }).Count
$liveBeaters = @($oneTankerPairs | Where-Object { $_.lower_servings -gt 174 }).Count
$topBotBeaters = @($oneTankerPairs | Where-Object { $_.lower_servings -gt 182 }).Count

[pscustomobject]@{
    input_sha256 = (Get-FileHash -LiteralPath $LogPath -Algorithm SHA256).Hash
    cases = $caseCount
    live_score = '5/30/174'
    top_bot_score = '5/30/182'
    pairs = @($pairs)
    one_tanker_class_wins_vs_all_patrol = $classWins
    one_tanker_class_losses_vs_all_patrol = $classLosses
    direction_reversals = $directionReversals
    one_tanker_identities_beating_live_both_orders = $liveBeaters
    one_tanker_identities_beating_top_bot_both_orders = $topBotBeaters
} | ConvertTo-Json -Depth 6
