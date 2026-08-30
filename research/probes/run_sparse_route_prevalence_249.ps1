param(
    [Parameter(Mandatory = $true)][string]$Binary,
    [string]$Manifest = 'research/holdouts/ATTR-SPARSE-ROUTE-PREVALENCE-249.csv',
    [string]$Output = 'research/evidence/ATTR-SPARSE-ROUTE-PREVALENCE-249-development.log',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '2E4F5289871D1A76611CD5CB1C367C4E5A7B5F56CC7B034E661CFE07587D3939'
$expectedBinaryHash = '07F9E0CE5AC63F01F23E0B6990B276B32CD65B4632C9AD1813969B42416CAF79'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
$binaryHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $binaryPath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($binaryHash -ne $expectedBinaryHash) { throw "binary hash mismatch: $binaryHash" }

$rows = @(Import-Csv -LiteralPath $Manifest)
$suiteSides = @{
    'multiteam-12' = 12
    'multiteam-16' = 16
    'multiteam-24' = 24
    'multiteam-32' = 32
}
$allowedPlayers = @(8, 9, 10)
$allowedRoles = @('fixed', 'deadline')
$allowedFuel = @('low', 'default', 'high', 'generated')
$seen = @{}
$expectedPairs = 0
foreach ($row in $rows) {
    if ($row.experiment_id -ne 'ATTR-SPARSE-ROUTE-PREVALENCE-249') {
        throw "wrong experiment id: $($row.experiment_id)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spotCount = [int]$row.spot_count
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or [int]$row.role_ms -ne 5000) {
        throw "invalid frozen work/budget row: $($row.suite)/$($row.first_seed)"
    }
    if ($players -notin $allowedPlayers -or $row.role_mode -notin $allowedRoles -or
        $row.fuel_profile -notin $allowedFuel) {
        throw "invalid public stratum: $($row.suite)/$($row.first_seed)"
    }
    if ($row.suite -eq 'general') {
        if ($spotCount -ne 0) { throw 'general suite must derive its spot count' }
    } elseif ($suiteSides.ContainsKey($row.suite)) {
        $side = [int]$suiteSides[$row.suite]
        if ($spotCount -le 0 -or $spotCount -gt ($side * $side - 8)) {
            throw "spot count cannot fit eligible cells: $($row.suite)/$spotCount"
        }
    } else {
        throw "unknown frozen suite: $($row.suite)"
    }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|$seed"
        if ($seen.ContainsKey($key)) { throw "duplicate manifest case: $key" }
        $seen[$key] = $true
    }
    $expectedPairs += $count
}
if ($rows.Count -ne 12 -or $expectedPairs -ne 24) {
    throw "expected 12 rows and 24 pairs, got $($rows.Count)/$expectedPairs"
}
if ($ValidateOnly) {
    Write-Output 'validated 12 rows and 24 fresh official-domain pairs before evidence creation'
    exit 0
}

$header = @(
    'experiment=ATTR-SPARSE-ROUTE-PREVALENCE-249'
    "manifest_sha256=$manifestHash"
    "binary_sha256=$binaryHash"
)
if (Test-Path -LiteralPath $Output) {
    if (-not $Resume) { throw "refusing to overwrite evidence: $Output" }
    if (((Get-Content -LiteralPath $Output -TotalCount 3) -join "`n") -ne ($header -join "`n")) {
        throw 'evidence header mismatch'
    }
} else {
    $header | Set-Content -LiteralPath $Output -Encoding utf8
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
foreach ($key in $resultKeys) {
    if (-not $completed.Contains($key)) { throw "ambiguous partial result: $key" }
}
foreach ($key in $completed) {
    if (-not $resultKeys.Contains($key)) { throw "completion without result: $key" }
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'sparse') } else { @('sparse', 'parent') }
        foreach ($label in $labels) {
            $key = "$label|$($row.suite)|$seed"
            if ($completed.Contains($key)) { continue }
            $arguments = @(
                '--version', $label, '--track', 'sparse-route-prevalence-249',
                '--suite', $row.suite, '--first-seed', $seed, '--seeds', 1,
                '--budget-ms', $row.budget_ms, '--role-ms', $row.role_ms,
                '--role-mode', $row.role_mode, '--role-mask', 1,
                '--fuel-profile', $row.fuel_profile, '--spot-count', $row.spot_count,
                '--players', $row.players, '--short-role-fallback', 1, '--day-details'
            )
            if ($label -eq 'sparse') { $arguments += @('--sparse-route-states', 50000) }
            $caseOutput = @(& $binaryPath @arguments 2>&1)
            if ($LASTEXITCODE -ne 0) { throw "$label/$($row.suite)/$seed exited $LASTEXITCODE" }
            if (@($caseOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
                throw "$label/$($row.suite)/$seed did not emit exactly one result"
            }
            @(
                "case_begin,label=$label,suite=$($row.suite),seed=$seed"
                $caseOutput
                "case_complete,label=$label,suite=$($row.suite),seed=$seed"
            ) | Add-Content -LiteralPath $Output -Encoding utf8
            [void]$completed.Add($key)
        }
    }
}
if ($completed.Count -ne 48) { throw "completed $($completed.Count), expected 48" }
"run_complete,results=48,pairs=24" | Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed 48 results in $Output"
