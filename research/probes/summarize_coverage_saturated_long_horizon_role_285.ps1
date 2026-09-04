param(
    [string]$LogPath = "research/evidence/ATTR-COVERAGE-SATURATED-LONG-HORIZON-ROLE-285-development.log"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
    throw "Missing complete evidence: $LogPath"
}
if (-not (Select-String -LiteralPath $LogPath -Pattern '^run_complete cases=14$' -Quiet)) {
    throw "Missing run_complete cases=14"
}

$caseCount = @(Select-String -LiteralPath $LogPath -Pattern '^case_complete order=(forward|reverse) mask=\d+ exit=0$').Count
if ($caseCount -ne 14) {
    throw "Expected 14 complete cases, found $caseCount"
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
            mask = [int]$Matches[1]
            days = [int]$Matches[2]
            lifetime_distinct = [int]$Matches[3]
            daily_distinct = [int]$Matches[4]
            servings = [int]$Matches[5]
        })
    }
}

if ($rows.Count -ne 14) {
    throw "Expected 14 summary rows, found $($rows.Count)"
}

$paired = foreach ($mask in 0,1,2,4,8,16,32) {
    $forward = $rows | Where-Object { $_.mask -eq $mask -and $_.order -eq 'forward' }
    $reverse = $rows | Where-Object { $_.mask -eq $mask -and $_.order -eq 'reverse' }
    if (@($forward).Count -ne 1 -or @($reverse).Count -ne 1) {
        throw "Missing unique forward/reverse pair for mask $mask"
    }
    [pscustomobject]@{
        mask = $mask
        forward = "$($forward.lifetime_distinct)/$($forward.daily_distinct)/$($forward.servings)"
        reverse = "$($reverse.lifetime_distinct)/$($reverse.daily_distinct)/$($reverse.servings)"
        stable = ($forward.lifetime_distinct -eq $reverse.lifetime_distinct -and
                  $forward.daily_distinct -eq $reverse.daily_distinct -and
                  $forward.servings -eq $reverse.servings)
    }
}

[pscustomobject]@{
    input_sha256 = (Get-FileHash -LiteralPath $LogPath -Algorithm SHA256).Hash
    cases = $caseCount
    pairs = @($paired)
} | ConvertTo-Json -Depth 5
