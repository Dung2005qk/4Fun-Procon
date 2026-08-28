param(
    [Parameter(Mandatory = $true)]
    [string]$ParentBinary,
    [Parameter(Mandatory = $true)]
    [string]$CandidateBinary,
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/PERF-WHOLE-PROGRAM-IPO-233.csv',
    [string]$Output = ''
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '13B63A618EF483F6A448EE644BA55289090DEB76709DCAB57340BA9AFF0288B7'
$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
if ($manifestHash -ne $expectedManifestHash) {
    throw "manifest hash mismatch: $manifestHash"
}

$parentPath = (Resolve-Path -LiteralPath $ParentBinary).Path
$candidatePath = (Resolve-Path -LiteralPath $CandidateBinary).Path
$parentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $parentPath).Hash
$candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidatePath).Hash
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/PERF-WHOLE-PROGRAM-IPO-233-$Phase.log"
}
if (Test-Path -LiteralPath $Output) {
    throw "refusing to overwrite existing evidence: $Output"
}

$rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.split -eq $Phase })
if ($rows.Count -eq 0) {
    throw "manifest has no rows for phase $Phase"
}

@(
    'experiment=PERF-WHOLE-PROGRAM-IPO-233'
    "phase=$Phase"
    "manifest_sha256=$manifestHash"
    "parent_sha256=$parentHash"
    "candidate_sha256=$candidateHash"
) | Set-Content -LiteralPath $Output -Encoding utf8

$completed = 0
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'ipo') } else { @('ipo', 'parent') }
        foreach ($label in $labels) {
            $binary = if ($label -eq 'parent') { $parentPath } else { $candidatePath }
            $arguments = @(
                '--version', $label,
                '--track', 'whole-program-ipo',
                '--suite', $row.suite,
                '--first-seed', $seed,
                '--seeds', 1,
                '--budget-ms', $row.budget_ms,
                '--role-ms', $row.role_ms,
                '--role-mode', $row.role_mode,
                '--role-mask', 1,
                '--fuel-profile', $row.fuel_profile,
                '--spot-count', $row.spot_count,
                '--short-role-fallback', 1,
                '--protected-wait-closed-loop',
                '--protected-wait-ms', 1600,
                '--terminal-sparse-ms', 5000,
                '--terminal-pair', 1,
                '--midday-chain', 1,
                '--midday-pair', 0,
                '--midday-target-followup', 1,
                '--public-window-probe-ms', $row.public_window_ms,
                '--checkpoint-closed-loop', 1,
                '--day-details'
            )
            "case_begin,label=$label,suite=$($row.suite),seed=$seed,role=$($row.role_mode),public_window_ms=$($row.public_window_ms)" |
                Add-Content -LiteralPath $Output -Encoding utf8
            & $binary @arguments 2>&1 | Add-Content -LiteralPath $Output -Encoding utf8
            if ($LASTEXITCODE -ne 0) {
                throw "$label/$($row.suite)/$seed exited $LASTEXITCODE"
            }
            "case_complete,label=$label,suite=$($row.suite),seed=$seed" |
                Add-Content -LiteralPath $Output -Encoding utf8
            ++$completed
        }
    }
}

"run_complete,results=$completed,pairs=$($completed / 2)" |
    Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $completed results in $Output"
