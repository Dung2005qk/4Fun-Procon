param(
    [string]$Manifest = 'research/holdouts/ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-v2.csv',
    [string]$Binary = 'build-contract-fix/udonshield_historical_tournament.exe',
    [string]$Output = 'research/evidence/ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-v2-development.log',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$manifestSha256 = '015FFC8F3B0A2FE82A862160FD29CBAB422484FD01C456083288FF83488A9E20'
$binarySha256 = 'C888319A69F4AE16A832CC69E3844407162F002C9AFAD4C073B55D9B1633FBF0'

function Assert-Sha256([string]$PathValue, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $PathValue -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($actual -ne $Expected) {
        throw "SHA256 mismatch for ${PathValue}: expected ${Expected}, got ${actual}"
    }
}

$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
Assert-Sha256 $manifestPath $manifestSha256
Assert-Sha256 $binaryPath $binarySha256
$rows = @(Import-Csv -LiteralPath $manifestPath)
$suiteSides = @{'multiteam-12'=12; 'multiteam-16'=16; 'multiteam-24'=24; 'multiteam-32'=32}
$seen = @{}
$expectedResults = 0
foreach ($row in $rows) {
    if ($row.experiment_id -ne 'ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-v2') {
        throw "wrong experiment id: $($row.experiment_id)"
    }
    $count = [int]$row.count
    $players = [int]$row.players
    $spots = [int]$row.spot_count
    if ($count -le 0 -or [int]$row.budget_ms -ne 3375 -or
        [int]$row.role_ms -ne 5000 -or $players -notin @(8,9,10) -or
        $row.role_mode -notin @('fixed','deadline') -or
        $row.fuel_profile -notin @('low','default','high','generated')) {
        throw "invalid frozen stratum: $($row.suite)/$($row.first_seed)"
    }
    if ($row.suite -eq 'general') {
        if ($spots -ne 0) { throw 'general suite must derive spot count' }
    } elseif ($suiteSides.ContainsKey($row.suite)) {
        $side = [int]$suiteSides[$row.suite]
        if ($spots -le 0 -or $spots -gt ($side * $side - 8)) {
            throw "invalid spot count: $($row.suite)/${spots}"
        }
    } else { throw "unknown suite: $($row.suite)" }
    for ($offset = 0; $offset -lt $count; ++$offset) {
        $key = "$($row.suite)|$([int64]$row.first_seed + $offset)"
        if ($seen.ContainsKey($key)) { throw "duplicate case: ${key}" }
        $seen[$key] = $true
    }
    $expectedResults += $count
}
if ($rows.Count -ne 12 -or $expectedResults -ne 24) {
    throw "expected 12 rows/24 cases, got $($rows.Count)/${expectedResults}"
}
if ($ValidateOnly) {
    Write-Output 'validated 12 rows and 24 replacement official-domain cases'
    exit 0
}

$outputPath = if ([IO.Path]::IsPathRooted($Output)) {
    [IO.Path]::GetFullPath($Output)
} else { [IO.Path]::GetFullPath((Join-Path (Get-Location) $Output)) }
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $outputPath) | Out-Null
$header = @(
    'experiment=ATTR-PERMUTED-TERMINAL-PAIR-YIELD-252-v2'
    "manifest_sha256=${manifestSha256}"
    "binary_sha256=${binarySha256}"
)
if (Test-Path -LiteralPath $outputPath) {
    if (-not $Resume) { throw "refusing to overwrite evidence: ${outputPath}" }
    if (((Get-Content -LiteralPath $outputPath -TotalCount 3) -join "`n") -ne
        ($header -join "`n")) { throw 'evidence header mismatch' }
} else { $header | Set-Content -LiteralPath $outputPath -Encoding utf8 }

$completed = [Collections.Generic.HashSet[string]]::new()
$results = [Collections.Generic.HashSet[string]]::new()
$runComplete = $false
foreach ($line in Get-Content -LiteralPath $outputPath) {
    if ($line -match '^case_complete,suite=([^,]+),seed=([0-9]+)$') {
        [void]$completed.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^result,version=probe,track=[^,]+,suite=([^,]+),.*seed=([0-9]+),') {
        [void]$results.Add("$($Matches[1])|$($Matches[2])")
    } elseif ($line -match '^run_complete,') { $runComplete = $true }
}
foreach ($key in $results) {
    if (-not $completed.Contains($key)) { throw "ambiguous partial result: ${key}" }
}
foreach ($key in $completed) {
    if (-not $results.Contains($key)) { throw "completion without result: ${key}" }
}
if ($runComplete) {
    if ($completed.Count -ne $expectedResults) { throw 'invalid completed count' }
    Write-Output "already complete: $($completed.Count) results"
    exit 0
}
if ($Resume) {
    "resume_begin,completed=$($completed.Count),expected=${expectedResults}" |
        Add-Content -LiteralPath $outputPath -Encoding utf8
}

foreach ($row in $rows) {
    for ($offset = 0; $offset -lt [int]$row.count; ++$offset) {
        $seed = [int64]$row.first_seed + $offset
        $key = "$($row.suite)|${seed}"
        if ($completed.Contains($key)) { continue }
        $arguments = @(
            '--version','probe','--track','permuted-terminal-pair-yield-252-v2',
            '--suite',$row.suite,'--first-seed',$seed,'--seeds',1,
            '--budget-ms',$row.budget_ms,'--role-ms',$row.role_ms,
            '--role-mode',$row.role_mode,'--role-mask',1,
            '--fuel-profile',$row.fuel_profile,'--spot-count',$row.spot_count,
            '--players',$row.players,'--short-role-fallback',1,
            '--protected-wait-closed-loop','--protected-wait-ms',1600,
            '--terminal-sparse-ms',5000,'--terminal-pair',1,
            '--midday-chain',1,'--midday-pair',0,'--midday-target-followup',1,
            '--public-window-probe-ms',5000,'--checkpoint-closed-loop',1,
            '--permuted-terminal-probe-states',50000,'--day-details'
        )
        $caseOutput = @(& $binaryPath @arguments 2>&1)
        if ($LASTEXITCODE -ne 0) {
            throw "$($row.suite)/${seed} exited ${LASTEXITCODE}: $($caseOutput -join [Environment]::NewLine)"
        }
        if (@($caseOutput | Where-Object { $_ -match '^result,' }).Count -ne 1) {
            throw "$($row.suite)/${seed} did not emit exactly one result"
        }
        @(
            "case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players)"
            $caseOutput
            "case_complete,suite=$($row.suite),seed=${seed}"
        ) | Add-Content -LiteralPath $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}
if ($completed.Count -ne $expectedResults) { throw 'final completed count mismatch' }
"run_complete,results=$($completed.Count)" | Add-Content -LiteralPath $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) results in ${outputPath}"
