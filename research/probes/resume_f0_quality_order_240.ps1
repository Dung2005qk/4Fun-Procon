param(
    [Parameter(Mandatory = $true)][string]$ParentBinary,
    [Parameter(Mandatory = $true)][string]$CandidateBinary,
    [ValidateSet('development', 'holdout')][string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-F0-QUALITY-ORDER-240.csv',
    [string]$Output = ''
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '1144BFC69B7C9582686D24FCB86800CE460281B0324B54604DBE555145BF415A'
$expectedParentHash = 'A5786150E4807FD2EE87CE2FFCEEB14CDE98E0A7D232E583FCBC04E81AE72E6E'
$expectedCandidateHash = '07A8F321546475263286CCF7C54CEEBAFDF5E6C6796FD85E5960FC23F908F3A2'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$parentPath = (Resolve-Path -LiteralPath $ParentBinary).Path
$candidatePath = (Resolve-Path -LiteralPath $CandidateBinary).Path
$parentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $parentPath).Hash
$candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidatePath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($parentHash -ne $expectedParentHash) { throw "parent hash mismatch: $parentHash" }
if ($candidateHash -ne $expectedCandidateHash) { throw "candidate hash mismatch: $candidateHash" }
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-F0-QUALITY-ORDER-240-$Phase.log"
}
if (-not (Test-Path -LiteralPath $Output)) { throw "evidence does not exist: $Output" }
$expectedHeader = @(
    'experiment=SCORE-F0-QUALITY-ORDER-240'
    "phase=$Phase"
    "manifest_sha256=$manifestHash"
    "parent_sha256=$parentHash"
    "candidate_sha256=$candidateHash"
)
if (((Get-Content -LiteralPath $Output -TotalCount 5) -join "`n") -ne ($expectedHeader -join "`n")) {
    throw 'evidence header mismatch'
}
$completeKeys = [Collections.Generic.HashSet[string]]::new()
$resultKeys = [Collections.Generic.HashSet[string]]::new()
foreach ($line in Get-Content -LiteralPath $Output) {
    if ($line -match '^case_complete,label=([^,]+),suite=([^,]+),seed=([0-9]+)$') {
        [void]$completeKeys.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
    } elseif ($line -match '^result,version=([^,]+),track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        [void]$resultKeys.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
    }
}
foreach ($key in $resultKeys) { if (-not $completeKeys.Contains($key)) { throw "ambiguous partial result: $key" } }
foreach ($key in $completeKeys) { if (-not $resultKeys.Contains($key)) { throw "completion without result: $key" } }
$rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.split -eq $Phase })
$expectedResults = 2 * (($rows | Measure-Object -Property count -Sum).Sum)
"resume_begin,completed=$($completeKeys.Count),expected=$expectedResults" |
    Add-Content -LiteralPath $Output -Encoding utf8
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'signature') } else { @('signature', 'parent') }
        foreach ($label in $labels) {
            $key = "$label|$($row.suite)|$seed"
            if ($completeKeys.Contains($key)) { continue }
            $binary = if ($label -eq 'parent') { $parentPath } else { $candidatePath }
            $arguments = @(
                '--version', $label, '--track', 'f0-quality-order',
                '--suite', $row.suite, '--first-seed', $seed, '--seeds', 1,
                '--budget-ms', $row.budget_ms, '--role-ms', $row.role_ms,
                '--role-mode', $row.role_mode, '--role-mask', 1,
                '--fuel-profile', $row.fuel_profile, '--spot-count', $row.spot_count,
                '--short-role-fallback', 1, '--protected-wait-closed-loop',
                '--protected-wait-ms', 1600, '--terminal-sparse-ms', 5000,
                '--terminal-pair', 1, '--midday-chain', 1, '--midday-pair', 0,
                '--midday-target-followup', 1,
                '--public-window-probe-ms', $row.public_window_ms,
                '--checkpoint-closed-loop', 1, '--day-details'
            )
            "case_resume,label=$label,suite=$($row.suite),seed=$seed,role=$($row.role_mode),public_window_ms=$($row.public_window_ms)" |
                Add-Content -LiteralPath $Output -Encoding utf8
            & $binary @arguments 2>&1 | Add-Content -LiteralPath $Output -Encoding utf8
            if ($LASTEXITCODE -ne 0) { throw "$label/$($row.suite)/$seed exited $LASTEXITCODE" }
            "case_complete,label=$label,suite=$($row.suite),seed=$seed" |
                Add-Content -LiteralPath $Output -Encoding utf8
            [void]$completeKeys.Add($key)
        }
    }
}
if ($completeKeys.Count -ne $expectedResults) { throw "completed $($completeKeys.Count), expected $expectedResults" }
"run_complete,results=$($completeKeys.Count),pairs=$($completeKeys.Count / 2)" |
    Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $($completeKeys.Count) results in $Output"
