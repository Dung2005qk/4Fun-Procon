param(
    [Parameter(Mandatory = $true)]
    [string]$Replay,
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [Parameter(Mandatory = $true)]
    [ValidateSet('low', 'default', 'high')]
    [string]$Fuel
)

$ErrorActionPreference = 'Stop'
$events = Get-Content -LiteralPath $Replay | ForEach-Object { $_ | ConvertFrom-Json }
$setup = ($events | Where-Object kind -eq 'setup' | Select-Object -First 1).body
$decisions = @($events | Where-Object kind -eq 'decision')
$actionResults = @($events | Where-Object kind -eq 'action_result')
$kinds = @($events | ForEach-Object kind)
$expectedFuel = @{ low = 100; default = 200; high = 300 }[$Fuel]

$configOk =
    $setup.map.width -eq 32 -and
    $setup.map.height -eq 32 -and
    $setup.players -eq 4 -and
    @($setup.daySteps).Count -eq 10 -and
    @($setup.daySteps | Where-Object { $_ -ne 100 }).Count -eq 0 -and
    @($setup.daySeconds).Count -eq 10 -and
    @($setup.daySeconds | Where-Object { $_ -ne 5 }).Count -eq 0 -and
    @($setup.agents).Count -eq 8 -and
    @($setup.spots).Count -eq 12 -and
    @($setup.spots | ForEach-Object brand | Sort-Object -Unique).Count -eq 6 -and
    $setup.fuelLimits -eq $expectedFuel

$validActions = @($actionResults | Where-Object {
    $_.status -eq 200 -and $_.body.valid -eq $true
}).Count
$decisionReconciledFlags = @($decisions | Where-Object {
    $_.body.decision.reconciledAuthoritativeState -eq $true
}).Count
$emergency = @($decisions | Where-Object { $_.body.decision.emergency -eq $true }).Count
$maxTotalMs = ($decisions | ForEach-Object { [int]$_.body.decision.timing.totalMs } |
    Measure-Object -Maximum).Maximum
$maxExactOverrunMs = ($decisions | ForEach-Object {
    [int]$_.body.decision.audit.columnGeneration.exactOrienteeringDeadlineOverrunMilliseconds
} | Measure-Object -Maximum).Maximum
$deadlineSkips = @($kinds | Where-Object { $_ -eq 'actions_deadline_skip' }).Count
$serverWaits = @($kinds | Where-Object { $_ -eq 'actions_server_wait' }).Count
$fallbacks = @($kinds | Where-Object {
    $_ -eq 'actions_fallback' -or $_ -eq 'actions_recovery_wait'
}).Count
$transportRetries = @($kinds | Where-Object { $_ -like '*transport_retry' }).Count
$results = @($kinds | Where-Object { $_ -eq 'result' }).Count

$replayOutput = & $Executable replay-check --replay $Replay --response-ms 5000 2>&1
$replayExit = $LASTEXITCODE
$replaySummary = ($replayOutput | Select-Object -Last 1)
$reconciled = if ($replaySummary -match 'reconciled_transitions=(\d+)') {
    [int]$Matches[1]
} else {
    -1
}

$summary = [pscustomobject]@{
    replay = Split-Path -Leaf $Replay
    fuel = $Fuel
    config_ok = [int]$configOk
    decisions = $decisions.Count
    valid_actions = $validActions
    reconciled = $reconciled
    decision_reconciled_flags = $decisionReconciledFlags
    emergency = $emergency
    deadline_skips = $deadlineSkips
    server_waits = $serverWaits
    fallbacks = $fallbacks
    transport_retries = $transportRetries
    results = $results
    max_total_ms = $maxTotalMs
    max_exact_overrun_ms = $maxExactOverrunMs
    replay_exit = $replayExit
    replay_summary = $replaySummary
    replay_sha256 = (Get-FileHash -LiteralPath $Replay -Algorithm SHA256).Hash
}
$summary | ConvertTo-Json -Compress

if (-not $configOk -or $decisions.Count -ne 10 -or $validActions -ne 10 -or
    $reconciled -ne 9 -or $emergency -ne 0 -or $deadlineSkips -ne 0 -or
    $serverWaits -ne 0 -or $fallbacks -ne 0 -or $transportRetries -ne 0 -or
    $results -ne 1 -or $maxTotalMs -gt 5000 -or $replayExit -ne 0) {
    exit 2
}
