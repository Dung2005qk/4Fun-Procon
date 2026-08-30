param(
    [Parameter(Mandatory = $true)][string]$Binary,
    [string]$Manifest = 'research/holdouts/ATTR-SPARSE-ROUTE-PREVALENCE-247.csv',
    [string]$Output = 'research/evidence/ATTR-SPARSE-ROUTE-PREVALENCE-247-development.log',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = 'EE08FDF1A997C21C11F84E19FD0AF23172B19F0B3C838647E153AEECC3FD0116'
$expectedBinaryHash = '55F09C751AEA2557FFB7F2BD0B66DD1EC918E778C4D223A8ADC7933F0D6A76F8'
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
$manifestCases = @{}
$expectedPairs = 0
foreach ($row in $rows) {
    if ($row.experiment_id -ne 'ATTR-SPARSE-ROUTE-PREVALENCE-247') {
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
        $side = $suiteSides[$row.suite]
        if ($spotCount -le 0 -or $spotCount -gt $side) {
            throw "spot count $spotCount violates $($row.suite) bound $side"
        }
    } else {
        throw "unknown frozen suite: $($row.suite)"
    }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|$seed"
        if ($manifestCases.ContainsKey($key)) { throw "duplicate manifest case: $key" }
        $manifestCases[$key] = $true
    }
    $expectedPairs += $count
}
if ($rows.Count -ne 12 -or $expectedPairs -ne 24) {
    throw "expected 12 rows and 24 pairs, got $($rows.Count)/$expectedPairs"
}
if ($ValidateOnly) {
    Write-Output "validated 12 rows and 24 fresh pairs before evidence creation"
    exit 0
}

if ((Test-Path -LiteralPath $Output) -and -not $Resume) {
    throw "refusing to overwrite evidence: $Output"
}
if (-not (Test-Path -LiteralPath $Output)) {
    @(
        'experiment=ATTR-SPARSE-ROUTE-PREVALENCE-247'
        "manifest_sha256=$manifestHash"
        "binary_sha256=$binaryHash"
    ) | Set-Content -LiteralPath $Output -Encoding utf8
}
$completed = @{}
Get-Content -LiteralPath $Output | ForEach-Object {
    if ($_ -match '^case_complete,label=([^,]+),suite=([^,]+),seed=([0-9]+)$') {
        $completed["$($Matches[1])|$($Matches[2])|$($Matches[3])"] = $true
    }
}
$written = $completed.Count
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'sparse') } else { @('sparse', 'parent') }
        foreach ($label in $labels) {
            $key = "$label|$($row.suite)|$seed"
            if ($completed.ContainsKey($key)) { continue }
            $arguments = @(
                '--version', $label, '--track', 'sparse-route-prevalence-247',
                '--suite', $row.suite, '--first-seed', $seed, '--seeds', 1,
                '--budget-ms', $row.budget_ms, '--role-ms', $row.role_ms,
                '--role-mode', $row.role_mode, '--role-mask', 1,
                '--fuel-profile', $row.fuel_profile,
                '--spot-count', $row.spot_count,
                '--players', $row.players,
                '--short-role-fallback', 1,
                '--day-details'
            )
            if ($label -eq 'sparse') {
                $arguments += @('--sparse-route-states', 50000)
            }
            "case_begin,label=$label,suite=$($row.suite),seed=$seed" |
                Add-Content -LiteralPath $Output -Encoding utf8
            & $binaryPath @arguments 2>&1 |
                Add-Content -LiteralPath $Output -Encoding utf8
            if ($LASTEXITCODE -ne 0) {
                throw "$label/$($row.suite)/$seed exited $LASTEXITCODE"
            }
            "case_complete,label=$label,suite=$($row.suite),seed=$seed" |
                Add-Content -LiteralPath $Output -Encoding utf8
            ++$written
        }
    }
}
"run_complete,results=$written,pairs=$($written / 2)" |
    Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $written results in $Output"
