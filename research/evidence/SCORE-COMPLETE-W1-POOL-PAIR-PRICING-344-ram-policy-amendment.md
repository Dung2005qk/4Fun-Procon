# 344 operational amendment — explicit user direction, 2026-09-07

The user explicitly requested removal of the available-RAM stopping condition.
This supersedes only the preregistered 1024 MiB pre-side launch floor; it does
not change any score gate, sample, repeat, run order, solver resource limit,
binary, deadline, validation or partial-evidence recovery requirement.

Preservation disclosure: this removes the operational free-RAM gate by the
user's direct instruction, not a solver function. No designed solver capability
is removed, disabled, deferred or reduced. No data is deleted and no substitute
RAM threshold, override branch or alternate solver is introduced.

The original runner5608 stopped before starting its next side, solely at the
RAM check. Before changing the runner, all frozen hashes and the atomic evidence
audit passed:14 completed side results,3 complete fixture markers, no ambiguous
partial side, no remaining child, no run_complete. No partial paired score or
statistical result was inspected or aggregated. Original stderr is preserved.

Original execution SHA256:
925F79E5751CF057660D8460F782A91D82039C6B7E5E5514A98E4F5248EE507F.
Original runner/test files are preserved byte-for-byte as forensic text archives,
not executable alternate paths. The one canonical runner removes its free-RAM
query and points to a new execution provenance manifest. Its measurement,
recovery, validation and summarizer code are otherwise identical.

The amendment freeze records every existing side artifact hash and exact runner
diff before resume. All14 existing sides remain in the predeclared96-side phase;
no completed side is rerun or selected by score. The boundary and mixed execution
provenance must be disclosed in the final phase report. Sealed and protected
phases have not opened; they use this same no-free-RAM-floor runner throughout.
Final qualification remains the original frozen score/safety/causality policy.

Future research runners must not reinstate a fixed available-RAM stopping rule
without the user's authorization. Actual allocation/process/validator/deadline
errors remain errors and must be preserved and investigated, not suppressed.
