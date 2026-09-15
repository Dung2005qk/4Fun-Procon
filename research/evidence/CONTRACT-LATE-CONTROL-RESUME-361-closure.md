# 361 complete: restart and expired-window contract passes

2026-09-09. Four synthetic restart sessions completed on the existing VM using
the exact360 plain/probe binaries, each with control0 and1. No compilation,
production change, function reduction/deletion, budget change, commit or holdout.

Each session restored the same two already-ACKed days from360's mixed/native
fixture. Both historical transitions were independently revalidated before the
synthetic server accepted the restored state. Prefix replay bytes were unchanged.
No assignment repost or historical action POST occurred. Deliberately expired
day3 took the original no-POST/server-WAIT path once; terminal day4 then had one
valid POST/ACK. Both probe sessions observed zero new treatment reads, including
replay-check. All four full replay-checks pass4days/3transitions and restore
last_wire_day3. This does not test a previous strict resource takeover: the
restored360 history is a no-gain history, and different histories are not assumed
equal by the source argument.

Totals:4new ACKs,4expected expired WAITs,8restored prefix days,zero duplicate
prefix POSTs,zero assignment reposts,zero unexpected errors/stderr/control reads.
Expected deadline skips are negative-test stimuli, not permission to allow skips
in a SCORE run. No original360 match was resumed or replayed against a live server.
This is our synthetic process-RPC host on VM, not official BTC.

Complete SHA2566A9478EA267C5220A69376DC4F584AB40915E67F4ABD385F48744772398A3184.
Exact downloaded archiveC8AD766D85DCE3E182E0CD8D901DE0006FFE6104D1555C70BD9C96F725BB0CCE.
Frozen runnerB2E915B6817E18707DB47704DA108A2CA27CC6BAB26E81482DD79323EAA854CF.
VM read-only check and independent local verify_resume_control_361_copy.py pass.
An initial local .cmd pipe quoting error affected only a process-list command;
the no-pipe process listing confirmed no competing solver. No contract was rerun.

## Qualification decision

360/361 close the registered finite boundary tests, not a universal timing proof.
Source plus compiled normal-flow inspection now support treatment noninterference
in an entirely inactive match under equal starting persistent state and equal
exogenous clock/I/O/allocation/scheduling inputs. Resume preserves that boundary
because the flag is neither serialized nor loaded as state. An earlier enabled
day may legitimately change future state; mixed-window cross-history equality is
not claimed. Later short days must still preserve their own complete checkpoint.

Plain execution removes instrumentation without a hidden alternate planner; the
same host object retains the same guarded read site. Hard-enabling/recompiling
production is a separate uncompleted integration gate, not covered by this result.

357 remains inconclusive. Its exact independently-timed pairing gate is not
retroactively replaced, and its54sealed/108protected fixtures stay unopened.
Next permitted work: a separately preregistered prospective SCORE qualification
for the frozen late-control mechanism, defining causal operation equivalence and
retaining ALL independent-run differences as measured downside. No inactive gain
may count as causal benefit. If that protocol cannot meet AGENTS' equivalence and
generality requirements without discarding adverse evidence, it must not launch.
Current canonical c76a8ea/accepted258 remains unchanged; no signature yet.
