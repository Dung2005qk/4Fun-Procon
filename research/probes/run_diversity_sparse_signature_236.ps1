param(
    [Parameter(Mandatory = $true)][string]$ParentBinary,
    [Parameter(Mandatory = $true)][string]$CandidateBinary,
    [ValidateSet('development', 'holdout')][string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/PERF-DIVERSITY-SPARSE-SIGNATURE-236.csv',
    [string]$Output = ''
)

$ErrorActionPreference = 'Stop'
$expectedManifestHash = '70A338D104DCF70AB8F5D857BDC80AAB5BB7363F83B9CE9DA635C4B85708BA4E'
$expectedParentHash = '7438B2FA9171B2492E9C6DBE38C73584F3C10AD41A1F18F06A53F4932BB37739'
$expectedCandidateHash = 'EBF649E6E45B902936F80B31D5E125B804BC078D0486DBBA0E2DA829E6AC619C'

$manifestHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $Manifest).Hash
$parentPath = (Resolve-Path -LiteralPath $ParentBinary).Path
$candidatePath = (Resolve-Path -LiteralPath $CandidateBinary).Path
$parentHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $parentPath).Hash
$candidateHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $candidatePath).Hash
if ($manifestHash -ne $expectedManifestHash) { throw "manifest hash mismatch: $manifestHash" }
if ($parentHash -ne $expectedParentHash) { throw "parent hash mismatch: $parentHash" }
if ($candidateHash -ne $expectedCandidateHash) { throw "candidate hash mismatch: $candidateHash" }
if ([string]::IsNullOrWhiteSpace($Output)) {
    $Output = "research/evidence/PERF-DIVERSITY-SPARSE-SIGNATURE-236-$Phase.log"
}
if (Test-Path -LiteralPath $Output) { throw "refusing to overwrite evidence: $Output" }

$rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.split -eq $Phase })
if ($rows.Count -eq 0) { throw "manifest has no rows for $Phase" }
@(
    'experiment=PERF-DIVERSITY-SPARSE-SIGNATURE-236'
    "phase=$Phase"
    "manifest_sha256=$manifestHash"
    "parent_sha256=$parentHash"
    "candidate_sha256=$candidateHash"
) | Set-Content -LiteralPath $Output -Encoding utf8

$completed = 0
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $labels = if (($seed % 2) -eq 0) { @('parent', 'signature') } else { @('signature', 'parent') }
        foreach ($label in $labels) {
            $binary = if ($label -eq 'parent') { $parentPath } else { $candidatePath }
            $arguments = @(
                '--version', $label, '--track', 'diversity-sparse-signature',
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
            "case_begin,label=$label,suite=$($row.suite),seed=$seed,role=$($row.role_mode),public_window_ms=$($row.public_window_ms)" |
                Add-Content -LiteralPath $Output -Encoding utf8
            & $binary @arguments 2>&1 | Add-Content -LiteralPath $Output -Encoding utf8
            if ($LASTEXITCODE -ne 0) { throw "$label/$($row.suite)/$seed exited $LASTEXITCODE" }
            "case_complete,label=$label,suite=$($row.suite),seed=$seed" |
                Add-Content -LiteralPath $Output -Encoding utf8
            ++$completed
        }
    }
}
"run_complete,results=$completed,pairs=$($completed / 2)" |
    Add-Content -LiteralPath $Output -Encoding utf8
Write-Output "completed $completed results in $Output"
