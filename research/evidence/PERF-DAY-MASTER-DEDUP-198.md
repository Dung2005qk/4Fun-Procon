# PERF-DAY-MASTER-DEDUP-198

Parent: `690728a`

Frozen manifest SHA256:
`BA23588A49AC6E431D401F63BB8538E919421844FB07484645F881780D87C34C`

## Development gate

The fresh 60-case paired development matrix completed with `22/30/8`, serving
gain/loss `+366/-60`, tail `-19/+129`, zero lifetime- or daily-distinct loss,
zero invalid or emergency result, and zero refiner failure. The result was
positive in aggregate but contained four independently timed native-role mask
mismatches and two very large positive outliers. It therefore did not by itself
authorize the holdout.

Development log SHA256:
`54642CB2E387B03067D766FA361943A4F4F7F1A30FCF7EFDA6B7C60897B779A0`

## Fixed-role A/B/B/A attribution

All eight observed development losses, both masks of the two relevant
role-selection disagreements, and the five largest material gains were replayed
with the role mask fixed. Each case ran parent/candidate/candidate/parent. Across
15 cases the stability classification was three confirmed gains, one confirmed
loss, seven mixed and four ties, with zero invalid or emergency result.

The development loss on seed `4863009` was not attributable to the flat key:
mask 8 tied in both orders, while mask 2 gave candidate gains of `+72` and
`+56`. The raw `-19` loss on seed `4863000` became candidate gains of `+1` and
`+2`. Conversely, the apparent `+129` development gain on seed `4863011` became
a reproducible candidate loss of `-12` and `-20`; this is a real bounded
post-role downside and cannot be hidden by the aggregate.

Reproduction log SHA256:
`23F78F3B19D22E1143E502C9B6D2229DC7DB4FB5710A9ECE4056737B436C6BAD`

## Holdout decision

The frozen holdout completed all 108 pairs at `24/58/26`, serving gain/loss
`+213/-345`, tail `-101/+41`, with zero lifetime- or daily-distinct loss,
invalid, emergency or refiner failure. Seven native-role pairs selected
different masks under independent timed runs and are not treated as causal
candidate regressions. The fixed-role lane alone still finished `13/33/14`,
`+137/-72`, tail `-16/+41`, so the candidate does not provide monotonic score
protection even when the role is identical.

The downside is systematic and material. Low fuel finished `7/22/11`,
`+88/-241`, tail `-101/+41`; very-hard finished `10/9/11`, `+144/-226`, tail
`-101/+41`. The same high-fuel lane that looked zero-loss in development
finished `9/19/6` and contains a fixed-role `-15` loss, so no fuel dispatcher is
authorized. Families and spot densities also contain losses on both sides of
any plausible public threshold.

Experiment 198 is rejected. The flat representation preserves exact membership
classes and complete search semantics, but in a deadline-incomplete master it
changes which useful work finishes before cutoff. Its real gains do not protect
the canonical parent and are outweighed by broader and heavier losses. BTC is
not opened because the protected holdout already fails the score gate.

Holdout log SHA256:
`32B7600DB5ACE1959665015AEA42E527819387FC284E940E8545598C14DBAEE2`
