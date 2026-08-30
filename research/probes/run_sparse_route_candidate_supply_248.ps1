param(
    [Parameter(Mandatory = $true)][string]$ParentBinary,
    [Parameter(Mandatory = $true)][string]$CandidateBinary,
    [ValidateSet('development', 'holdout')][string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-SPARSE-ROUTE-CANDIDATE-SUPPLY-248.csv',
    [string]$Output = '',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '329A7FA1FDF7CE94C3F6DFCBEC9DE50A582D6B9710615A2BD0F856911A9C6E2E'
$expectedParentHash = '55F09C751AEA2557FFB7F2BD0B66DD1EC918E778C4D223A8ADC7933F0D6A76F8'
$expectedCandidateHash = 'F6ECBD84A76DB366C5512F4EC3A26E30B8A9E28E03FCD4E9696C008F6D3845E3'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$parentPath = (Resolve-Path -LiteralPath $ParentBinary).Path
$candidatePath = (Resolve-Path -LiteralPath $CandidateBinary).Path
$parentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $parentPath).Hash
$candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidatePath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($parentHash -ne $expectedParentHash) { throw "parent hash mismatch: $parentHash" }
if ($candidateHash -ne $expectedCandidateHash) { throw "candidate hash mismatch: $candidateHash" }

$allRows = @(Import-Csv -LiteralPath $Manifest)
$suiteSides = @{
    'multiteam-12' = 12
    'multiteam-16' = 16
    'multiteam-24' = 24
    'multiteam-32' = 32
}
$allowedPlayers = @(8, 9, 10)
$allowedRoles = @('fixed', 'deadline')
$allowedFuel = @('low', 'default', 'high', 'generated')
$expectedPhasePairs = @{ development = 30; holdout = 54 }
$seenCases = @{}
$phasePairs = @{ development = 0; holdout = 0 }
foreach ($row in $allRows) {
    if ($row.experiment_id -ne 'SCORE-SPARSE-ROUTE-CANDIDATE-SUPPLY-248') {
        throw "wrong experiment id: $($row.experiment_id)"
    }
    if ($row.split -notin @('development', 'holdout')) {
        throw "unknown split: $($row.split)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spotCount = [int]$row.spot_count
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or [int]$row.role_ms -ne 5000) {
        throw "invalid frozen work/budget row: $($row.split)/$($row.suite)/$($row.first_seed)"
    }
    if ($players -notin $allowedPlayers -or $row.role_mode -notin $allowedRoles -or
        $row.fuel_profile -notin $allowedFuel) {
        throw "invalid public stratum: $($row.split)/$($row.suite)/$($row.first_seed)"
    }
    if ($row.suite -eq 'general') {
        if ($spotCount -ne 0) { throw 'general suite must derive its spot count' }
    } elseif ($suiteSides.ContainsKey($row.suite)) {
        $side = $suiteSides[$row.suite]
        if ($spotCount -le 0 -or $spotCount -gt $side) {
            throw "spot count $spotCount violates $($row.suite) bound $side"
        }
    } else {
        throw "unknown frozen suite: $($row.suite)"
    }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.split)|$($row.suite)|$seed"
        if ($seenCases.ContainsKey($key)) { throw "duplicate manifest case: $key" }
        $seenCases[$key] = $true
    }
    $phasePairs[$row.split] += $count
}
if ($allRows.Count -ne 24 -or $phasePairs.development -ne 30 -or $phasePairs.holdout -ne 54) {
    throw "expected 24 rows and development/holdout 30/54 pairs, got $($allRows.Count)/$($phasePairs.development)/$($phasePairs.holdout)"
}
if ($ValidateOnly) {
    Write-Output 'validated 24 rows, 30 development pairs and 54 sealed holdout pairs before evidence creation'
    exit 0
}

if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/SCORE-SPARSE-ROUTE-CANDIDATE-SUPPLY-248-$Phase.log"
}
$expectedHeader = @(
    'experiment=SCORE-SPARSE-ROUTE-CANDIDATE-SUPPLY-248'
    "phase=$Phase"
    'public_window_ms=5000'
    "manifest_sha256=$manifestHash"
    "parent_sha256=$parentHash"
    "candidate_sha256=$candidateHash"
)
if ($Resume) {
    if (-not (Test-Path -LiteralPath $Output)) { throw "evidence does not exist: $Output" }
    if (((Get-Content -LiteralPath $Output -TotalCount 6) -join "`n") -ne ($expectedHeader -join "`n")) {
        throw 'evidence header mismatch'
    }
} else {
    if (Test-Path -LiteralPath $Output) { throw "refusing to overwrite evidence: $Output" }
    $expectedHeader | Set-Content -LiteralPath $Output -Encoding utf8
}

$completed = [Collections.Generic.HashSet[string]]::new()
$resultKeys = [Collections.Generic.HashSet[string]]::new()
foreach ($line in Get-Content -LiteralPath $Output) {
    if ($line -match '^case_complete,label=([^,]+),suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
    } elseif ($line -match '^result,version=([^,]+),track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        [void]$resultKeys.Add("$($Matches[1])|$($Matches[2])|$($Matches[3])")
    }
}
foreach ($key in $resultKeys) { if (-not $completed.Contains($key)) { throw "ambiguous partial result: $key" } }
foreach ($key in $completed) { if (-not $resultKeys.Contains($key)) { throw "completion without result: $key" } }

$rows = @($allRows | Where-Object { $_.split -eq $Phase })
$expectedResults = 2 * $expectedPhasePairs[$Phase]
if ($Resume) {
    "resume_begin,completed=$($completed.Count),expected=$expectedResults" |
        Add-Content -LiteralPath $Output -Encoding utf8
}
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'sparse') } else { @('sparse', 'parent') }
        foreach ($label in $labels) {
            $key = "$label|$($row.suite)|$seed"
            if ($completed.Contains($key)) { continue }
            $binary = if ($label -eq 'parent') { $parentPath } else { $candidatePath }
            $arguments = @(
                '--version', $label, '--track', 'sparse-route-candidate-supply-248',
                '--suite', $row.suite, '--first-seed', $seed, '--seeds', 1,
                '--budget-ms', $row.budget_ms, '--role-ms', $row.role_ms,
                '--role-mode', $row.role_mode, '--role-mask', 1,
                '--fuel-profile', $row.fuel_profile, '--spot-count', $row.spot_count,
                '--players', $row.players,
                '--short-role-fallback', 1, '--protected-wait-closed-loop',
                '--protected-wait-ms', 1600, '--terminal-sparse-ms', 5000,
                '--terminal-pair', 1, '--midday-chain', 1, '--midday-pair', 0,
                '--midday-target-followup', 1,
                '--public-window-probe-ms', 5000,
                '--checkpoint-closed-loop', 1, '--day-details'
            )
            if ($label -eq 'sparse') {
                $arguments += @('--sparse-route-candidate-supply', 1)
            }
            $caseOutput = @(& $binary @arguments 2>&1)
            $exitCode = $LASTEXITCODE
            if ($exitCode -ne 0) { throw "$label/$($row.suite)/$seed exited $exitCode" }
            if (@($caseOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
                throw "$label/$($row.suite)/$seed did not emit exactly one result"
            }
            @(
                "case_begin,label=$label,suite=$($row.suite),seed=$seed,role=$($row.role_mode),public_window_ms=5000"
                $caseOutput
                "case_complete,label=$label,suite=$($row.suite),seed=$seed"
            ) | Add-Content -LiteralPath $Output -Encoding utf8
            [void]$completed.Add($key)
        }
    }
}
if ($completed.Count -ne $expectedResults) { throw "completed $($completed.Count), expected $expectedResults" }
"run_complete,results=$($completed.Count),pairs=$($completed.Count / 2)" |
    Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $($completed.Count) results in $Output"
