param(
    [ValidateSet("screen", "full")]
    [string]$Mode = "screen",
    [string]$Lane = "",
    [switch]$SkipBuild,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$CheckpointFile = Join-Path $Root "old\CHECKPOINTS.csv"
$MatrixFile = Join-Path $PSScriptRoot "MATRIX.csv"
$RunId = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $PSScriptRoot "results\$RunId-$Mode"

$Checkpoints = Import-Csv $CheckpointFile
$Lanes = Import-Csv $MatrixFile | Where-Object { $_.enabled -eq "1" }
if ($Lane) {
    $Lanes = @($Lanes | Where-Object { $_.lane -eq $Lane })
    if ($Lanes.Count -ne 1) {
        throw "Expected exactly one enabled lane named '$Lane'."
    }
}

if (-not $DryRun) {
    New-Item -ItemType Directory -Force $RunDirectory | Out-Null
    Copy-Item $CheckpointFile (Join-Path $RunDirectory "CHECKPOINTS.csv")
    Copy-Item $MatrixFile (Join-Path $RunDirectory "MATRIX.csv")
}

foreach ($Checkpoint in $Checkpoints) {
    $BuildDirectory = Join-Path $Root "old\builds\$($Checkpoint.label)"
    $Executable = Join-Path $BuildDirectory "historical_tournament.exe"
    if (-not $SkipBuild) {
        $BuildCommand = @("cmake", "--build", $BuildDirectory, "--target", "historical_tournament")
        Write-Host ($BuildCommand -join " ")
        if (-not $DryRun) {
            & $BuildCommand[0] $BuildCommand[1..($BuildCommand.Count - 1)]
            if ($LASTEXITCODE -ne 0) {
                throw "Build failed for $($Checkpoint.label)."
            }
        }
    }
    if (-not $DryRun -and -not (Test-Path $Executable)) {
        throw "Missing tournament executable: $Executable"
    }

    foreach ($MatrixLane in $Lanes) {
        $SeedCount = if ($Mode -eq "screen") {
            [int]$MatrixLane.screen_seeds
        } else {
            [int]$MatrixLane.full_seeds
        }
        $Arguments = @(
            "--version", $Checkpoint.label,
            "--track", $MatrixLane.lane,
            "--suite", $MatrixLane.suite,
            "--first-seed", $MatrixLane.first_seed,
            "--seeds", $SeedCount,
            "--budget-ms", $MatrixLane.budget_ms,
            "--role-ms", $MatrixLane.role_ms,
            "--role-mode", $MatrixLane.role_mode,
            "--role-mask", $MatrixLane.role_mask
        )
        Write-Host "$Executable $($Arguments -join ' ')"
        if ($DryRun) {
            continue
        }
        $OutputPath = Join-Path $RunDirectory "$($MatrixLane.lane)-$($Checkpoint.label).txt"
        & $Executable @Arguments | Tee-Object -FilePath $OutputPath
        if ($LASTEXITCODE -ne 0) {
            throw "Tournament failed for $($Checkpoint.label) / $($MatrixLane.lane)."
        }
    }
}

if (-not $DryRun) {
    & python (Join-Path $PSScriptRoot "summarize_checkpoint_matrix.py") $RunDirectory
    if ($LASTEXITCODE -ne 0) {
        throw "Matrix summary failed."
    }
    Write-Host "Run directory: $RunDirectory"
}
