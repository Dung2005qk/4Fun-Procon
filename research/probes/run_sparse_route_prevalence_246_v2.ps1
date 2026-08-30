param(
    [Parameter(Mandatory = $true)][string]$Binary,
    [string]$Manifest = 'research/holdouts/ATTR-SPARSE-ROUTE-PREVALENCE-246-v2.csv',
    [string]$Output = 'research/evidence/ATTR-SPARSE-ROUTE-PREVALENCE-246-v2-development.log',
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '96A94FB56EF84D2EB766034CF2FF11238A73353CBD45E3DD0CA125244357E16D'
$expectedBinaryHash = '55F09C751AEA2557FFB7F2BD0B66DD1EC918E778C4D223A8ADC7933F0D6A76F8'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
$binaryHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $binaryPath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($binaryHash -ne $expectedBinaryHash) { throw "binary hash mismatch: $binaryHash" }
if ((Test-Path -LiteralPath $Output) -and -not $Resume) {
    throw "refusing to overwrite evidence: $Output"
}
if (-not (Test-Path -LiteralPath $Output)) {
    @(
        'experiment=ATTR-SPARSE-ROUTE-PREVALENCE-246-v2'
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
$rows = @(Import-Csv -LiteralPath $Manifest)
$written = $completed.Count
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'sparse') } else { @('sparse', 'parent') }
        foreach ($label in $labels) {
            $key = "$label|$($row.suite)|$seed"
            if ($completed.ContainsKey($key)) { continue }
            $arguments = @(
                '--version', $label, '--track', 'sparse-route-prevalence-v2',
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
