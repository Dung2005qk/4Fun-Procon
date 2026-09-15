# 359: late-control isolation proof design

Registered 2026-09-09 before the bounded audit implementation. Canonical parent
is c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c / accepted258. The current source-only
follow-up is authorized by 358. This is a qualification-design attribution, not
a SCORE candidate, solver run, new A/A matrix, or authority to open a holdout.

Preservation answers: no designed functionality is removed, disabled, deferred
or reduced; nothing is deleted. All main/role, refinement, pair, tanker, witness,
terminal258, checkpoint, ACK/retry and post-ACK responsibilities remain intact.
Changing where an experimental treatment is observed is a measurement-control
proposal, not permission to defer any planner responsibility.

## Inputs and question

Use frozen 357 source/execution and complete 358 attribution. Do not change them.
357 binary SHA256 is
083B134AD9E0AFE847DDF8E95B7E4352F5F3E3CA5E6F281D3D83DB2B7961144D;
execution F0188694FF3E0042D1B85F8111F06E42AF7F1538A71C6AECBB44F384ED4C102C;
358 result 773E1C9B783C79764C1C7BF3767C30CCC143B61DA219DFDF29ED1FB52D504400.

358 proved no new optimizer entry at 5000, but not equal machine operations.
Initial read-only inspection found a startup treatment read and treatment-first
guards in run_http, including telemetry before submission. Determine whether the
frozen binary actually retains those differing conditional branches. Preserve the
finding regardless of direction; it does not prove these branches caused a loss.

Inspect source and disassemble existing bytes only. No build, solver execution,
clock injection, worker serialization, VM action, BTC, or additional scores.
Record tool/binary/source/report hashes and the exact relevant instructions.
An initial C:/MinGW/bin/objdump could not read ELF; C:/mingw64/bin/objdump can.
This tool-format mismatch changed no evidence and is not a product failure.

## Finite checks and decision

1. Verify frozen input identities and all source entries of the execution manifest.
2. Locate EVERY new treatment read/consumer in the frozen source. Establish the
   RuntimeOptions pointer/first-byte relation from the compiled entry and inspect
   its direct control-flow use. Do not claim a whole-program taint proof.
3. Check the treatment-independent canonical main/checkpoint and later ACK/state
   consumers; account for persistent traffic/response-ledger and idle work.
4. Exhaust the Boolean guard truth table for the actual extracted condition and a
   proposed authorization-first, late-read condition. Reject any activation change
   or treatment observation in an inactive lane in the proposed model. Include
   negative mutation controls; this model is NOT an executable-runtime proof.
5. Write exact remaining obligations for a real compiled late-control contract.
   Independent clock schedules need not produce equal timed-search trajectories;
   the target is treatment noninterference under equal exogenous inputs. Never
   conflate this conditional property with score benefit or literal A/A equality.

If the frozen machine retains treatment-dependent branches on an inactive path,
do not certify its inactive path as operation-identical. A late-read integration
may proceed only as a separately registered contract implementation with no
promotion authority. It must preserve the existing full algorithm and diagnostic
consumers. A future SCORE needs fresh manifests and a prospectively declared
qualification protocol; 357 remains inconclusive and its sealed54/protected108
stay unopened. No automatic gate waiver and no repeated broad runs for good luck.
