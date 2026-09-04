# ATTR-TARGET-FOLLOWUP-PREFIX-FEASIBILITY-303

Date: 2026-09-04
Authority: read-only architecture attribution; no score or promotion authority
Frozen manifest SHA256: `8E5E6CA627B931A88A5371249979A6C230C4B3F636600706A51285799E98FE2B`

## Question

Can accepted capability 300 run strictly after an order-identical canonical
target-followup prefix, using only time left by the parent, so that its exact
reverse terminal bound cannot cause the 5000-ms mismatches or mixed losses seen
in rejected experiment 301?

## Runtime trace

1. `SlackRefiner::refine_midday_chain` first builds the ordinary global route
   pools for all Patrol tasks. Four workers claim tasks through one atomic index;
   every route enumeration receives the same wall-clock `searchDeadline`.
2. `run_one_agent_midday_ascent` consumes those pools and repeatedly commits the
   best strict protected improvement. It returns only at a fixed point or a
   wall-clock deadline.
3. Target follow-up begins only if the global ascent did not report a deadline.
   This is the already-accepted order-identical boundary of experiment 215, but
   it is before—not after—the canonical target-followup work whose output 300
   would need to preserve.
4. The target phase allocates a new reachability vector. The same worker pool
   claims Patrol tasks through a second atomic index. Each required-terminal
   enumeration again receives the one shared wall-clock `searchDeadline`; there
   is no per-task operation budget, deterministic task completion order or
   committed complete-frontier marker.
5. All workers join, then the entire ordinary reachability vector is replaced
   by the target vector. The replacement may contain complete, deadline-partial
   or never-started task results. The target ascent consumes that vector before
   `completedTargetTasks` is checked. Therefore the completion counter is only
   retrospective telemetry; it is not a safe checkpoint.
6. The target ascent may commit several strict current-day improvements. A
   different partial target pool can change the greedy chain and its terminal
   state even though every individual replacement passes the local protected
   certificate. Experiment 302 observed all three consequences: candidate
   starvation, extra locally greedy acceptance and upstream timed role
   divergence.
7. Only after target ascent returns does the code mark a deadline when fewer
   than all target tasks completed. At that point the partial pool may already
   have changed the committed plan. If the target ascent itself consumed the
   deadline, no authoritative time remains for a sidecar.

## Candidate phase boundaries

| Boundary | Parent preserved? | Useful headroom? | Verdict |
| --- | --- | --- | --- |
| After ordinary global ascent | Yes for pre-215 work, no for target follow-up | Sometimes | Forbidden: a sidecar here would replace or compete with canonical target work. |
| After target workers join with incomplete tasks | No | At most validation reserve | Forbidden: task/frontier identity is deadline- and scheduling-dependent; no committed parent target prefix exists. |
| After all target frontiers complete | Yes | Possible on easy cases | Inert: capability 300 is exact-output-equivalent to the already complete parent frontiers; rerunning only duplicates work. |
| After target ascent reaches fixed point | Yes | Possible on easy cases | Inert for the same complete candidate set; no additional certified plan exists merely from recomputing it faster. |
| After target ascent reaches deadline | Parent plan is committed | No | Impossible: no authoritative compute window remains. |

## Verdict

No useful prefix-preserving phase boundary exists in the accepted runtime. The
only boundary with positive time is before canonical target work and would
steal or replace designed functionality. Boundaries after incomplete work are
not order-identical; boundaries after complete work are output-equivalent and
cannot add candidates; the deadline boundary has no remaining time.

Consequently capability 300 remains a valid exact algorithmic result but cannot
be integrated as a post-parent sidecar in the current architecture. Direct
in-place integration and prefix-sidecar integration are both closed. Reopening
requires a genuinely different invariant, such as a deterministic operation-
bounded target contract whose whole policy is evaluated as a new logic
candidate on fresh evidence, or a state-coupled remaining-horizon certificate
that proves a replacement dominates the incumbent. It cannot be reopened by
another wall-clock threshold, map/fuel dispatcher, task-order tweak, cap increase
or reuse of experiments 301/302.

Functionality-preservation answers: (1) this audit removes, disables, defers or
reduces no designed functionality; (2) nothing is deleted.
