param(
    [string]$Probe = "build-research-258-msvc/udonshield_claim_probe.exe",
    [string]$Replay = "artifacts/btc/m-5846-ab3d699-series.jsonl",
    [string]$Output = "research/evidence/ATTR-MULTITEAM-SHORT-HORIZON-LARGE-MAP-DEFICIT-267-sparse.log"
)

$ErrorActionPreference = "Stop"
$expectedProbe = "B6B05A3603DC4318B04A99F27B7936AEA3DDCF9917A8DE5001D6750214C7CE94"
$expectedReplay = "D0FC728FA84271885074911D24477A192008E6BDFEDA059A0E8B27249146414F"

function Assert-Hash([string]$Path, [string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Missing frozen input: $Path"
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    if ($actual -ne $Expected) {
        throw "Frozen hash mismatch for ${Path}: expected $Expected actual $actual"
    }
}

Assert-Hash $Probe $expectedProbe
Assert-Hash $Replay $expectedReplay
if (Test-Path -LiteralPath $Output) {
    throw "Refusing to overwrite attribution evidence: $Output"
}

$parent = Split-Path -Parent $Output
if ($parent) {
    [System.IO.Directory]::CreateDirectory($parent) | Out-Null
}
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$patrolAgents = @(0, 1, 2, 3, 5, 6, 7)
$cases = [System.Collections.Generic.List[string]]::new()

foreach ($day in 1..4) {
    foreach ($agent in $patrolAgents) {
        $lines = @(& $Probe $Replay --recorded-sparse-budget $day $agent 50000 64 2>&1)
        $exitCode = $LASTEXITCODE
        $cases.Add("case_begin day=$day agent=$agent max_states=50000 max_routes=64")
        foreach ($line in $lines) {
            $cases.Add([string]$line)
        }
        $cases.Add("case_complete day=$day agent=$agent exit=$exitCode")
        if ($exitCode -ne 0) {
            [System.IO.File]::WriteAllLines("$Output.partial", $cases, $utf8NoBom)
            throw "Sparse attribution failed for day=$day agent=$agent; partial evidence preserved"
        }
    }
}

$cases.Add("run_complete cases=28 max_states=50000 max_routes=64")
[System.IO.File]::WriteAllLines($Output, $cases, $utf8NoBom)
