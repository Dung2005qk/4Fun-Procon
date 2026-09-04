param(
    [ValidateSet('development', 'holdout')]
    [string]$Phase = 'development',
    [string]$Manifest = 'research/holdouts/SCORE-MIDDAY-PREFERRED-BRAND-SPARSE-FLOOR-268.csv',
    [string]$Binary = 'build-research-268-msvc/udonshield_historical_tournament.exe',
    [string]$Output = '',
    [string]$BinarySha256 = 'CCF728E9206141DE7A14D054E0F7E0F71A670DA972C5D91632B75E9B8A380CBC',
    [switch]$ValidateOnly,
    [switch]$Resume
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$expectedManifestSha256 = '30F98957323E74AE66E6BCF158482E9E9629C7B4133E8496E052221A40295316'
$suiteSides = @{'multiteam-12'=12;'multiteam-16'=16;'multiteam-24'=24;'multiteam-32'=32}

function Assert-Sha256([string]$PathValue, [string]$Expected) {
    $actual = (Get-FileHash -LiteralPath $PathValue -Algorithm SHA256).Hash
    if ($actual -ne $Expected) { throw "SHA256 mismatch for ${PathValue}: expected ${Expected}, got ${actual}" }
}

$manifestPath = (Resolve-Path -LiteralPath $Manifest).Path
$binaryPath = (Resolve-Path -LiteralPath $Binary).Path
Assert-Sha256 $manifestPath $expectedManifestSha256
Assert-Sha256 $binaryPath $BinarySha256
$allRows = @(Import-Csv -LiteralPath $manifestPath)
$caseCounts = @{development=0;holdout=0}
$seen = [Collections.Generic.HashSet[string]]::new()
foreach($row in $allRows) {
    if($row.experiment_id -ne 'SCORE-MIDDAY-PREFERRED-BRAND-SPARSE-FLOOR-268' -or $row.split -notin @('development','holdout')) { throw "invalid experiment/split" }
    $count=[int]$row.count;$players=[int]$row.players;$spots=[int]$row.spot_count;$window=[int]$row.public_window_ms
    if($count -le 0 -or [int]$row.budget_ms -ne 3375 -or [int]$row.role_ms -ne 5000 -or $players -notin @(8,9,10) -or $window -notin @(5000,10000,15000) -or $row.role_mode -notin @('fixed','deadline') -or $row.fuel_profile -notin @('low','default','high','generated')) { throw "invalid frozen stratum $($row.suite)/$($row.first_seed)" }
    if($row.suite -eq 'general') { if($spots -ne 0){throw 'general spot count'} }
    elseif($suiteSides.ContainsKey($row.suite)) { $side=[int]$suiteSides[$row.suite];if($spots -le 0 -or $spots -gt ($side*$side-$players)){throw 'invalid spot count'} }
    else { throw "unknown suite $($row.suite)" }
    foreach($offset in 0..($count-1)){ if(-not $seen.Add("$($row.split)|$($row.suite)|$([int64]$row.first_seed+$offset)")){throw 'duplicate case'} }
    $caseCounts[$row.split]+=$count
}
if($allRows.Count -ne 24 -or $caseCounts.development -ne 30 -or $caseCounts.holdout -ne 54){throw 'manifest count mismatch'}
if($ValidateOnly){Write-Output 'validated 24 rows, 30 development and 54 sealed holdout cases';exit 0}

$expected=[int]$caseCounts[$Phase]
if([string]::IsNullOrWhiteSpace($Output)){$Output="research/evidence/SCORE-MIDDAY-PREFERRED-BRAND-SPARSE-FLOOR-268-${Phase}.log"}
$outputPath=[IO.Path]::GetFullPath((Join-Path (Get-Location) $Output))
[IO.Directory]::CreateDirectory((Split-Path -Parent $outputPath))|Out-Null
if((Test-Path $outputPath)-and -not $Resume){throw 'output exists; use -Resume after atomic validation'}
if(-not(Test-Path $outputPath)){@("run_begin,experiment=SCORE-MIDDAY-PREFERRED-BRAND-SPARSE-FLOOR-268,phase=${Phase},expected=${expected}","frozen,manifest_sha256=${expectedManifestSha256},binary_sha256=${BinarySha256}")|Set-Content $outputPath -Encoding utf8}
$completed=[Collections.Generic.HashSet[string]]::new();$resultKeys=[Collections.Generic.HashSet[string]]::new();$runComplete=$false
foreach($line in Get-Content $outputPath){
    if($line -match '^case_complete,suite=([^,]+),seed=(\d+)$'){[void]$completed.Add("$($Matches[1])|$($Matches[2])")}
    elseif($line -match '^result,version=causal-268,track=[^,]+,suite=([^,]+),.*seed=(\d+),'){[void]$resultKeys.Add("$($Matches[1])|$($Matches[2])")}
    elseif($line -match '^run_complete,'){$runComplete=$true}
}
foreach($key in $resultKeys){if(-not $completed.Contains($key)){throw "ambiguous partial result $key"}}
foreach($key in $completed){if(-not $resultKeys.Contains($key)){throw "completion without result $key"}}
if($runComplete){if($completed.Count -ne $expected){throw 'bad completed count'};Write-Output "already complete $expected";exit 0}
if($Resume){"resume_begin,completed=$($completed.Count),expected=${expected}"|Add-Content $outputPath -Encoding utf8}

foreach($row in @($allRows|Where-Object split -eq $Phase)){
    for($offset=0;$offset -lt [int]$row.count;++$offset){
        $seed=[int64]$row.first_seed+$offset;$key="$($row.suite)|${seed}";if($completed.Contains($key)){continue}
        $arguments=@('--version','causal-268','--track','midday-preferred-brand-sparse-floor-268','--suite',$row.suite,'--first-seed',$seed,'--seeds',1,'--budget-ms',$row.budget_ms,'--role-ms',$row.role_ms,'--role-mode',$row.role_mode,'--role-mask',1,'--fuel-profile',$row.fuel_profile,'--spot-count',$row.spot_count,'--players',$row.players,'--short-role-fallback',1,'--protected-wait-closed-loop','--protected-wait-ms',1600,'--terminal-sparse-ms',5000,'--terminal-pair',1,'--terminal-marginal-reservoir',1,'--midday-chain',1,'--midday-pair',0,'--midday-target-followup',1,'--preferred-brand-sparse-floor',1,'--public-window-probe-ms',$row.public_window_ms,'--checkpoint-closed-loop',1,'--day-details')
        $caseOutput=@(& $binaryPath @arguments 2>&1);$exitCode=$LASTEXITCODE
        if($exitCode -ne 0){throw "$key exited $exitCode`n$($caseOutput -join [Environment]::NewLine)"}
        if(@($caseOutput|Where-Object{$_ -match '^result,'}).Count -ne 1){throw "$key emitted invalid result count"}
        @("case_begin,suite=$($row.suite),seed=${seed},role=$($row.role_mode),players=$($row.players),public_window_ms=$($row.public_window_ms)",$caseOutput,"case_complete,suite=$($row.suite),seed=${seed}")|Add-Content $outputPath -Encoding utf8
        [void]$completed.Add($key)
    }
}
if($completed.Count -ne $expected){throw "completed $($completed.Count), expected $expected"}
"run_complete,results=$($completed.Count)"|Add-Content $outputPath -Encoding utf8
Write-Output "completed $($completed.Count) results in $outputPath"
