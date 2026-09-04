param(
    [string]$LogPath = "research/evidence/ATTR-ROW24-ROUTE-FRONTIER-289-development.log",
    [string]$SummaryPath = "research/evidence/ATTR-ROW24-ROUTE-FRONTIER-289-summary.json"
)

$ErrorActionPreference = "Stop"
$lines = @(Get-Content -LiteralPath $LogPath)
if (@($lines | Where-Object { $_ -match '^run_complete cases=24 ' }).Count -ne 1) {
    throw "Missing unique run_complete marker"
}

function Parse-Score([string]$Text) {
    if ($Text -notmatch '^(\d+)/(\d+)/(\d+)$') {
        throw "Invalid score: $Text"
    }
    return @([int]$Matches[1], [int]$Matches[2], [int]$Matches[3])
}

function Score-Delta($Best, $Base) {
    return @(
        ($Best[0] - $Base[0]),
        ($Best[1] - $Base[1]),
        ($Best[2] - $Base[2]))
}

$roots = [System.Collections.Generic.List[object]]::new()
$current = $null
foreach ($line in $lines) {
    if ($line -match '^case_begin case=(\S+) day=(\d+) agent=(\d+) ') {
        if ($null -ne $current) { throw "Nested case_begin" }
        $current = [ordered]@{ case = $Matches[1]; day = [int]$Matches[2]; agent = [int]$Matches[3] }
        continue
    }
    if ($null -eq $current) { continue }
    if ($line -match '^sparse_global_best=(\d+/\d+/\d+) baseline=(\d+/\d+/\d+) agent=(-?\d+) mask=(\S+)$') {
        $current.baseline = Parse-Score $Matches[2]
        $current.unrestricted = Parse-Score $Matches[1]
        $current.unrestricted_delta = Score-Delta $current.unrestricted $current.baseline
        $current.unrestricted_agent = [int]$Matches[3]
        $current.unrestricted_mask = $Matches[4]
        continue
    }
    if ($line -match '^sparse_protected_best=(\d+/\d+/\d+) baseline=(\d+/\d+/\d+) agent=(-?\d+) mask=(\S+)$') {
        $current.protected = Parse-Score $Matches[1]
        $current.protected_delta = Score-Delta $current.protected (Parse-Score $Matches[2])
        $current.protected_agent = [int]$Matches[3]
        $current.protected_mask = $Matches[4]
        continue
    }
    if ($line -match '^road_equivalent_summary exact=(\d+) strict=(\d+) strict_changed_terminal=(\d+) baseline=(\d+/\d+/\d+) best=(\d+/\d+/\d+) agent=(-?\d+) mask=(\S+) terminal_distance=(-?\d+) fuel_delta=(-?\d+)$') {
        $current.road_equal_candidates = [int64]$Matches[1]
        $current.road_equal_strict = [int64]$Matches[2]
        $current.road_equal_changed_terminal = [int64]$Matches[3]
        $current.road_equal_best = Parse-Score $Matches[5]
        $current.road_equal_delta = Score-Delta $current.road_equal_best (Parse-Score $Matches[4])
        $current.road_equal_terminal_distance = [int]$Matches[8]
        $current.road_equal_fuel_delta = [int]$Matches[9]
        continue
    }
    if ($line -match '^traffic_nonincreasing_summary candidates=(\d+) strict=(\d+) strict_changed_terminal=(\d+) baseline=(\d+/\d+/\d+) best=(\d+/\d+/\d+) agent=(-?\d+) mask=(\S+) terminal_distance=(-?\d+) fuel_delta=(-?\d+) road_reduction=(-?\d+)$') {
        $current.traffic_nonincreasing_candidates = [int64]$Matches[1]
        $current.traffic_nonincreasing_strict = [int64]$Matches[2]
        $current.traffic_nonincreasing_changed_terminal = [int64]$Matches[3]
        $current.traffic_nonincreasing_best = Parse-Score $Matches[5]
        $current.traffic_nonincreasing_delta = Score-Delta $current.traffic_nonincreasing_best (Parse-Score $Matches[4])
        $current.traffic_nonincreasing_terminal_distance = [int]$Matches[8]
        $current.traffic_nonincreasing_fuel_delta = [int]$Matches[9]
        $current.traffic_nonincreasing_road_reduction = [int64]$Matches[10]
        continue
    }
    if ($line -match '^case_complete case=(\S+) exit=0$') {
        if ($Matches[1] -ne $current.case) { throw "Case completion mismatch" }
        foreach ($required in @('baseline','unrestricted','protected','road_equal_best','traffic_nonincreasing_best')) {
            if (-not $current.Contains($required)) { throw "Missing $required for $($current.case)" }
        }
        $roots.Add([pscustomobject]$current)
        $current = $null
    }
}
if ($null -ne $current) { throw "Unclosed final case" }
if ($roots.Count -ne 24 -or @($roots.case | Sort-Object -Unique).Count -ne 24) {
    throw "Expected 24 unique completed roots, found $($roots.Count)"
}

$strict = { param($delta) ($delta[0] -gt 0) -or ($delta[0] -eq 0 -and $delta[1] -gt 0) -or ($delta[0] -eq 0 -and $delta[1] -eq 0 -and $delta[2] -gt 0) }
$byDay = [System.Collections.Generic.List[object]]::new()
foreach ($day in 1..6) {
    $dayRoots = @($roots | Where-Object { $_.day -eq $day })
    $byDay.Add([pscustomobject][ordered]@{
        day = $day
        unrestricted_yielding = @($dayRoots | Where-Object { & $strict $_.unrestricted_delta }).Count
        protected_yielding = @($dayRoots | Where-Object { & $strict $_.protected_delta }).Count
        road_equal_yielding = @($dayRoots | Where-Object { & $strict $_.road_equal_delta }).Count
        traffic_nonincreasing_yielding = @($dayRoots | Where-Object { & $strict $_.traffic_nonincreasing_delta }).Count
        max_unrestricted_serving_delta = ($dayRoots | ForEach-Object { $_.unrestricted_delta[2] } | Measure-Object -Maximum).Maximum
        max_protected_serving_delta = ($dayRoots | ForEach-Object { $_.protected_delta[2] } | Measure-Object -Maximum).Maximum
    })
}

$summary = [ordered]@{
    experiment = 'ATTR-ROW24-ROUTE-FRONTIER-289'
    cases = $roots.Count
    unrestricted_yielding = @($roots | Where-Object { & $strict $_.unrestricted_delta }).Count
    protected_yielding = @($roots | Where-Object { & $strict $_.protected_delta }).Count
    road_equal_yielding = @($roots | Where-Object { & $strict $_.road_equal_delta }).Count
    traffic_nonincreasing_yielding = @($roots | Where-Object { & $strict $_.traffic_nonincreasing_delta }).Count
    road_equal_strict_candidates = ($roots | Measure-Object -Property road_equal_strict -Sum).Sum
    traffic_nonincreasing_strict_candidates = ($roots | Measure-Object -Property traffic_nonincreasing_strict -Sum).Sum
    by_day = $byDay
    roots = $roots
}
$json = $summary | ConvertTo-Json -Depth 8
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($SummaryPath, ($json + [Environment]::NewLine), $utf8NoBom)
$json
