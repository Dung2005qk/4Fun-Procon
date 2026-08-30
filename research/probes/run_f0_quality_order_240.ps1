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
if (Test-Path -LiteralPath $Output) { throw "refusing to overwrite evidence: $Output" }
$rows = @(Import-Csv -LiteralPath $Manifest | Where-Object { $_.split -eq $Phase })
if ($rows.Count -eq 0) { throw "manifest has no rows for $Phase" }
@(
    'experiment=SCORE-F0-QUALITY-ORDER-240'
    "phase=$Phase"
    "manifest_sha256=$manifestHash"
    "parent_sha256=$parentHash"
    "candidate_sha256=$candidateHash"
) | Set-Content -LiteralPath $Output -Encoding utf8
$completed = 0
foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        # The frozen 236 summarizer names its generic candidate side "signature".
        $labels = if (($seed % 2) -eq 0) { @('parent', 'signature') } else { @('signature', 'parent') }
        foreach ($label in $labels) {
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
