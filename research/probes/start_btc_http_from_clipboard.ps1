param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^m-[0-9]+$')]
    [string] $MatchId,

    [ValidateRange(1, 600000)]
    [int] $ResponseMs = 5000,

    [string] $ArtifactStem = '',

    [switch] $ReadTokenFromStdin
)

$ErrorActionPreference = 'Stop'

$repoRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$binaryPath = Join-Path $repoRoot 'build-release\udonshield_btc.exe'
if (-not (Test-Path -LiteralPath $binaryPath -PathType Leaf)) {
    throw "BTC binary not found: $binaryPath"
}

$active = Get-Process -Name 'udonshield_btc' -ErrorAction SilentlyContinue
if ($active) {
    throw 'An udonshield_btc process is already active; refusing to duplicate it.'
}

if ([string]::IsNullOrWhiteSpace($ArtifactStem)) {
    $ArtifactStem = "$MatchId-series"
}
if ($ArtifactStem -notmatch '^[A-Za-z0-9._-]+$') {
    throw 'ArtifactStem contains unsupported characters.'
}

$artifactDirectory = Join-Path $repoRoot 'artifacts\btc'
[System.IO.Directory]::CreateDirectory($artifactDirectory) | Out-Null
$replayPath = Join-Path $artifactDirectory "$ArtifactStem.jsonl"
$stdoutPath = Join-Path $artifactDirectory "$ArtifactStem.stdout.log"
$stderrPath = Join-Path $artifactDirectory "$ArtifactStem.stderr.log"

$teamToken = if ($ReadTokenFromStdin) {
    $tokenBuffer = [System.Text.StringBuilder]::new()
    while ($true) {
        $key = [Console]::ReadKey($true)
        if ($key.Key -eq [ConsoleKey]::Enter) {
            break
        }
        if ($key.Key -eq [ConsoleKey]::Backspace) {
            if ($tokenBuffer.Length -gt 0) {
                $tokenBuffer.Length--
            }
            continue
        }
        [void] $tokenBuffer.Append($key.KeyChar)
    }
    $tokenBuffer.ToString().Trim()
} else {
    (Get-Clipboard -Raw).Trim()
}
if ($teamToken -notmatch '^bot-[A-Za-z0-9]+$') {
    throw 'The saved BTC token was not copied correctly.'
}

try {
    $env:HEXUDON_TOKEN = $teamToken
    $arguments = @(
        'http',
        '--match', $MatchId,
        '--url', 'https://procon.ptit.edu.vn',
        '--response-ms', $ResponseMs.ToString(),
        '--poll-ms', '220',
        '--action-ack-ms', '750',
        '--replay', $replayPath
    )
    $startParameters = @{
        FilePath = $binaryPath
        ArgumentList = $arguments
        WorkingDirectory = $repoRoot
        RedirectStandardOutput = $stdoutPath
        RedirectStandardError = $stderrPath
        WindowStyle = 'Hidden'
        PassThru = $true
    }
    $process = Start-Process @startParameters
} finally {
    Remove-Item Env:HEXUDON_TOKEN -ErrorAction SilentlyContinue
    if (-not $ReadTokenFromStdin) {
        Set-Clipboard -Value ''
    }
    $teamToken = $null
}

[pscustomobject]@{
    pid = $process.Id
    match_id = $MatchId
    response_ms = $ResponseMs
    replay = $replayPath
    stdout = $stdoutPath
    stderr = $stderrPath
} | ConvertTo-Json -Compress
