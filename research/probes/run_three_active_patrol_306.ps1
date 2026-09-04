param(
    [string]$Binary = 'build-research-304-msvc/udonshield_multi_patrol_oracle.exe',
    [string]$Manifest = 'research/holdouts/CEILING-THREE-ACTIVE-PATROL-306.csv',
    [string]$Split = 'development',
    [string]$Output = 'research/evidence/CEILING-THREE-ACTIVE-PATROL-306-development.log',
    [string]$ExpectedManifestSha256,
    [string]$ExpectedBinarySha256,
    [string]$ExpectedSourceSha256
)

$ErrorActionPreference = 'Stop'

function Assert-Hash([string]$Path, [string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "missing frozen input: $Path"
    }
    if ([string]::IsNullOrWhiteSpace($Expected)) {
        throw "missing expected SHA256 for $Path"
    }
    $Actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $Path).Hash
    if ($Actual -ne $Expected) {
        throw "SHA256 mismatch for $Path; expected $Expected actual $Actual"
    }
}

if ($Split -ne 'development') {
    throw '306 runner is development-only; sealed holdout cannot be opened here'
}

Assert-Hash $Manifest $ExpectedManifestSha256
Assert-Hash $Binary $ExpectedBinarySha256
Assert-Hash 'research/probes/multi_patrol_oracle.cpp' $ExpectedSourceSha256

$Lines = & $Binary --manifest $Manifest --split $Split 2>&1
$ExitCode = $LASTEXITCODE
$Lines | Set-Content -LiteralPath $Output -Encoding utf8
$Lines | ForEach-Object { Write-Output $_ }
if ($ExitCode -ne 0) {
    throw "three-active-Patrol oracle exited with code $ExitCode"
}

$CaseCount = @($Lines | Where-Object { $_ -like 'case,*' }).Count
$SummaryCount = @($Lines | Where-Object { $_ -like 'summary,*' }).Count
if ($CaseCount -ne 12 -or $SummaryCount -ne 1) {
    throw "incomplete 306 development evidence: cases=$CaseCount summaries=$SummaryCount"
}
