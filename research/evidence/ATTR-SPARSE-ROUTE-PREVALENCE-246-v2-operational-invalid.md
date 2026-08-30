# ATTR-SPARSE-ROUTE-PREVALENCE-246-v2 — invalid operational run

The corrected v2 runner completed 44 atomic results. Every emitted `result` has
exactly one matching `case_complete`; there is no `run_complete` marker and no
completed result without its atomic completion marker. All 44 completed results
reported zero invalid and zero emergency.

Before producing result 45, parent `multiteam-16` seed `9615100` failed config
validation with:

`historical tournament failed: spot count is outside the published bounds`

The frozen v2 manifest requested 18 spots on a 16x16 map. The published bound
is at most `max(width,height)`, hence 16. This is a manifest construction defect,
not a production solver, protocol, validity, deadline or resource-safety failure.
V2 and its 44 completed results are preserved as invalid operational evidence;
they must not be resumed, aggregated, or mixed with any successor.

The replacement experiment is `ATTR-SPARSE-ROUTE-PREVALENCE-247`. It uses an
entirely fresh seed pool and validates every manifest row against the declared
suite, public player range, role/fuel vocabulary and spot bound before creating
its evidence log. The sparse attribution mechanism and corrected research binary
are unchanged; production-library behavior remains untouched.
