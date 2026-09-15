param(
    [Parameter(Mandatory=$true)][ValidatePattern('^m-[0-9]+$')][string]$MatchId,
    [Parameter(Mandatory=$true)][ValidateSet('inactive5000','active10000')][string]$Lane
)
$ErrorActionPreference = 'Stop'
$taskRoot370 = 'C:\Users\LMC\Desktop\4Fun'
$candidate370 = Join-Path $taskRoot370 'artifacts\research\370\build\udonshield_btc.exe'
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $candidate370).Hash -ne '8C37BB43A2938AB315DE4DE1BD22FDB6E61E6EBB02C1DD269AF5E172CB0E4A8E') { throw 'Frozen candidate mismatch' }
if (@(Get-Process -Name udonshield_btc -ErrorAction SilentlyContinue).Count -ne 0) { throw 'A BTC bot is already running; inspect rather than duplicate' }
$replay370 = Join-Path $taskRoot370 "artifacts\btc\$MatchId-integration370-$Lane.jsonl"
$out370 = $replay370 + '.stdout'
$err370 = $replay370 + '.stderr'
foreach ($target370 in @($replay370,$out370,$err370)) {
    if (Test-Path -LiteralPath $target370) { throw 'Existing evidence; no duplicate accepted-day launch' }
}
$token370 = ([string](Get-Clipboard -Raw)).Trim()
if ($token370 -notmatch '^bot-[a-f0-9]{20,}$') { throw 'The explicitly copied team token is not available in clipboard' }
$previousToken370 = $env:HEXUDON_TOKEN
try {
    $env:HEXUDON_TOKEN = $token370
    $process370 = Start-Process -FilePath $candidate370 -ArgumentList @('http','--match',$MatchId,'--response-ms','5000','--replay',('"' + $replay370 + '"')) -WorkingDirectory $taskRoot370 -WindowStyle Hidden -RedirectStandardOutput $out370 -RedirectStandardError $err370 -PassThru
    [pscustomobject]@{Match=$MatchId;Lane=$Lane;PID=$process370.Id;StartedUtc=(Get-Date).ToUniversalTime().ToString('o');Replay=$replay370;BinarySHA256='8C37BB43A2938AB315DE4DE1BD22FDB6E61E6EBB02C1DD269AF5E172CB0E4A8E'} | ConvertTo-Json -Compress
} finally {
    $env:HEXUDON_TOKEN = $previousToken370
    Remove-Variable token370,previousToken370 -ErrorAction SilentlyContinue
}
