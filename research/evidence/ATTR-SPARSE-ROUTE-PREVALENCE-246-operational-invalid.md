# ATTR-SPARSE-ROUTE-PREVALENCE-246 — invalid operational run

The first frozen attribution run stopped before producing the ninth result.
Its eight emitted `result` rows each have a matching `case_complete` marker;
there is no completed result without that marker and no partial score was
aggregated. The next case, parent `multiteam-12` seed `9601000`, failed during
configuration validation with:

`historical tournament failed: daySteps violates the published map-dependent bounds`

The isolated research harness inherited `daySteps=100` from the BTC-like
fixture generator. A 12x12 map has the published range 24 through 96, so the
fixture was invalid before either attribution arm could run. This is a new
harness-suite defect, not a production solver, protocol, validity, deadline or
score failure. The v1 binary, manifest and eight completed results are preserved
as invalid operational evidence and must not be resumed, aggregated or mixed
with the replacement run.

The replacement `ATTR-SPARSE-ROUTE-PREVALENCE-246-v2` uses fresh seeds and a
new frozen manifest. Its only harness correction clamps the generated BTC-like
`daySteps` to the published map-dependent range; production-library behavior
and the frozen sparse attribution mechanism remain unchanged.
