param(
    [string]$Root = (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [string]$Manifest = "research/holdouts/ATTR-MASTER-FACTOR-SEPARABILITY-291.csv",
    [string]$Binary = "build-release/udonshield_claim_probe.exe",
    [string]$Output = "research/evidence/ATTR-MASTER-FACTOR-SEPARABILITY-291-development.log"
)

$ErrorActionPreference = "Stop"
$expectedManifest = "AEA522CF303E2ADBE152E83EDEB0D8CA4EAB157C7F20FB8C539037B7E6313F92"
$expectedBinary = "01329BA4EF91B02FE7A60D1D1A9E7D399BCFB0B3FD084127AC11DA1F0566A4C9"
$manifestPath = Join-Path $Root $Manifest
$binaryPath = Join-Path $Root $Binary
$outputPath = Join-Path $Root $Output

if ((Get-FileHash -Algorithm SHA256 -LiteralPath $manifestPath).Hash -ne $expectedManifest) {
    throw "manifest hash mismatch"
}
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $binaryPath).Hash -ne $expectedBinary) {
    throw "probe binary hash mismatch"
}
if (Test-Path -LiteralPath $outputPath) {
    throw "output already exists; ATTR-291 is not resumable"
}

$rows = @(Import-Csv -LiteralPath $manifestPath)
if ($rows.Count -ne 28) {
    throw "expected exactly 28 manifest rows"
}
$hashCache = @{}
$started = [System.Diagnostics.Stopwatch]::StartNew()
foreach ($row in $rows) {
    $replayPath = Join-Path $Root $row.replay
    if (-not $hashCache.ContainsKey($replayPath)) {
        $hashCache[$replayPath] = (Get-FileHash -Algorithm SHA256 -LiteralPath $replayPath).Hash
    }
    if ($hashCache[$replayPath] -ne $row.replay_sha256) {
        throw "replay hash mismatch for $($row.case_id)"
    }
    Add-Content -LiteralPath $outputPath -Encoding utf8 -Value (
        "case_begin id={0} replay={1} day={2}" -f $row.case_id, $row.replay, $row.day)
    $caseOutput = @(& $binaryPath $replayPath --recorded-master-factor $row.day 2>&1)
    $exitCode = $LASTEXITCODE
    foreach ($line in $caseOutput) {
        Add-Content -LiteralPath $outputPath -Encoding utf8 -Value ([string]$line)
    }
    if ($exitCode -ne 0) {
        Add-Content -LiteralPath $outputPath -Encoding utf8 -Value (
            "case_failure id={0} exit={1}" -f $row.case_id, $exitCode)
        throw "probe failed for $($row.case_id)"
    }
    Add-Content -LiteralPath $outputPath -Encoding utf8 -Value (
        "case_complete id={0} elapsed_ms={1}" -f $row.case_id, $started.ElapsedMilliseconds)
}
Add-Content -LiteralPath $outputPath -Encoding utf8 -Value (
    "run_complete cases=28 elapsed_ms={0}" -f $started.ElapsedMilliseconds)
