# ATTR-ROAD-EQUIVALENT-SPARSE-YIELD-278

Status: closed negative attribution; no production source candidate and no
holdout.

## Frozen inputs

- Manifest SHA256: `41A331B72D197C102A620BB57F8893DEF4A53A69A914D30260D2EFA8611C9683`
- Probe source SHA256: `0CA860C2A5BB7588267A1A8F37FAE9200BA33CA00CE742BC783448B8D0B6FEA2`
- Probe binary SHA256: `51A5881F99309AF134D5CE7F824E5D0DF57B6A25906134A110A99D93D2CA00B1`
- Atomic runner SHA256: `B8672DD29082F4EC7CD816C65A063BFC79DE24D81935DDBD901CE2CAE39651C8`
- Complete log SHA256: `06A1BC836CEAA2201FA77C258F08B9FAD1DE8F8609A9CA0C892BBFB1398F7A6C`

## Complete evidence

All six consumed roots completed atomically: `m-6134` days 1/4/8 and
`m-6213` days 1/5/8. Each Patrol used the frozen 50,000-state, 64-route sparse
enumerator. Every retained substitution passed the exact simulator and
independent validator before classification.

The complete own-road footprint equality count was zero on every root. Thus
exact-road strict gains and exact-road changed-terminal gains were also zero on
every root. The breadth gate failed `0/6` before terminal distance, Patrol fuel
or remaining-horizon reasoning could matter. There was no partial case,
invalidity, validator mismatch or incumbent mutation.

## Verdict

The unrestricted one-Patrol sparse family does not provide the exact-road
changed-terminal witness omitted by experiment 271. A remaining-horizon
successor cannot assume traffic-history equality. Reopen only through a
separately registered sound relation that explicitly carries and repairs a
non-equal traffic footprint; do not change this frozen metric or treat
componentwise lower traffic as equality.

No designed functionality was removed, disabled, deferred or reduced. Nothing
was deleted; production behavior was never changed.
