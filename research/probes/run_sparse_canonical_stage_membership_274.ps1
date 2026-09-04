param()

$ErrorActionPreference = 'Stop'

$workspace = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$source = Join-Path $workspace 'research\probes\claim_probe.cpp'
$binary = Join-Path $workspace 'build-research-268-msvc\udonshield_claim_probe.exe'
$replay6134 = Join-Path $workspace 'artifacts\btc\m-6134-ab3d699-series.jsonl'
$replay6213 = Join-Path $workspace 'artifacts\btc\m-6213-ab3d699-series.jsonl'
$evidence = Join-Path $workspace 'research\evidence'
$resultDirectory = Join-Path $evidence 'ATTR-SPARSE-CANONICAL-STAGE-MEMBERSHIP-274-results'
$combinedLog = Join-Path $evidence 'ATTR-SPARSE-CANONICAL-STAGE-MEMBERSHIP-274-development.log'

$expectedHashes = @{
    $source = '13D12C134E095D5284CD5C5E9BE5A71EAD72E5DB6055E6A16ABA723372604D54'
    $binary = 'DF182A9D4366E7D01175FA1B126E9367A09708E84532CA6C216C2DA5C94E370F'
    $replay6134 = '9A0AADF8A2DCA35C3C3E6B77FD337A3352085D9880A81BE72010DDE9B26D6BB5'
    $replay6213 = 'FC3D31AC086D2F7ABE8FCBB00802648EAB9675C8C7F86891AC45A35C7AF0F5DA'
}
foreach ($entry in $expectedHashes.GetEnumerator()) {
    $actual = (Get-FileHash -LiteralPath $entry.Key -Algorithm SHA256).Hash
    if ($actual -ne $entry.Value) {
        throw "frozen hash mismatch: $($entry.Key) expected=$($entry.Value) actual=$actual"
    }
}

New-Item -ItemType Directory -Force -Path $resultDirectory | Out-Null
if (Test-Path -LiteralPath $combinedLog) {
    throw "combined evidence already exists: $combinedLog"
}

$cases = @(
    [pscustomobject]@{ Id = '01-m6134-d1'; Replay = $replay6134; Day = 1; Stratum = 'early' },
    [pscustomobject]@{ Id = '02-m6134-d4'; Replay = $replay6134; Day = 4; Stratum = 'middle' },
    [pscustomobject]@{ Id = '03-m6134-d8'; Replay = $replay6134; Day = 8; Stratum = 'late' },
    [pscustomobject]@{ Id = '04-m6213-d1'; Replay = $replay6213; Day = 1; Stratum = 'early' },
    [pscustomobject]@{ Id = '05-m6213-d5'; Replay = $replay6213; Day = 5; Stratum = 'middle' },
    [pscustomobject]@{ Id = '06-m6213-d8'; Replay = $replay6213; Day = 8; Stratum = 'late' }
)

foreach ($case in $cases) {
    $resultPath = Join-Path $resultDirectory "$($case.Id).result"
    $partialPath = "$resultPath.partial"
    if (Test-Path -LiteralPath $resultPath) {
        continue
    }
    if (Test-Path -LiteralPath $partialPath) {
        throw "ambiguous partial result exists: $partialPath"
    }
    $started = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
    $caseOutput = @(
        & $binary $case.Replay '--recorded-sparse-stage' "$($case.Day)" 2>&1 |
            ForEach-Object { "$_" }
    )
    $exitCode = $LASTEXITCODE
    $finished = [DateTimeOffset]::UtcNow.ToUnixTimeMilliseconds()
    $record = @(
        "case_id=$($case.Id) replay=$([IO.Path]::GetFileName($case.Replay)) day=$($case.Day) stratum=$($case.Stratum) started_unix_ms=$started"
        $caseOutput
        "case_complete=$($case.Id) exit_code=$exitCode elapsed_ms=$($finished - $started)"
    )
    Set-Content -LiteralPath $partialPath -Value $record -Encoding utf8
    if ($exitCode -ne 0) {
        throw "probe failed for $($case.Id); partial evidence preserved at $partialPath"
    }
    Move-Item -LiteralPath $partialPath -Destination $resultPath
}

$results = Get-ChildItem -LiteralPath $resultDirectory -Filter '*.result' |
    Sort-Object Name
if ($results.Count -ne $cases.Count) {
    throw "expected $($cases.Count) atomic results, found $($results.Count)"
}
$combined = foreach ($result in $results) {
    Get-Content -LiteralPath $result.FullName
}
$combined += "run_complete=ATTR-SPARSE-CANONICAL-STAGE-MEMBERSHIP-274 cases=$($cases.Count)"
Set-Content -LiteralPath $combinedLog -Value $combined -Encoding utf8
Write-Output "completed cases=$($cases.Count) log=$combinedLog"
