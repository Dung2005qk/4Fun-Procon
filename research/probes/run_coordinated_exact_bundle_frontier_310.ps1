param(
    [string]$Binary = "build-release/udonshield_multi_patrol_oracle.exe",
    [string]$Manifest = "research/holdouts/ATTR-THREE-ACTIVE-PATROL-FRONTIER-308.csv",
    [string]$OutputDirectory = "research/evidence/ATTR-COORDINATED-EXACT-BUNDLE-FRONTIER-310"
)

$ErrorActionPreference = "Stop"

$expected = @{
    $Binary = "9E6E77D602BB127DDDAA9775DD8E665D98007AA2A693A37EF32CC5849CFF9504"
    $Manifest = "2BC105B7009F1E3017D7DB960C21D10E4C8A525EFCAA410EA414A119732159CE"
    "include/udon/planner.hpp" = "0D9566FD09CC47DB17E2B7F61D9EEF70F0E0E3313B1B99FD9F5CD7796AB984EE"
    "src/planner.cpp" = "D9E0A81253619039BEB74D19A1278A11A48DC483B85DF346536F91A7683AF40C"
    "research/probes/multi_patrol_oracle.cpp" = "221547286BC011CBE1F4C3D1768294501FB600846B91BA480636E81BF5D489F3"
}
foreach ($entry in $expected.GetEnumerator()) {
    $actual = (Get-FileHash -Algorithm SHA256 -LiteralPath $entry.Key).Hash
    if ($actual -ne $entry.Value) {
        throw "frozen hash mismatch for $($entry.Key): $actual"
    }
}

$cases = @(
    @("10500000", "2.-16|5.5.5.5.5.-8|5.5.5.5.5.-8", "2.2.2.-12|5.5.-14|5.5.5.5.5.5.-6"),
    @("10500001", "2.-14|5.5.5.5.5.-6|5.5.5.5.5.-6", "2.2.2.-10|5.5.-12|5.5.5.5.5.5.-4"),
    @("10500100", "2.-15|5.5.5.5.5.-7|5.5.5.5.5.-7", "2.2.2.2.2.-7|5.5.5.5.5.-7|-17"),
    @("10500101", "2.-16|5.5.5.5.5.-8|5.5.5.5.5.-8", "2.2.2.2.2.-8|5.5.5.5.5.-8|-18")
)

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
foreach ($case in $cases) {
    $seed = $case[0]
    $result = Join-Path $OutputDirectory "$seed.result"
    if (Test-Path -LiteralPath $result) {
        continue
    }
    $partial = Join-Path $OutputDirectory "$seed.partial"
    $stderrPartial = Join-Path $OutputDirectory "$seed.stderr.partial"
    & $Binary `
        --manifest $Manifest `
        --split development `
        --only-seed $seed `
        --inspect-plan $case[1] `
        --inspect-parent-plan $case[2] `
        > $partial 2> $stderrPartial
    if ($LASTEXITCODE -ne 0) {
        throw "case $seed exited with $LASTEXITCODE"
    }
    if ((Select-String -LiteralPath $partial -Pattern '^exact_bundle_frontier_attribute,').Count -ne 1) {
        throw "case $seed did not emit exactly one frontier record"
    }
    if ((Get-Item -LiteralPath $stderrPartial).Length -ne 0) {
        throw "case $seed emitted stderr"
    }
    Remove-Item -LiteralPath $stderrPartial
    Move-Item -LiteralPath $partial -Destination $result
}

$combinedPartial = "$OutputDirectory-development.log.partial"
$combined = "$OutputDirectory-development.log"
$writer = [System.IO.StreamWriter]::new(
    $combinedPartial,
    $false,
    [System.Text.UTF8Encoding]::new($false))
try {
    foreach ($case in $cases) {
        $result = Join-Path $OutputDirectory "$($case[0]).result"
        foreach ($line in [System.IO.File]::ReadLines($result)) {
            $writer.WriteLine($line)
        }
    }
    $writer.WriteLine("run_complete,cases=4")
} finally {
    $writer.Dispose()
}
Move-Item -Force -LiteralPath $combinedPartial -Destination $combined

