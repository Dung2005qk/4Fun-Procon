# UDON-SHIELD Research State

Updated: 2026-08-10

## Current phase

Forward research starts from the current global champion `afcd2da`. Re-running or
rebuilding every historical checkpoint is not a research objective; old
checkpoints remain lane champions for targeted A/B only. `PERF-DEADLINE-003` is
accepted as a semantics-preserving BTC transport repair. `SCORE-ROLE-030`, 031
and `SCORE-FINAL-001` are rejected and no opened holdout may be retuned. Source
and tests are restored to the `afcd2da` planner/decision line. `SCORE-ROLE-032`
is also rejected: deterministic work stabilized an evaluator that still selected
the losing role. `PERF-SYNC-033` and `SCORE-EXACT-034` are rejected as well and
fully absent from source. No implementation axis is currently open.
`ATTR-MASTER-035` and `ATTR-ROUTE-036` have closed the exact claim-mask and
frontier-membership ambiguities. `ATTR-POOL-037` and `ATTR-ALNS-038` narrowed the
remaining source to regular one-agent ALNS but cannot distinguish portfolio from
synthesized route. `SCORE-ALNS-039` is rejected and absent from source. No
implementation candidate is active. `ATTR-DENIAL-040` falsified exchange over
the retained production frontiers. `ATTR-OPTIMAL-041` proves complete
one-patrol infeasibility and `ATTR-TEAM-042` proves a dual-valid multi-patrol
score of `6/60/322`. `SCORE-QUEUE-043` is rejected and absent from source. No
production implementation candidate is currently active. `ATTR-QUEUE-044`
localized the observed -72 tail to a trajectory that changes action bytes on
day 1 and loses score from day 2, before the intended terminal anytime queue is
directly enabled. `ATTR-QUEUE-045` exposed a protected-harness wiring flaw:
`exact_supported=0` because the harness used default harvest mode 6 while BTC
production uses mode 7. `ATTR-QUEUE-046` then closed the causal comparison:
cardinality-first improved exact-local evidence but tied the final official
score on the alleged negative state. No production candidate is active; the
temporary switch/probe must be reverted before a new mode-7 holdout is frozen.
The temporary attribution source/probe is fully reverted. `EVAL-PARITY-047` is
accepted: the forward harness now declares harvest/future mode 7 and exposes
terminal exact wiring telemetry; historical reports remain untouched mode-6
artifacts. `SCORE-QUEUE-048` supplied the frozen score mechanism for accepted
final candidate `FINAL-QUEUE-065`. The final source combines only its
canonical cardinality-first capped queue with exact HTTP deadline alignment,
identical chunked exact-state initialization and recursive feasibility
cancellation. Rejected HTTP retry/header experiments and attribution-only
decision telemetry are absent. `FINAL-QUEUE-065` is closed accepted and no
implementation or score axis remains active.

## Authoritative budget evidence

- The rules allow response time to vary per match.
- Standard team-vs-team match `m-1042` displayed `60000 ms` per day. This is the
  outer server window, not permission to increase the internal compute budget.
- BTC preflight `m-1181` used `5000 ms`; current HEAD passed connection, assignment,
  valid-plan and deadline checks, with slowest response `279 ms` and final rank `#1`.
- User operating decision on 2026-07-31: `5000 ms` remains the primary hard cap for
  solver and role selection. No candidate may be promoted using work that exceeds it.
- Existing `500/1200/2500 ms` local lanes are diagnostic degradation lanes. They are
  not production-selection gates unless BTC announces a matching deadline.

## Frozen checkpoints

- `6132f41` — baseline-btc
- `02df79d` — exact-highfuel general-score plateau
- `6f84a06` — current correctness/proof checkpoint

The complete immutable mapping is `old/CHECKPOINTS.csv`.

## Active gap

Promotion semantics were corrected by the user on 2026-08-01. A comprehensive
win means a sufficiently large, broadly distributed paired advantage with bounded
downside; it does not require zero losses on every map and it is not a fixed W/L
ratio. Exact dominance remains preferred. Invalid/emergency output and exceeding
the 5000 ms internal cap remain hard blocks. Every match is still compared by the
official lexicographic vector; the global verdict reports W/T/L, first differing
tier, gain/loss magnitude and downside distribution without a weighted sum.

The current score champion is `08771f1` (`SCORE-ROLE-023`). SCORE-ROLE-024 through
029 are closed or inconclusive and absent from source; their opened evidence may
not be retuned. `PERF-DEADLINE-003` fixes the independent ACK-loss counterexample
from `m-1278` without changing role selection, planner, comparator or action JSON.
Its normal and forced-resend BTC gates both completed 10/10 valid days and ranked
first. No further ACK-slice tuning is permitted against these opened fixtures.

Fresh discovery matches from transport champion `afcd2da` used explicit advanced
configuration and the 5000 ms hard cap. Low-fuel `m-1285` selected two-tanker
mask96, completed 10/10 valid days and ranked first at `6/60/320`; exact
counterfactual mask160 reached `6/60/319` while one-tanker mask64 lost tier 2 at
`6/57/238`, so that map does not open a score mechanism. Replay SHA256 is
`D638BE3D0E136EF94542BBCE58AA6B086D6A7C872732EF27188392D5D19D2F77`.

High-fuel `m-1286` selected one-tanker mask4, completed 10/10 valid days and
ranked first at `6/60/412`; maximum server-reported response was `2725 ms` and
maximum solver time was `2619 ms`. Replay SHA256 is
`8C3F6D8A975EA4263C921438517307478B66F8174700F1A3970666B512C45122`.
Same-binary exact counterfactuals reproduced mask4 at `6/60/412` and mask1 at
`6/60/422` in both run orders. The incomplete role rollout instead ranked them
`6/60/231` and `6/60/230`. Day 1 ties at 41 servings; the exact gain develops on
days 2, 4, 5, 7 and 10, so a central-tanker heuristic or another day-1 probe is
falsified.

`SCORE-ROLE-030`, parent `afcd2da`, targets only this measured resolution gap.
For horizons above five days with fuel strictly above twice the maximum daily
steps and incomplete full-horizon role evidence, it will take the top two
assignments having the leader's patrol count, split a bounded refinement window
equally, rerun the canonical exact rollout with the existing 8000-combination
portfolio and retain each mask's lexicographic maximum old/new evidence. It does
not use centrality, facility score, seed, map family or a tier trade. Short,
low/default/medium fuel paths remain unchanged, returned width is unchanged and
the role wall is at most 96% of 5000 ms. Frozen holdout SHA256 is
`4F399B26E148A0B58DAEB44551C12BF7502711D987B24C7DBFE73126EA2D859C`.

The candidate failed its first causal gate and is absent from source. The richer
paired refinement consumed the available role window through `4794 ms` but left
mask4 at rollout `6/60/231` and mask1 at `6/60/230`; it therefore did not expose
the stable exact +10. The frozen holdout remained unopened and no BTC candidate
was run. Wider or longer variants of the same coarse rollout are now closed.
Reopen this gap only after trajectory-consistent attribution explains why the
roles tie at 41 on day 1 but mask1 gains on days 2, 4, 5, 7 and 10. Source and
tests are byte-identical to `afcd2da`.

Further exact attribution rejects two easy proxies. In an all-patrol production
solve, agent0 served 6 on day 1 and 18 over days 1--3 while agent2 served only 5
and 16; a marginal-patrol rule would therefore choose agent2 as tanker, exactly
the losing mask4. Tanker centrality also chooses mask4 and was already falsified
on `m-0929`. The causal difference is the multi-day joint trajectory: mask1 moves
its tanker through hubs 790 then 593 and keeps the team at 43--44 servings on
several later days, while mask4 begins at hub712 and loses coordinated servings
on days 4--5 despite identical day-1 score. A static placement proxy is not an
admissible successor.

`SCORE-ROLE-031`, parent `afcd2da`, changes allocation rather than scoring. For
only long horizons whose fuel is strictly above twice the maximum daily steps,
the independent full-horizon rollouts for the existing beam run concurrently in
isolated engines after the common one-day probe. Every assignment uses the same
canonical 256-combination/day exact rollout and the same absolute 85% deadline;
there is no shared mutable router/master cache and merge order remains the
official comparator. This exploits the mathematical independence of role
subproblems to replace eight serial fragments with equal wall-clock evidence,
without a proxy or more than 5000 ms wall time. Low/default/medium and short paths
are unchanged. Frozen holdout SHA256 is
`BAEC90234791109009D56D10402C5DD720B2399DB0A1A67DFF38036A14144EA2`.

The parallel candidate is rejected and absent from source. Isolated tasks did
remove serial wall allocation, but their ranked evidence became a function of CPU
competition: on the contended development host mask4 remained first at rollout
`6/60/228` while exact-better mask1 fell to `6/55/218`. The observed `2947 ms`
elapsed value has no performance authority; the rejection is that the causal rank
remained wrong and scheduling/load now changed evidence quality, violating the
requirement that production quality not depend on the local host. The frozen
holdout remained unopened and no live candidate was run. Parallel role evaluation
may reopen only with fixed operation counts per assignment and deterministic
evidence independent of CPU scheduling. Source is byte-identical to `afcd2da`.

Planner telemetry opens a separate, non-role gap on terminal day. In `m-1285`
day 10, the selected exact score was `6/60/320`, guidance/optimistic upper score
was `6/60/322`, the master hit its deadline, and certification consumed zero
measured milliseconds. In the negative control `m-1286` day 10, selected and
guidance scores were both `6/60/412`. Code tracing shows why: every master
candidate has already passed the exact simulator and independent validator;
there are no future days, so every traffic-scenario witness is exactly
`scoreAfterToday`, and the final selected plan is independently validated again.
Nevertheless the BTC schedule reserves 20 percent for future-witness
certification and the F0 phase reserves another 25 percent of its window for
future profiles.

`SCORE-FINAL-001`, parent `afcd2da`, may act only when
`state.dayNumber == config.day_count()`. It certifies each provisional scenario
profile directly from the already independently validated current-day candidate,
collapses the certification bucket to the existing 25 ms validation floor,
transfers only the released duration to search, and sets the F0 future-profile
reserve to zero. It retains the existing 150 ms F0 boundary guard, 1600 ms BTC
network reserve, official lexicographic comparator, exact final validation and
total 5000 ms cap. Non-terminal deadlines, profiles, actions and search must be
byte-equivalent. Frozen holdout SHA256 is
`CDBC63E7AF1FFA22E0B6C9CD5342CA3C0BC8A2E269B668C18567DACDC0116E66`.

`SCORE-FINAL-001` passed its direct development gate: the unit suite passed,
`m-1285` day 10 improved from `6/60/320` to `6/60/321`, and the `m-1286`
negative control tied `6/60/412`, all with zero invalid/emergency output. The
frozen general-fixed lane then tied `0/12/0`. In the native lane, seed `923007`
changed from parent mask `8` and `6/24/38` to candidate mask `4` and `6/24/32`.
An exact parent rebuilt from `08771f1` reproduced mask `8`; planner/decision are
byte-identical between `08771f1` and transport parent `afcd2da`. The role rollout
attribution is causal within the candidate binary: fixed mask `8` reproduces
`6/24/38`, while fixed mask `4` reproduces `6/24/32`. The role rollout
does not execute the terminal mechanism, but its independent planner is bounded
by 60 ms wall time even in `select_roles`, so a changed binary layout/local
schedule can change rollout evidence and the selected mask. Local duration is
not a performance verdict; the decisive failure is that the claimed unchanged
role surface was not invariant and its observed downside of six servings was
larger than the one-serving causal gain. The remaining holdout and BTC gate were
not run. Candidate source/tests were fully reverted. Reopen only after role
evidence is operation-bounded and independent of code layout and host load.

`SCORE-ROLE-032`, parent `afcd2da`, is preregistered before source change at
holdout SHA256
`FAB2F1E71CC90185AB56D11ADF7FBD604F1FD1331AD77C528C8F3C9E8BDDA44E`.
It retains EventConflict, MacroMCTS, the route master, exact simulator and
independent validator. Each role assignment/day receives the same fixed work
allowance derived only from the existing `maximumCombinationsPerDay`; wall-clock
is an outer hard abort, not a ranking budget, and partial unequal-depth evidence
cannot outrank completed equal-depth evidence. There is one shared implementation
for low/default/high fuel and short/long horizons; no seed, map, match or bot
dispatcher is allowed. Development must first recover mask `8` on seed `923007`
and preserve the `m-1285`/`m-1286` role controls. Local timing has no promotion
authority; BTC target-host telemetry remains the final performance gate.

The unit suite passed, but the first causal development gate rejected
`SCORE-ROLE-032`. In the paired run both parent and candidate selected mask `4`
and scored `6/24/32`; fixed mask attribution already proves mask `8` reaches
`6/24/38`. Thus equal operation counts remove one noise source but merely make
the wrong coarse evaluator reproducible. The holdout stayed unopened and no BTC
candidate ran. Work-limit, equal-depth and test changes were fully reverted.
This also confirms that retrying wider/longer/fixed-count versions of the same
role rollout would be circular; reopen only for a newly attributed capability
mismatch not already falsified by `SCORE-ROLE-008`, 012, 030, 031 and 032.

`PERF-SYNC-033`, parent `afcd2da`, is preregistered before source change at
holdout SHA256
`C40DD7A1FDA69FEA54ABE1ED3B374506C56B82628386855465F30664C9B74CA1`.
The causal telemetry is not the leaderboard rank: low-fuel `m-1285` leaves a
2--5-serving master guidance gap on every day and performs 80 to 116625 partial
synchronization checks per day while deadline-bound; high-fuel `m-1286` is the
closed-bound control. The current DFS predicate reconstructs assigned-agent,
escort and refuel coverage state by scanning the entire partial selection on
every node. The candidate may replace only that DFS rescan with reversible
push/pop state implementing the identical predicate. Portfolio construction,
branch order, beam search, complete synchronization validation, simulator,
independent validator, official comparator and the 5000 ms internal work cap
remain unchanged. Local runs may establish equivalence and falsify the mechanism,
but only fresh BTC target-host telemetry may establish performance or promotion.

The candidate passed the unit suite and matched all 27 day plan hashes, official
scores and combination counts on fresh fixed-role seeds `936000..936005`. Its
final causal gate nevertheless failed: on `m-1285` day 10 the exact frozen parent
reached `6/60/321` with 5119 combinations and 195961 partial checks, while the
candidate reached `6/60/320` with 1129 combinations and 9685 checks. Both were
exact-valid, deadline-bound and retained guidance `6/60/322`. Local elapsed time
has no performance authority; the official-score regression and failure to create
useful search headroom reject this implementation before BTC or the remaining
holdout. Source and tests are restored byte-identical to `afcd2da`. Do not retune
incremental containers against this replay; reopen synchronization performance
only from a different target-host-profiled bottleneck.

Exact-route attribution opens `SCORE-EXACT-034`, parent `afcd2da`, at frozen
holdout SHA256
`6A9D06F06DBB49FDE8B3EAC1CA983C3CD8D2C33683CD012DADFEB775A7D2655B`.
On `m-1285` day 10 the accepted exact bundle serves 34; coordinated exact search
visits 504176 feasibility nodes, including 500129 overlap nodes, and records zero
improvements. The canonical master/ALNS can reach 35 while the root guidance is
36. Source tracing shows the low-fuel anytime enumerator retains only its 32
`maximalRoutes` once lifetime coverage is complete: `preferredBrands == 0`
disables the separately capped `supplementalRoutes` frontier, although daily
distinct and servings are still live official tiers. The candidate changes only
the existing anytime call: when no lifetime brand is missing, all public config
brands become the preferred set, activating the existing supplemental frontier
and enhanced coordination pass. Incomplete-lifetime behavior, high-fuel exact
enumeration, route semantics, settled-state and per-frontier caps, simulator,
validator and the 5000 ms cap remain unchanged. Development must first improve
the exact bundle or final selected score on `m-1285` day 10 and preserve the
closed high-fuel `m-1286` control before any holdout or BTC run.

The first causal gate rejected `SCORE-EXACT-034`. The unit suite passed, and the
candidate did activate the intended path: exact bundles increased from one to
two and each patrol portfolio gained one exact column. Nevertheless the exact
seed/local bundle remained at 34 servings and the final selected score remained
`6/60/320`, with guidance `6/60/322`, zero invalid/emergency and zero exact
deadline overrun. The high-fuel control, remaining holdout and BTC were not run.
Source/tests are restored byte-identical to `afcd2da`. Supplemental rank/cap may
not be tuned against `m-1285`; the next admissible step is to extract the exact
per-agent/per-spot claim masks from the already observed 35-serving canonical
witness and compare them to the 34-serving exact bundle.

`ATTR-MASTER-035` freezes the authoritative replay at SHA256
`D638BE3D0E136EF94542BBCE58AA6B086D6A7C872732EF27188392D5D19D2F77`
and the exact-valid parent `6/60/321` day-10 plan at SHA256
`787F2C1774D11D08125115FD3F75488A222572CB0CB79692F14A877F5EB81DA4`.
A research-only probe may reconstruct the accepted ledger and day-10 state,
parse that frozen plan and print exact served/denied claims by agent/spot. It may
not call search, alter planner/decision source, infer performance from local time
or tune a successor directly against action bytes.

The attribution completed with exact simulator/validator agreement. Both plans
start day 10 from ledger `6/54/286`; the exact bundle ends at `6/60/320` and the
canonical witness at `6/60/321`. Agents 1 through 7 are byte-identical. Agent 0
alone replaces mask `0x999 = {0,3,4,7,8,11}` with
`0x9D8 = {3,4,6,7,8,11}`. Both routes claim and serve six spots. The exchange
removes one excess claim from spot 0 (stock two, four claims in the exact bundle)
and adds one claim to spot 6 (stock six, four claims), removing exactly one
denial. Evidence is `research/evidence/ATTR-MASTER-035.md`.

`ATTR-ROUTE-036` may now reconstruct the same frozen day-10 state and call only
the canonical agent-0 anytime resource enumerator with the production semantic
parameters: minimum five spots, 32 retained routes, 1250000 settled states and
zero preferred-brand mask. It uses no wall deadline because local elapsed time
has no authority; the fixed work cap is the deterministic boundary. It may
report membership and ranks for `0x999` and `0x9D8` but may not change a rank,
cap, planner or coordinator. This membership result must close before any new
implementation candidate or holdout is opened.

The membership result is decisive. The canonical no-deadline fixed-work probe
is supported but incomplete at exactly 1250000 settled states, with 32 maximal
routes and no supplemental/terminal routes. `0x999` is present at index 1;
`0x9D8` is absent. Because eight retained masks have only five spots, a discovered
six-spot `0x9D8` could not have been evicted by the cardinality-first rank. Its
frozen action path consumes 96 movement steps plus wait four and raw fuel 73
from initial fuel 100, so it is not refuel-dependent. The omission is therefore
cap-limited queue exploration before discovery. Evidence is
`research/evidence/ATTR-ROUTE-036.md`.

`ATTR-POOL-037` must now trace where the unchanged canonical pipeline obtained
the exact-valid `0x9D8` route: initial portfolio, independent candidate, ALNS
candidate-route augmentation or recombination. It permits current source
tracing and one frozen-parent replay-solve attribution run, but no repeated
sampling, local performance inference, source change or enumerator-cap variant.
Any successor must reuse a public, exact-valid route surface and must not route
on `m-1285`, agent 0 or mask bytes.

The one permitted frozen-parent provenance run reproduced the exact 321 witness.
Exact seed/local stayed at 34, exact feasibility made zero improvements over
504176 nodes, final score reached 35, ALNS recorded one improvement and late
recombination recorded zero. This narrows the gain to the one-agent ALNS interval,
but `btc_main` does not print pre-ALNS best, per-operator accepted counts,
synthesized counters or proof-guided counters. `ATTR-POOL-037` is therefore
inconclusive only at the portfolio-column versus synthesized-repair substage;
evidence is `research/evidence/ATTR-POOL-037.md`.

`ATTR-ALNS-038` may extend only the untracked research probe and link it against
the unchanged frozen parent library. It must perform one solve at the canonical
5000 ms cap, require byte equality with the frozen 321 plan, and print the
already existing `AlnsDiagnostics` fields. Local elapsed time is ignored. If the
plan differs or the counters remain ambiguous, the attribution closes
inconclusive and no implementation assumption is allowed.

The wrapper reproduced the exact witness bytes and score `6/60/321`. Regular
ALNS ran 35 iterations, accepted 25 candidates and recorded one improvement;
it generated 66 synthesized routes and accepted 11. `StockMultiVisit` (operator
index 4) was attempted and accepted five times. Proof-guided iterations and
recombination improvements were both zero. These aggregate counters do not
identify whether the winning route was an existing column or synthesized, so
`ATTR-ALNS-038` closes inconclusive on that narrow provenance question. They do
prove a separate schedule gap: guidance remained `6/60/322` while the existing
proof-guided phase, placed after generic ALNS, received no iteration. Evidence
is `research/evidence/ATTR-ALNS-038.md`.

`SCORE-ALNS-039` may open only after hashing
`research/holdouts/SCORE-ALNS-039.csv`. Its sole mechanism is schedule priority:
when the valid official upper bound is lexicographically above the current best,
execute at most one existing proof-guided patrol sweep before generic ALNS, then
continue the unchanged generic loop under the same absolute deadline and
iteration caps. When the score is already at the valid bound, behavior must be
unchanged. No new operator, route rank, map/fuel dispatcher, cap increase or
fixture-derived mask is allowed.

The holdout is now frozen at SHA256
`83CB73EF1A1EB78C841E2AFB966E12FEF511A1F85EB3680EBD14E71428F43E18`.
The first causal gate is only `m-1285` day 10, followed immediately by the
closed-bound `m-1286` day-10 control. The fresh generated and protected lanes
remain unopened until both gates pass. No BTC candidate may run before the
paired local semantic/score gates establish a promotable opportunity.

The first causal gate rejected `SCORE-ALNS-039`. Unit tests passed and the proof
prefix executed as intended: six proof iterations produced 36 routes and
accepted six exact candidates, but improved none. Generic ALNS then fell from
the frozen parent's 35 iterations with one improvement to 31 iterations with
zero improvements; selected score regressed from `6/60/321` to `6/60/320`.
Exact seed/local remained 34 and recombination remained unchanged. The high-fuel
control, all frozen holdout lanes and BTC stayed unopened. Source is restored
byte-identical to `afcd2da`; evidence is
`research/evidence/SCORE-ALNS-039.md`. Do not tune prefix length or placement on
this replay. Proof scheduling may reopen only from a new diverse counterexample
where a specific existing proof repair is already shown to improve before
generic ALNS.

`ATTR-DENIAL-040` freezes the 35-serving witness and enumerates the unchanged
canonical 32-route anytime frontier for every patrol at the fixed 1250000-state
cap, without a wall deadline. Each route may replace only the corresponding
agent path in the frozen plan and must pass both exact engines. The result is an
official-score feasibility oracle, not a performance benchmark. It may not
change caps/ranks or synthesize a target route. No new implementation candidate
may open until this probe proves or falsifies a 36-serving one-exchange.

The canonical-frontier oracle evaluated 192 exact-valid, independently agreed
mutations: all 32 routes for each patrol `0,1,2,3,4,7`. Every per-agent and
global best remained `6/60/321`. Thus no retained exact route can close 35 to
the guidance value 36, and an exchange layer over the current 32-route frontiers
is closed. Evidence is `research/evidence/ATTR-DENIAL-040.md`.

`ATTR-OPTIMAL-041` may run `enumerate_exact_resource_routes` to completion only
for the two unique start/fuel states among agents 2, 4 and 7, the agents claiming
oversubscribed spot 0, with no wall deadline. The initial method call through the
anytime wrapper was not interpreted because that wrapper deliberately forces
`complete=false`; it settled 3148570 states but produced no proof verdict. The
proof API must return `complete=true`, and every inclusion-maximal route must
pass dual exact validation after substitution. Inclusion-maximal masks suffice
for servings because adding a first-visit claim cannot reduce the team total
`min(claimCount, stock)` at any spot. This remains a local mathematical oracle;
elapsed time and work are forbidden as production performance or cap evidence.

The corrected proof run returned `supported=1, complete=1`, 3148570 settled
states and 78 inclusion-maximal routes for each of the two unique claimant
states. All 156 substitutions passed both engines; every best remained
`6/60/321`. The 35-to-36 gap is therefore impossible through any one-patrol
resource-feasible exchange. Evidence is
`research/evidence/ATTR-OPTIMAL-041.md`.

`ATTR-TEAM-042` may enumerate complete frontiers for all unique patrol
start/fuel states and run a deterministic DP keyed only by per-spot claim counts
capped at public stock. It must retain an exact route-choice witness, reconstruct
the full plan with unchanged tanker paths and pass both exact engines. The DP
uses official daily brands then servings; no weighted sum or local timing claim
is allowed. No production candidate may open before this multi-agent feasibility
question closes.

The complete team DP found the guidance score after 38 nodes and seven memo
states. All six patrol frontiers were proof-complete; the reconstructed masks
were agent 3 `0xB59`, agents 0/1/2/4 `0x9D9`, and agent 7 `0x1EC`. The exact
simulator and independent validator agreed on `6/60/322`. This proves a genuine
multi-agent capability gap rather than a loose upper bound. Evidence is
`research/evidence/ATTR-TEAM-042.md`.

`SCORE-QUEUE-043` may open only after hashing its frozen holdout. Its sole
mechanism is to align the capped resource-search priority with the existing
retention objective: greater visited-spot cardinality first, then lower used
steps/fuel and stable label order. The state graph, Pareto dominance, route
reconstruction, 32-route retention, 1250000-state cap, coordinator, exact
engines and high-fuel path remain unchanged. No replay mask, agent identity,
map/fuel dispatcher or cap increase is allowed.

The frozen holdout SHA256 is
`BFFB6312262CC28A21D8131968BF5A603CB37B12EB087472F1D0E2CBDE8AE48B`.
The first gate is the attributed `m-1285` day-10 route/frontier score, followed
by the separate high-fuel `m-1286` closed-bound control. All fresh and protected
lanes remain unopened until those gates pass; BTC is prohibited until paired
local semantic/score evidence is promotable.

`SCORE-QUEUE-043` proved the deep-route mechanism but failed global downside.
On `m-1285` day 10 it changed the representative capped frontier from 24
six-spot plus eight five-spot routes to 24 seven-spot plus eight six-spot routes;
exact seed/local rose from 34 to 37 and final score rose from `6/60/321` to
`6/60/323`. High-fuel `m-1286` tied `6/60/412`, and the fresh general-fixed lane
was `0/18/0`. On the first four BTC-like low-fuel cases, however, seed 957002
threshold-corridor regressed from 346 to 274 while the other three tied. A
tier-3 tail loss of 72 is not bounded relative to a gain of two, so the candidate
is rejected without opening remaining lanes or BTC. Source is restored
byte-identical to `afcd2da`; evidence is
`research/evidence/SCORE-QUEUE-043.md`. Do not tune a mixed priority or weight
against the opened positive and negative fixtures.

`ATTR-QUEUE-044` may run one parent/candidate paired fixture for frozen
BTC-like low-fuel seed 957002 with day details. It may report cumulative/day
official scores, plan hashes and the first differing day only. Frozen binaries
must be used; no queue variation, source change or local timing conclusion is
allowed. Any successor must preserve parent route evidence by construction and
use a new unopened holdout.

The single paired run reproduced parent `6/60/346` and candidate `6/60/274`
with zero invalid/emergency. The plan hash already differs on day 1 while both
score `6/36`; the first official-score difference is day 2 at cumulative
`6/12/74` versus `6/12/72`. Day 3 adds a further 13-serving loss and the
candidate remains lower every later day. This is a full-trajectory divergence,
not a terminal-day localization. Source tracing confirms the production
current-day anytime low-fuel queue is guarded to the terminal day, so the -72
cross-binary tail cannot be assigned directly to the intended queue mechanism.
Evidence is `research/evidence/ATTR-QUEUE-044.md`.

`ATTR-QUEUE-045` may add a temporary research-only queue-policy switch and one
attribution harness. In one binary, generate a shallow-policy prefix through day
9, retain the exact submitted decisions, then instantiate fresh engines and
replay those identical decisions into both so belief/response-ledger history is
equal and route caches are equally cold. On the identical day-10 state, ledger
and traffic, compare shallow versus cardinality-first capped anytime queues in
both execution orders. Only official score, exact plan hash and existing
orienteering diagnostics may decide causality; local elapsed values are ignored.
The 1,250,000-state/32-route caps, exact full-proof path and both validators must
remain unchanged. The research switch must be fully reverted when the probe
closes; no holdout or BTC is authorized.

The same-binary result used state hash `2130807422699737667` and prefix ledger
`6/54/244`. Forward shallow, forward cardinality, reverse cardinality and reverse
shallow all returned exact day score `6/30`, cumulative `6/60/274` and plan hash
`14218929452539799733`, with zero invalid/emergency. This equality is not queue
equivalence: every run reported `exact_supported=0`, `exact_settled=0` and zero
exact bundles. The historical harness constructs `UdonShieldEngine(config)`,
whose default harvest mode is 6; BTC runtime explicitly defaults to mode 7. The
terminal fuel-constrained anytime path requires mode greater than 6. Therefore
the `SCORE-QUEUE-043` generated protected lanes never executed the candidate
mechanism, and their -72 cross-binary tail is not causal downside. Evidence is
`research/evidence/ATTR-QUEUE-045.md`.

`ATTR-QUEUE-046` may change only the two research-harness engine constructors to
explicit harvest mode 7 and future mode 7. It then repeats the already frozen
prefix/fresh-engine terminal fork once. `exact_supported` and settled states must
be nonzero before any policy comparison is interpreted. This remains opened-seed
attribution only; it cannot promote the prior candidate or authorize BTC. Any
successor logic requires a new production-parity holdout because every
`SCORE-QUEUE-043` generated seed has already been opened.

The explicit mode-7 probe used state hash `7024081118606904448` and prefix
ledger `6/54/312`. Both shallow runs selected exact `6/39`, cumulative
`6/60/351` and plan hash `3450257480091618005`; both cardinality runs selected
the same official score with plan hash `108925725917910114`. All four were
dual-valid with zero invalid/emergency and `exact_supported=7`. Cardinality
raised exact local servings from 38 to 39, but shallow's full planner already
found 39 through another path, so the causal official delta on this state is
zero. This disproves the prior -72 attribution without promoting the mechanism.
Evidence is `research/evidence/ATTR-QUEUE-046.md`.

The next admissible implementation experiment must begin from restored
`afcd2da`, correct the research evaluator to explicit production mode 7, freeze
a completely new diverse holdout before queue source changes, and compare
same-terminal-state paired official outcomes. The old mode-6 matrix has no
promotion authority for this axis, and no opened SCORE-QUEUE-043 fixture may
serve as tuning evidence.

`EVAL-PARITY-047` may change only `old/harness/historical_tournament.cpp` and the
stale budget wording in `old/README.md`. The harness must instantiate production
harvest mode 7 and future mode 7 explicitly and print those values together
with exact-orienteering supported/settled/seed/local counters. One already-opened
seed 957002 may be used only to prove the runtime path is entered. Existing
historical score reports remain mode-6 artifacts and must not be relabeled or
rerun as a substitute for forward research. No solver source, candidate score,
holdout or BTC action is permitted in this experiment.

The wiring gate passed. The harness emitted harvest/future mode `7/7`; days 1--9
reported zero exact-supported agents, matching the terminal-only guard, while
day 10 reported seven supported agents, 711197 settled states and exact
seed/local servings `31/32`. Both exact engines agreed and invalid/emergency
were zero. Local score and elapsed values have no performance authority.

`SCORE-QUEUE-048` is frozen at holdout SHA256
`13EC4F536FA573AA3158D35D93CDC5A5D6F0DEBF6572E1D2BDEADDA471122231`.
Its only logic change is cardinality-first pending-label order inside the capped
anytime resource path: greater visited-spot count, then lower steps/fuel and
stable label ID. The exact full-proof path, state graph, dominance, 1,250,000
settled-state cap, 32-route retention, coordinator, current terminal/fuel guard,
simulator and validator remain unchanged. A temporary same-binary policy switch
is allowed only for causal evaluation and must be removed from a final candidate.
Development first compares archived `m-1285` day 10 and the already-opened neutral
957002 control. The frozen screen then covers six general families, deadline and
fixed roles, one/two-tanker low fuel and default/high controls on fresh seeds.
No BTC run is allowed before both screen and full paired gates pass.

The development gate passed causally in one temporary binary. On archived
`m-1285` day 10, shallow runs in both execution positions selected
`6/60/320` with exact seed/local `34/34`; cardinality-first runs in both
positions selected `6/60/323` with exact seed/local `37/37`. All four runs had
six supported agents, exact validation and zero emergency. The official tier-3
gain is therefore `+3` and policy-stable. Local elapsed is ignored. This opens
only the frozen screen; BTC, promotion and commit remain prohibited.

The frozen screen closed at `3/24/0`, with only tier-3 differences, three
independent `+1` gains and no loss tail. Gains span low/default fuel,
one/two-tanker compositions and fuel-tight/threshold-corridor/balanced
families. General fixed/deadline lanes were `0/12/0`; high fuel was `0/3/0`
with identical exact settled counts across policies. Invalid and emergency were
zero. The already frozen full rows are now opened exactly once. BTC and commit
remain blocked until the full gate closes.

The first full general deadline-role execution for seeds 971010--971015 lost
its raw result lines because a PowerShell summary expression was malformed and
printed empty `//` fields. Those synthetic fields have no evidence value. With
the candidate and binary frozen unchanged, the same six already-opened seeds
may be repeated once solely to recapture raw output. No source or gate tuning is
allowed from this instrumentation recovery.

The full frozen matrix is complete at `4/74/0` over 78 unique terminal states.
All first differences are tier 3; gain tail is `+1`, observed loss tail is zero,
and invalid/emergency are zero. Lane results are general `0/36/0`, low fuel
one tanker `2/10/0`, low fuel two tankers `1/5/0`, default fuel `1/11/0` and
high fuel `0/12/0`. Gains span two independent fuel-tight seeds plus separate
threshold-corridor and balanced seeds across one/two-tanker and low/default
fuel. Together with the archived causal `m-1285` gain `+3`, the local semantic
gate supports canonicalization. The temporary switch must be removed and the
single cardinality-first capped-anytime order rebuilt before BTC. No commit is
authorized until BTC target-host validity, telemetry and real-opponent gates
pass.

Canonicalization is complete. The temporary policy API, replay switch and A/B
harness scaffolding are removed. The only production logic delta from `afcd2da`
is 31 lines in `src/orienteering.cpp`; it makes cardinality-first the single
pending-label order only when `minimumSpots` is present. The canonical unit suite
passes, and one canonical `m-1285` replay reproduces `6/60/323`, exact-local 37,
six supported agents, exact validity and zero emergency. Its BTC executable
SHA256 is `CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`.
Local elapsed output is not performance evidence.

The BTC admin statement supplied on 2026-08-09 confirms that response time may
vary by round. Therefore each fresh match configuration remains authoritative
for its outer response window, while the solver's internal compute cap stays at
5000 ms. `POST /practice` with the bot match token returned HTTP 401 because it
cannot create matches; an authenticated BTC UI then created fresh advanced match
`m-1796`: hard, three bots, 10 days, 32x32, 100 steps/day, 5000 ms response,
eight agents, 12 spots, six franchises and low fuel 1x. The frozen canonical
binary ran with harvest/future mode 7/7 and no rebuild or source change.

`m-1796` completed 10/10 valid applied days, reconciled 9/9 authoritative
transitions and independently validated every decision. Final score was
`6/60/312`; zero deadline skip, retry, emergency, exact overrun or hard-cap breach
occurred. Server response p95/p99/max were `2759/2759/2759 ms`, and decision
`totalMs` max was `2699 ms`. Day 10 exercised the candidate mechanism on six
supported agents: seed/local exact route servings were `39/40`, settled states
were `7,500,000`, enumeration plus finalization used `655 ms`, and overrun was
zero. The selected `6/60/312` equalled the master's optimistic upper bound even
though its separate search-guidance bound remained loose at 314. Replay SHA256
is `DCC0A6D36DFB7C2EFECB382ED220B977E37EBA145B25EF291DAF879C079EF272`.
Rank 1 over practice bots is only the asymmetric BTC failure gate. Target-host
validity/lifecycle/hard-cap evidence passes for this match, but the manifest still
reports `competitionReady=false` because `p99Calibrated=false`; this ten-day
sample does not replace calibration or real-opponent evidence. SCORE-QUEUE-048
remains frozen, active and uncommitted.

`ATTR-QUEUE-049` compiled clean parent `afcd2da` in an isolated worktree at
executable SHA256
`AB2556D6436C8AB14F53D6657E2C28FBA9E504DFD4BDF477A80C6CEC960D618A` and solved
only authoritative `m-1796` day 10 with its recorded prefix ledger/state.
Reverse-order repetitions were stable: candidate `6/60/312` with exact seed/local
`39/40`, parent `6/60/307` with `35/35`. Both sides supported six agents and had
zero emergency, overrun or validation failure. This is a fresh `+5` tier-3
development gain, not a new frozen-holdout row; local elapsed is ignored. Only
the terminal state is paired because the prior authoritative trajectory was
generated by the candidate, so no whole-match parent claim is made.

`ATTR-REAL-050` inspected the only two complete archived team-vs-team replays as
non-tunable observational controls. Both are two-team, four-day, 8x8, 32-step,
60-second-outer-window, fuel-64 fixtures with four agents, eight spots and four
brands. Live `m-1038` was `4/16/60` versus `4/16/52`; live `m-1042` was
`4/16/59` versus `4/16/53`. Candidate and clean parent replay-solve terminal
states tied `4/16/60` and `4/16/59` respectively with byte-identical plans,
closed optimistic/guidance bounds, exact validity and zero emergency/overrun.
This `0/2/0` control finds no short/small-map regression but is too narrow to
promote the low-fuel long-horizon queue change. Fresh competition-configuration
real-opponent evidence remains open.

`ATTR-GAP-051` checked whether fresh `m-1796` opens the next current-day score
axis. Days 1--8 have incumbent-to-optimistic serving gaps
`+1,+3,+5,+2,+1,+2,+3,+3`, but every exact-evaluated audit candidate ties the
selected current-day official score; no better feasible witness exists in the
pool. Day 9 optimistic/guidance are closed. Day 10 selected and optimistic are
`6/60/312`, while guidance `6/60/314` is only a loose unwitnessed bound. The
master deadline flag therefore cannot justify another master-width/search tuning
cycle. A post-QUEUE score axis requires an exact-valid higher bundle or another
first-differing feasible counterexample; this is not a ceiling claim.

`ROBUST-QUEUE-052` is the preregistered independent structural-downside audit of
the still-frozen `SCORE-QUEUE-048` candidate. Its manifest was frozen before any
harness change at `research/holdouts/ROBUST-QUEUE-052.csv`, SHA256
`D017068D6F47C73D22164265A235D545A24C2EB3396463264CB7D30AF8AA5359`.
Development contains 18 unopened small-map proof cases across balanced,
duplicate-brand and stock-contention profiles. The one-time holdout contains 72
unopened terminal cases covering low-fuel one/two-tanker roles, depleted default
fuel, a missing-lifetime tier guard and a high-fuel control. A research-only
same-binary policy switch must run shallow and cardinality-first in both orders
on identical state and caps; an exact stock-capped team DP reports official
lexicographic score, while small development cases also compare against complete
resource enumeration. Local elapsed has no authority. Any tier-1/tier-2 loss,
invalid or validator disagreement rejects the candidate; material/systematic
tier-3 downside also rejects it. The holdout may be opened once only after the
harness proves these invariants, and it cannot be retuned.

The `ROBUST-QUEUE-052` development proof passed `0/18/0`. Every capped shallow
and cardinality frontier yielded the same official team optimum as complete
resource enumeration; all repetitions were stable across reversed call order,
and every selected witness agreed between the exact simulator and independent
validator. Cardinality recall of complete maximal masks was never lower than
shallow and was strictly higher on 6/18 cases, without changing the optimum on
these small maps. This proves the harness/oracle is coherent but is not promotion
evidence. The 72-case holdout remains unopened at this point; local elapsed was
not recorded or used.

The `ROBUST-QUEUE-052` holdout was then opened exactly once and closed accepted
at `4/67/1` over 72 independent terminal fixtures. All five differences were
tier 3: gains `+1,+2,+1,+4`, with one `-1` loss. Low-one-tanker was `2/22/0`,
low-two-tanker `0/11/1`, depleted-default `2/10/0`, missing-lifetime `0/12/0`,
and high-fuel control `0/12/0` with identical frontier signatures. All 72 cases
were stable across reversed call order and dual-valid. The evidence file
`research/evidence/ROBUST-QUEUE-052-holdout.csv` has SHA256
`6B1DF3B756BF54EE00C25A595623F6814819AD957266F472FE137B4BACDA2EFD`.
The sole regression is bounded and not repeated across the second balanced
two-tanker seed; the larger and more diverse positive tail retains
`SCORE-QUEUE-048`, but does not authorize commit or replace BTC/real-opponent
final gates.

`ATTR-QUEUE-053` was opened as a bounded attribution. It inspected only the
already-opened low-two-tanker balanced seed `982008` and reported the shallow,
cardinality and union-frontier exact team witnesses. It did not change queue
priority, retention, caps, fixture or metric, and cannot tune against this loss.

`ATTR-QUEUE-053` closed accepted. Shallow's `6/60/96` witness requires patrol 4
mask `0x27C`, which is absent from that patrol's cardinality frontier; the
cardinality witness therefore stops at `6/60/95`. The union of both frozen
frontiers restores exactly `6/60/96` using the same shallow-only patrol-4 mask
and does not exceed it. Thus the one-serving loss is a real capped-frontier
trade-off, not DP or validation noise, but one isolated opened seed cannot
justify a mixed queue or dispatcher. Evidence is in
`research/evidence/ATTR-QUEUE-053.txt`.

`ARCHIVE-QUEUE-054` is the sole active non-opponent logic audit. Before any
replay-adapter change, one complete replay per unique 32x32, 10-day, 8-agent,
12-spot, six-brand BTC match was frozen in
`research/holdouts/ARCHIVE-QUEUE-054.csv`, SHA256
`FC874C1CCDC9785EE3D178AE31C3E4262B4B54913CB421E6B856D30A3655FB3D`.
The manifest contains 75 exact file hashes: already-opened `m-1285` and
`m-1796` are development, while the 73-case holdout contains 17 low-fuel, 36
default-fuel and 20 high-config matches. Ten high-config terminal states have no
fuel-constrained patrol and are whole-policy controls; the other ten contain one
or two depleted patrols, so only their per-agent high-fuel paths are controls.
This classification was derived from manifest metadata before source change.
Selection did not inspect score or rank.
The adapter must reconstruct every accepted prefix ledger and the terminal
authoritative state with dual validation before the existing same-binary
shallow/cardinality team oracle may run. Rank and local elapsed have no
authority; high-fuel policy signatures must be identical. The holdout opens
once only after both development replays reproduce coherent terminal evidence.

The `ARCHIVE-QUEUE-054` adapter proof passed `2/0/0` and reproduced the two
previously established independent deltas exactly: `m-1285` shallow/candidate
`6/60/320 -> 6/60/323` (`+3`) and `m-1796` `6/60/307 -> 6/60/312` (`+5`).
Both were stable under reversed policy order and dual-valid after reconstructing
the accepted nine-day prefix ledger. This is an adapter/oracle gate, not new
promotion evidence. The 73 replay holdout remains unopened at this point.

The `ARCHIVE-QUEUE-054` holdout was then opened exactly once and closed accepted
at `18/55/0` over 73 unique authoritative BTC terminal states. All differences
were tier 3 and every delta was positive: gains ranged from `+1` to `+4`, with
no loss. Low fuel was `5/12/0`, default fuel `12/24/0`, and high-config
`1/19/0`; wins span role masks `1,4,16,32,64,128,192` and terminal constrained
patrol counts `1..7`. All 73 cases were stable and dual-valid. The ten states
where the queue policy was unreachable had byte-identical signatures and score.
Evidence SHA256 is
`B8C3F78B702BB84BCE47592C6CD91F41D56329EBB464C7946875025B2B4B0776`.

Across the three independent frozen non-development matrices,
`SCORE-QUEUE-048` now has aggregate `26/196/1` over 223 same-state pairs. Every
difference is tier 3; frozen gain tail is `+1..+4` and the only loss is `-1`.
This is broad non-opponent evidence with bounded downside, not a weighted-sum
promotion and not proof against real opponents. The candidate remains frozen,
active and uncommitted because target-host p99 calibration and diverse fresh
real-opponent evidence are still final gates.

`PERF-P99-055` is the sole active non-opponent final gate. Its manifest was
frozen before creating a match at `research/holdouts/PERF-P99-055.csv`, SHA256
`52ADA0B0ECBF3ADE92381D2B4AF8926578D21FFF9433795B71A25A94BC4F225B`.
It preregisters 30 fresh explicit-advanced BTC practice matches: ten each at
low, default and high fuel, all hard/three-bot/10-day/32x32/100-step/5000-ms/
8-agent/12-spot/six-brand. The unchanged executable SHA256 must remain
`CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`.
Exactly 300 fresh decisions are required. With zero hard-cap breaches, the
one-sided 95% binomial upper bound on breach probability is
`1 - 0.05^(1/300) = 0.994%`, which is below 1%. Any invalidity, transition or
validator mismatch, skip/retry/emergency/overrun, hard-cap breach or binary drift
rejects calibration. Bot rank is not quality evidence. Interrupted work may
resume only the still-unfilled preregistered replicate count; failed matches are
not replaced or hidden.

`PERF-P99-055` progress: all ten preregistered low-fuel replicates `m-1803` to
`m-1812` are complete. The 100/100 decisions were HTTP-valid and dual-valid,
90/90 observable transitions reconciled, and every explicit advanced config
matched the manifest. There were zero fallback actions, emergencies, exact
overruns or hard-cap breaches. The worst per-match server-response maximum was
`3965 ms` and the worst decision timing was `3358 ms`; final quantiles remain
deferred until all 300 samples are complete. Per-replay hashes and summaries are
in `research/evidence/PERF-P99-055-progress.csv`. This is progress only; the
calibration gate remains open at 100/300 fresh decisions.

All ten preregistered default-fuel replicates `m-1813` to `m-1822` are also
complete. BTC materialized the UI default profile as `fuelLimits=200`; all
other explicit advanced fields matched the manifest. The 100/100 decisions
were HTTP-valid and dual-valid, 90/90 transitions reconciled, with zero
fallback, emergency, exact overrun or hard-cap breach. The worst per-match
server-response maximum was `3574 ms` and the worst decision timing was
`3514 ms`. The high-fuel lane then completed all ten frozen matches, but exposed
three submission skips: `m-1827` day 6 and `m-1832` days 6-7 recorded
`actions_deadline_skip` plus `actions_server_wait` and never POSTed the valid
plans. Consequently high fuel returned only 97/100 accepted action results and
BTC exposed tier-2 daily sums 57 and 52 in those two matches. Across the 297
accepted responses, p50/p95/p99/max was `2989/3575/3965/4328 ms`, but these are
censored by the skipped submissions and cannot calibrate p99. `PERF-P99-055` is
closed rejected. Progress evidence SHA256 is
`8DCDAE465FB065FA0E0CEACFB78B906BA43BE82F37C950B9B7E963136B90976A`;
attribution evidence SHA256 is
`7F8628EC949D36F50E685F8BC719C1B0D189A4226B2EFF7F34DD4BBFDF81A7AF`.

The confirmed non-opponent gap has two aligned deadline defects. First,
`deadline_seconds` validates but ignores raw server `endsAt` and floors
`(receivedMs + responseBudgetMs) / 1000`; the affected raw windows
`5030/5198/5032 ms` became only `4030/4198/4032 ms`, losing
`970/802/968 ms` against the configured 5000 ms cap. Second, certification did
not consistently stop at the compute boundary implied by its existing 1600 ms
network reserve: planner timing consumed `3264/4171/3678 ms`, leaving only
`760/25/349 ms` at the 800 ms submission guard. Full decision replay recording
took only 2-3 ms and is not the material cause. The next experiment must align
planner and POST to one exact millisecond deadline capped by both the configured
5000 ms and raw server deadline, then enforce the existing reserve without
reducing solver logic or hiding skips.

`DEADLINE-ALIGN-056` froze exact deadline plumbing before source change. Its manifest
was frozen before source change at `research/holdouts/DEADLINE-ALIGN-056.csv`,
SHA256 `6D6270CEBBD12D96ACDBF779A8298466A0378793F2173C25D8514A3E843DAC3F`.
The candidate may only plumb one exact HTTP deadline equal to
`min(raw server endsAt, receivedAt + configured response budget)` into both the
planner and POST path. It may not change search/certification logic, the 1600 ms
network reserve, the 800 ms submission floor, action durability or recovery.
Exact alignment is tested first as the minimal general fix. If any of five fresh
high-fuel development matches still skips, this candidate is rejected rather
than weakening the guard; certification boundedness must then be a new,
separately frozen mechanism.

The candidate is frozen as BTC executable SHA256
`4636C380578F1AB79973A25E03AAE26E20809F8A5720F914D7DB7F38D1C3EFD6`.
The unit suite passed once. The parent and candidate replay-check outputs were
byte-identical across all 30 `PERF-P99-055` replays and all 300 recorded days,
with zero failure or mismatch. This establishes offline action/state/validator
equivalence only; local elapsed time is deliberately excluded. Evidence SHA256
is `54F2C1EE87964495ACA50F317A400072CBA75829D5C0554FF27E3D34D561D854`.
No fresh `DEADLINE-ALIGN-056` BTC development sample was consumed. The
candidate is closed inconclusive rather than promoted or rejected: exact
deadline alignment is a necessary protocol/runtime component and passed offline
equivalence, but `ATTR-CERT-058` proved a separate certification boundary hole
that this candidate deliberately leaves unchanged.

While BTC practice creation was unavailable, `ATTR-SELECT-057` completed an
independent read-only attribution over the already-opened 30-replay
`PERF-P99-055` set. The scan found zero non-selected candidate with a higher
`scoreAfterToday` and zero accepted exact bundle above the selected current-day
score. Two apparent lower-bound improvements (`m-1803` day 8 and `m-1804` day
9) lose current official score and are correctly excluded by the production
current-floor gate. The remaining q50 improvements tie current score and lower
bound but q50 is only third in the full q95/q80/q50/q20/q05 comparator (or the
candidate is outside the relative confidence gate). None is a complete-tuple
domination witness. This closes the evaluator axis without a source change;
evidence SHA256 is
`DAEF6B1842D8EDE357FA23BE2DA7DBD68CBFDE3DE572641C2718C83C8CC01F8F`.

`ATTR-CERT-058` is an accepted independent read-only target-host attribution and
does not modify the frozen `DEADLINE-ALIGN-056` candidate. In all 42/300
decisions that crossed the scheduler compute boundary, pre-certification still
ended before the boundary; certification alone crossed it. Counts by
low/default/high fuel are `0/5/37`; overrun p95/max is `405/1574 ms`, and the
three submission skips are the three largest tails. The first boundary hole is
dense initialization in `enumerate_exact_high_fuel_routes`: after one deadline
poll, each worker initializes at least 36 MiB over 4,194,304 states before the
next poll, and four joined workers can touch at least 144 MiB. The general
semantics-equivalent successor is same-sentinel chunked initialization with the
existing absolute deadline polled between chunks. Evidence SHA256 is
`6DF72E0342AA141A39DD8DA1D030D72FFB05293BBCD8E0FACF08D5E81C9151C8`.

`PERF-CERT-059` is now the sole active implementation experiment. It retains
the frozen exact-deadline plumbing and changes only dense exact-state
initialization to construct identical sentinels in fixed 65,536-state chunks,
polling the existing absolute deadline between chunks. At BTC scale this bounds
state initialization between polls to 576 KiB per worker and 2.25 MiB across
four workers, without changing graph/order/caps/worker count or the result of a
completed search. Its manifest was frozen before source change at
`research/holdouts/PERF-CERT-059.csv`, SHA256
`E9A6BF19348A1AE04325E59F9CAEC1552FE4690A5506C2EC2040F018D66D2B20`.
Local elapsed time has no promotion authority.

The implementation is frozen as BTC executable SHA256
`83EF15BE2139705ACE27B08727ED18074C8B9305E8FBD52566000BCC207B47E0`.
The authoritative cached toolchain built the candidate once and the full unit
suite passed. Against the frozen `DEADLINE-ALIGN-056` executable, replay-check
stdout is byte-identical for all 30 `PERF-P99-055` replays and all 300 recorded
day states: 10/10 low, 10/10 default and 10/10 high replay files matched, all
replay hashes matched the frozen progress evidence, all processes exited zero,
and there were zero invalid actions or validator disagreements. This clears the
offline semantic gate only; it says nothing about local performance. Evidence
SHA256 is
`394FF66499700E060B09BCFD299109045CCE8CA8610B794CBB87DE2F7CE54C75`.
The follow-up source-boundedness audit proves that sentinel construction between
polls is now at most 576 KiB per worker (2.25 MiB across four workers) at both
BTC scale and the architecture state limit. Main traversal and terminal scans
already poll by bounded entry/mask/cell intervals. Individual allocator calls,
the small pre-traversal metadata pass and bounded simulator/validator calls
remain explicit observation points rather than speculative patch targets.
Boundedness evidence SHA256 is
`51856B408CE1A5C63FE9CEEA22BB0C6615E2F3AFBD80CB58578106E132946CDD`.
BTC practice creation recovered and the first frozen high-fuel development
fixture was consumed as `m-1836`. It used hard/3 bots/10 days/32x32/100 steps/
5000 ms/8 agents/12 spots/6 brands/high fuel. The binary produced 10/10 valid
actions, replay-check confirmed 9/9 authoritative transitions and exact score
`6/60/407`, and there were zero emergency decisions. Rank 1 is ignored.
Nevertheless, certification crossed the `total - 1600 ms network` compute
boundary on days 3, 6, 8 and 9 by `363/47/307/218 ms`; pre-certification stayed
inside the boundary on every day. Current-day exact orienteering also overran
its own supplied deadline by 42 ms on day 4. This fails the preregistered zero-
crossing development gate, so the balanced BTC holdout remains unopened and
`PERF-CERT-059` is rejected as a promotable candidate. Replay SHA256 is
`B866ACB6A63C548D8BE05FBED03DDBD355422F78566CAC436A1C2E5BD6AC6676`;
BTC evidence SHA256 is
`DCD0C051D782B247CD3ADABA241DF0417CB1ABD003C8B3AFD09D6A26197A4419`.
The chunk mechanism remains only as a frozen diagnostic parent: offline
semantics are byte-identical and the first large initialization interval is
removed, but no successor may be promoted until the next crossing operation is
independently attributed. Do not tune chunk size or deadline guards from this
single match.

`ATTR-CERT-060` was opened as the sole certification-attribution experiment. The
separate frozen score candidate `SCORE-QUEUE-048` remains active pending its
runtime/real-opponent gates; `ATTR-CERT-060` neither changes nor reevaluates its
score evidence. This experiment is audit-only telemetry on the frozen rejected
`PERF-CERT-059` diagnostic parent, not another solver candidate.
It will time the existing future-witness phases and record the first operation
ending after both its candidate slice and the shared global certification
deadline, including nested exact-orienteering diagnostics. It may not change
control flow, deadlines, caps, candidate/scenario order, actions or score logic.
The manifest was frozen before source instrumentation at
`research/holdouts/ATTR-CERT-060.csv`, SHA256
`FA868D82D6880A408DC5C146293A4194EDCD59E716347787024C6D27CF78D402`;
design evidence SHA256 is
`4CC31C75472D5D51DDE481B3F3B1F8BD7526C536FD5250A111771C5200AF915B`.
Before any BTC diagnostic run, replay-check stdout must remain byte-identical to
the frozen `PERF-CERT-059` binary on all 300 states. A logic successor requires
a new manifest after attribution; no threshold or reserve change is allowed.
The audit binary is frozen at SHA256
`2A1BDA4F9E67FC3EC70F3A4144AA14145B9DA5DF64F5DBE29C5B45CC3F203C04`.
The full unit suite passed and replay-check stdout is byte-identical to the
frozen `PERF-CERT-059` binary on all 30 low/default/high replays and all 300
states, with correct replay hashes, exit zero, zero invalid and zero validator
disagreement. Offline diagnostic-integrity evidence SHA256 is
`183043B7992E1DA2359C965CE7D96DEE05B42F9C446FAFA9FD1236B2578D77BA`.
The three fresh high-fuel BTC diagnostic fixtures were consumed without
replacement; their ranks are irrelevant. `m-1837` and `m-1838` each completed
10/10 HTTP 200 valid actions and 9/9 reconciled transitions. Replay-check rebuilt
`6/60/440` and `6/60/384`; replay SHA256 values are
`7325885ADEDC16356CF1D4A694C563A994EE532335B9A30872564B5DA56E4C85` and
`C2D84D6A48132EDD8208C133A8EB0F858321FBC177F1CF6AE0CAA34A91AD2017`.
`m-1839` stopped after setup with no day state/decision/action and was not
replaced. Across the 20 complete decisions, every action was accepted and no
solver total exceeded either its exact authoritative deadline or the 5000 ms
hard cap. The diagnosed events are crossings of the earlier 1600 ms network-
reserve boundary, not invalid responses or hard-cap breaches.

Every certification-caused global crossing ended in column generation.
Material `m-1838` days 4/5/6/9 spent `1394/879/802/791 ms` in nested exact
orienteering and crossed globally by `381/250/23/69 ms`; maximum nested exact
overruns were `609/250/254/69 ms`. Days 1 and 7 first crossed in monotone-floor,
but certification consumed only 1/0 ms because pre-certification had already
used the reserve; they are not certification regressions. Current-day day 1
independently spent 252 ms enumerating and 2485 ms finalizing exact routes,
overran its supplied exact deadline by 1756 ms and visited 3,000,173 feasibility
nodes.

The source cause is exact: `capacity_feasibility_search` polls every 1024 nodes,
but deadline expiry returns ordinary `false`, indistinguishable from an
infeasible subtree. Parent recursion then continues sibling routes, so repeated
expiry polls prune only one subtree at a time and traversal can reach the
3,000,000-node phase cap. `ATTR-CERT-060` is accepted attribution. Evidence
SHA256 is
`6D762D6C3D19DEF3B77B08ABB4F4BD404D228A83D79ECCBE0F693DAE108B47A1`.

`PERF-CERT-061` is the sole active runtime implementation experiment. Its
manifest was frozen before source change at
`research/holdouts/PERF-CERT-061.csv`, SHA256
`D798DD62F7AFA090F2E2801200336B8A44768554569F7AAD50D792EA9057CD9E`;
design evidence SHA256 is
`FF42323F9C88D56967899695E92AEEA8483AD73F80957AB55E50320D41726C78`.
It may add only explicit cancellation propagation after the existing absolute-
deadline poll, restoring recursion state while unwinding and retaining the best
completed incumbent. It may not change poll cadence, route order, bounds,
memoization, node caps, beam, worker count, reserve, submission guard or score
logic. This does not remove, disable, defer or reduce designed functionality;
no code is deleted. It repairs the designed deadline contract and preserves all
exact work before expiry.

The candidate is now frozen as BTC executable SHA256
`F452AA3936801A68C7E1209D95B5E1724AF8484F480BE44F6475B8677A22039F`.
The cached MSVC/CMake/Ninja toolchain built the test and BTC targets and the full
unit suite passed once. Against the frozen `ATTR-CERT-060` audit parent,
replay-check stdout is byte-identical on all 30 preregistered low/default/high
replays and all 300 states; every process exited zero with zero invalid action
and zero validator disagreement. Local elapsed time is excluded. Offline
evidence SHA256 is
`F8AB1787E4400C72416D860A1F854FE87EF88E00C91EB2AE3BCFDA3CB0BCC4EA`.
The next and only open gate is one fresh high-fuel BTC development fixture with
this exact binary. The balanced target-host holdout remains closed until that
development fixture has zero material certification crossing, no POST skip,
zero hard-cap breach, zero emergency and complete transition reconciliation.

Fresh high-fuel BTC development fixture `m-1862` used the exact registered
hard/three-bot/10-day/32x32/100-step/5000-ms/eight-agent/12-spot/six-brand/high-
fuel configuration. The frozen candidate produced 10/10 HTTP 200 valid actions,
reconciled 9/9 transitions and replay-check rebuilt `6/60/410`; rank is ignored.
Maximum solver total was 2339 ms, remaining authoritative window at POST was at
least 1604 ms, and there were zero emergency, retry, server WAIT, no-POST or
hard-cap events.

The prior material future-certification tail did not recur. Days 1--9 ended
their global certification boundary by only 2--7 ms, and maximum nested exact
overrun was 1--9 ms, structurally bounded by the existing 1024-node poll block
plus recursive restoration/unwind. Current-day exact separately overran its
own slice by up to 166 ms, but the whole planner still retained the complete
1600 ms network reserve; this remains observable rather than being mislabeled
as fully closed. Replay SHA256 is
`693DC792B9419937DA455BFCDEEFF89684BB8EED2C529F082DD8BC29AF6ADEC8`;
development evidence SHA256 is
`9D4E1D7F40C1863EE9F6575FF98B3A4B959DA8C302E9FE19FCBF8C0EF4978187`.
The development gate passes and the unchanged binary may now open the frozen
balanced low/default/high BTC holdout exactly once.

The balanced holdout was opened once without replacement. Low-fuel `m-1863`
passed 10/10 valid and 9/9 reconciled with zero emergency/crossing/no-POST,
solver max 1915 ms and at least 1618 ms remaining at POST. Default-fuel
`m-1864` produced seven valid decisions, then the HTTP client terminated during
receive with WinHTTP 12002. Immediate same-match resume retrieved only the final
result, so days 8--10 were lost. High-fuel `m-1865` produced ten decisions and
nine accepted actions; authoritative day 2 arrived with only a 1380 ms exact
window, already below the unchanged 1600 ms reserve, causing one emergency and
one `insufficient-authoritative-day-window` server WAIT.

Aggregate holistic result is 26/30 accepted actions, 24/24 reconciled observed
transitions, one emergency and four missing POSTs. Replays for low/default/high
are `BE7E3753B95C3A566EA03DAC2592CBF02A36D9059A7FDDC5655EFD1F11198C2E`,
`B4F187FD1D1B98DAD4FF8A6016131528A64DE2B497639AF03AA0DBD8E7834880`
and `F2F28156A9CA2F84A427E61F333017ADEE9766466454C924D5FA92917808C9FC`.
The cancellation mechanism itself remained clean on all 27 decisions reaching
the solver: global certification overrun max 6 ms and nested exact max 10 ms,
with no tail moved to another future-witness phase.

`PERF-CERT-061` is therefore rejected as a promotable composite and cannot be
committed. Its cancellation repair is retained only as the frozen
semantics-equivalent research parent because the causal target passed. The new
independent gap is HTTP state reception: a receive timeout can terminate the
whole match, and a state can arrive after the solver's 1600 ms reserve is already
unavailable. Holdout evidence SHA256 is
`53541BE0FD0ED26EAB77DBD0A94BBF67AB7CFBD6B27CC49AA7F12A6F58E82745`.
No reserve, submission floor, map, fuel or bot threshold may be tuned from these
fixtures.

`ATTR-HTTP-062` is accepted read-only attribution. `WinHttpClient` gives every
request a session-default 5000 ms receive timeout unless the caller supplies a
shorter slice. The accepted action path supplies a bounded slice and catches
retryable timeout/resend exceptions, but `wait_for_get` catches no transport
exception and the main `/state` and `/result` callers use raw default-timeout
GETs. Therefore one receive 12002 escapes `run_http` and terminates the process,
exactly matching `m-1864`. All `/setup`, `/start`, `/state` and `/result`
operations are idempotent GETs but are not wired to a common retry contract.

`m-1865` wire day 1 was recorded with exactly 1379 ms before raw `endsAt`, which
proves the late-state counterexample but not its exact transport cause because
request-start telemetry is absent. It is only consistent with the same unbounded
GET receive interval. Evidence is `research/evidence/ATTR-HTTP-062.md`.
No source, solver, reserve, submission floor or timeout changed in this
attribution. A separately frozen successor may add bounded retry across every
idempotent GET caller; it may not lower the 1600 ms reserve or 800 ms guard, tune
against `m-1864`/`m-1865`, or suppress idle post-ACK work.

`PERF-HTTP-063` is preregistered before source change. Its frozen holdout is
`research/holdouts/PERF-HTTP-063.csv`, SHA256
`FB5C49FF5CCDC2AAD89C56099BFE4F17ED0731920AC59E131DAA783EAD1F6F40`;
design evidence SHA256 is
`C1748922EC660E514CF55280F03F765CE7C2B0D09113B953B446DF29974F3DCE`.
The candidate may reuse only the pre-existing 750 ms transport slice and the
existing timeout/resend classification. It must wire `/setup`, `/start`,
`/state` and `/result`, preserve transient-status handling and idle post-ACK
work, and leave every POST, solver, action, ledger, reserve and guard unchanged.
The first gates are source proof, frozen replay equivalence and one controlled
localhost receive-timeout recovery. Only then may one fresh default-fuel BTC
development match be consumed; the low/default/high holdout remains unopened.

`PERF-HTTP-063` passed the unit suite and the frozen 30-replay/300-state
byte-equivalence gate. Its frozen BTC executable SHA256 is
`21D9C7E6BC606C80E1B12A3886745006DE524380612E2235306C372B4EE07B6B`.
The corrected controlled fixture completed exactly one action, one accepted ACK
and one result, but a 3000 ms first-state response-header stall produced zero
`state_transport_retry`. Therefore `WinHttpSetTimeouts` alone did not implement
the declared header-wait bound, and the candidate is rejected before BTC or its
holdout. Evidence SHA256 is
`00FF3AFF0F7AB2E0B518A20F2B31A2A1EC3A420D559A2909DF4D0554869FAE12`.

Official WinHTTP documentation identifies
`WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT` as the request option that specifically
bounds receipt of all response headers; its default is 90 seconds.
`PERF-HTTP-064` is preregistered before adding that option. Its sole new mechanism
sets the option to the same already-frozen positive `ioTimeoutMs` on requests
that already have a bounded slice. Assignment POST remains unchanged at timeout
zero; existing action ACK recovery still permits only identical-body resend, and
GET recovery remains idempotent. Frozen holdout SHA256 is
`517A3CAFE052C25E1340ECA0C46A2601130FF02789DF97174679B93201C08781`;
design evidence SHA256 is
`841D56D3B63A8CF7A5F1FF6E569949BA8C820EE0B4E06285F4CC3E24451416A8`.
No 750/1600/800 ms value, poll interval, solver, action or ledger logic may
change. The corrected controlled header-timeout fixture is the first gate; BTC
development and the balanced holdout remain unopened.

`PERF-HTTP-064` is rejected. Its frozen binary SHA256 is
`ECB20530412264CF5C4DCCC1340A163D447272A9AB28610FF7ED87A1C5AF6E85`.
The unit suite passed once and all 30 frozen replays/300 states were
byte-identical to 063, but the corrected 3000 ms silent-header fixture again
produced zero `state_transport_retry` while completing exactly one action, one
accepted ACK and one result. No BTC match was opened. The GET retry and header
option are removed from final source. The natural WinHTTP 12002 remains one
sample after 30 prior fresh BTC matches on the same GET path produced none;
transport may reopen only after repeated target-host evidence under a stable
network, not another synthetic timeout variation.

`FINAL-QUEUE-065` is preregistered as the only final candidate before its clean
binary is built. Frozen manifest SHA256 is
`05BCA7A18146284615B367AE84EA687BBA37557A25336950F34A20C409CD0AF3`.
Its score authority remains the 223 independent frozen pairs from
`SCORE-QUEUE-048`: aggregate `26/196/1`, every first difference tier 3, gain
tail `+1..+4`, only loss `-1`, zero invalid/emergency. The final cumulative
runtime changes must be byte-identical to frozen SCORE-QUEUE-048 on all 30
PERF-P99 replays/300 states before opening exactly twelve fresh BTC recurrence
matches: four low, four default and four high fuel, with no replacement. Local
elapsed is excluded. No additional score research is permitted during closure.

The clean final build and unit suite passed once. Frozen BTC executable SHA256
is `8A8AD48AD5A9227AA43C9FF685D20CE7CA1EF657EBCDFC2A675C6FEA9A16816D`.
Against frozen canonical `SCORE-QUEUE-048`, replay-check stdout was
byte-identical on all 30 preregistered replays and all 300 states with zero
failure. Design/offline evidence SHA256 values are
`ED27742A75F4C67199803D7ED8EF9838EB6B2ABCFF114DFBDC2294936E37B180` and
`6DA98FC48FEB2103778CF4E7DC7AE77F33DC493733470A222999C4B0A7317BA0`.
This opens only the frozen twelve-match BTC recurrence row; source and binary
must not change during it.

The twelve-match target-host row is complete and accepted. All twelve explicit
advanced setups matched the frozen manifest. Aggregate results are 120/120
HTTP-valid accepted actions and 108/108 independently replay-reconciled
transitions, with zero emergency, deadline skip, server WAIT, fallback,
transport retry, WinHTTP 12002 and 5000 ms breach. Maximum decision `totalMs`
was 2753 ms. Maximum exact-orienteering deadline overrun was one bounded 6 ms
tail in high fuel with no submission effect; all other matches recorded zero.
BTC evidence CSV/MD SHA256 values are
`92990A8D7113CA9CA72597A6750504196E4F6DDFB10F2D00ADD7B294143060FF` and
`8F8511E305405703D44B439F461D04676055A10E05BCC138C0AB0DE1BEDD6B21`.
Rank is ignored. The same frozen binary entered the real-opponent queue once;
no second queue entry or new logic axis is authorized during closure.

The fresh real-opponent queue entry expired with no other team and created no
match, so no live human result was lost or silently omitted. Archived human
terminal controls remain `0/2/0`. This unavailable non-paired sample cannot
promote the candidate, but it also cannot override the causal promotion
authority: 223 frozen paired rows at `26/196/1`, broad tier-3 gain `+1..+4`,
only one `-1` loss, zero invalid/emergency, 300-state cumulative semantic
equivalence and the clean twelve-match target-host gate. `FINAL-QUEUE-065` is
therefore accepted and commit-authorized. This is a practical promotion, not a
claim that diverse fresh human opponents or the entire architecture have
reached a mathematical ceiling.

The read-only semantics attribution `SEM-REFUEL-001` is accepted. On
`m-1258`, transitions to days 9 and 10 reported a patrol at the simulator-predicted
terminal position but with full fuel `300` instead of predicted `185` and `194`.
In both cases the patrol first arrived on the tanker cell at step 100; the local
trace did not count a completed full co-location step. A scan of all 37 archived
live replays found no second match with this exact terminal-arrival/full-refuel
pattern. Therefore neither the documented sustained-docking rule nor a server
day-boundary refuel rule is yet established. No simulator or planner change is
authorized until a preregistered BTC conformance match isolates this boundary.
That match, `m-1261`, did so: assignment `[tanker, patrol, patrol]` was accepted;
one patrol first reached the stationary tanker at terminal step 20 and another
arrived at step 19 then waited one step. Both began day 2 on the tanker cell with
full fuel 20. The terminal-arrival patrol would have retained fuel 14 under both
local engines. The active candidate is now `SEM-REFUEL-002`, which may change only
the next-day terminal fuel transition after all current-day semantics are complete.
It must not grant fuel to an action within the day or alter score/traffic/claims.

`SEM-REFUEL-002` is accepted. Both engines implement the boundary independently;
unit tests pass, and rescanning all 37 live replays improves `m-1258` transition
agreement from 7/9 to 9/9 without introducing any mismatch. Its frozen paired
score matrix is 8 wins, 11 ties and 5 losses with zero invalid/emergency. Every
loss is tier 3 and at most five servings; the candidate has one tier-2 win and
low-fuel tier-3 gains of 86, 24 and 1. Fresh explicit-advanced BTC `m-1264`
selected mask128, submitted ten HTTP-2xx valid plans, reconciled all 9/9 state
transitions and ranked first at `6/60/261` versus `6/60/202` next. Maximum solver
time was 2845 ms, replayed role selection 3956 ms, and exact deadline overrun,
submission skip, server WAIT and emergency were all zero. Replay SHA256 is
`2811EC26C075829D63F61B7949394470B568CFDA72F6B30A51838CF9ACD4540A`.

`SCORE-ROLE-022` investigated fresh BTC `m-1264`, which selected one-tanker
mask128 and scored `6/60/261`, while same-budget production counterfactuals on the
same replay reached `6/60/326` with two-tanker mask192 and `6/60/330` with mask144.
The live mask counterfactual exactly reproduced `261`, so this is not a budget
artifact. On independent low-fuel replay `m-1255`, current one-tanker mask64 scored
`6/60/286`, while two-tanker masks80 and48 scored `313` and `305`. A blanket
two-tanker rule is false: on `m-1241`, one-tanker mask32 scored `331` and the best
two-tanker rollout mask96 scored only `282`.

The preregistered discriminator is visible before exact counterfactual score. On
`m-1264` and `m-1255`, the best one- and two-tanker incomplete rollouts tie on
lifetime and daily distinct; on `m-1241`, two tankers already lose daily distinct
`56` versus `58`. `SCORE-ROLE-022` may prefer the best existing two-tanker beam
member only for long-horizon low-fuel matches when evidence is incomplete, at
least one patrol remains per brand, and its rollout ties the best one-tanker on
the first two official tiers. It preserved all other paths and did not inspect
seed, match ID or map family, but it is rejected because the production
`beam-width 8` never retained a two-tanker assignment. The apparently successful
development attribution used beam width 16. On the frozen screen, all six short
horizons were exact role/score ties; all four low-fuel cases kept the same role
mask, with two raw local tier-3 cutoff deltas of -4 and -2 but causal `0/4/0`.
The unopened archive probes `m-1252`, `m-1138` and `m-1134` likewise contained no
two-tanker member at width 8. The source and unit-test candidate were fully
reverted; no production change is retained.

The next gap is therefore beam diversity, not the fallback comparator. Timed role
selection spends its probe phase only on all-patrol and at-most-one-tanker masks;
those refined masks fill the eight production slots before scanned two-tanker
masks can enter. Any successor must preserve the caller-visible beam width and the
same 5000 ms deadline while giving a bounded comparison pool enough tanker-count
diversity for the already demonstrated m-1264/m-1255 counterfactual. It must still
reject m-1241 through a pre-score lifetime/daily discriminator.

The active candidate is `SCORE-ROLE-023`, preregistered before source changes at
holdout SHA256 `B2F0A6E8532F0CEFA8408BAF5A6378133FBCFE441943D6C99D9091799081676A`.
For only long-horizon low-fuel configurations where two tankers leave at least one
patrol per brand, the internal comparison pool may expand to at most the smaller
of all masks and `max(requested width, 2 * agent count)`. It receives no extra
time: the existing rollout deadline is divided across the larger pool. After the
same tier-preserving fallback, the vector is reduced to the caller's requested
width before return. This composes the causal beam16 evidence with the rejected
candidate's strict lifetime/daily equality guard without changing public width or
the 5000 ms hard cap.

`SCORE-ROLE-023` passed its frozen matrix and the BTC final gate. Fresh low-fuel
results were `4/1/3`, all at tier 3: gains `+9,+1,+46,+5` totaled 61, losses
`-3,-23,-8` totaled 34, with tails `+46/-23` and zero invalid/emergency. The six
short-horizon cases were five exact ties plus one raw timed crossover gaining one
serving while the candidate branch was disabled. All eight default/high cases
kept the exact parent role masks with the candidate branches disabled; their raw
local cutoff deltas are retained in the experiment log but the causal result is
`0/8/0`. No protected case regressed lifetime or daily distinct.

Fresh explicit-advanced BTC `m-1266` used hard difficulty, three bots, ten days,
32x32, 100 steps/day, 5000 ms, eight agents, 12 spots, six brands and low fuel.
The candidate selected mask1 and finished rank1 at `6/60/228`; the next bot was
`6/59/158`. Parent `5599e76` selected mask4 on the same setup, whose exact
counterfactual was `6/60/220`, so the live candidate gain is causal `+8` at tier3
in addition to the tier2 victory over bots. Target role wall time was 4641 ms;
action p95/p99/max was 3178 ms and maximum recorded decision time was 3105 ms.
All 10 submissions were HTTP 200 valid, all nine transitions reconciled, and
emergency count was zero. Replay SHA256 is
`11F037E67D344D14D0E9A8AA9230D4DC482D3A5272DA764732C340980F690649`.
The candidate is accepted as the new global production champion.

`SCORE-ROLE-024` started from parent `08771f1`. On BTC
`m-1266`, the best retained two-tanker mask192 scores exact `6/60/275`, which is
47 servings above the live champion mask1, even though its incomplete rollout is
`6/48/176` versus `6/51/192`. A daily-gap threshold alone is unsafe: on m-1241
the best two-tanker rollout is only two daily distinct behind (`56` versus `58`)
but loses 49 exact servings. The pre-score discriminator is patrol-normalized
tier-3 productivity. Cross-multiplication gives `176*7 > 192*6` on m-1266 and
`160*7 < 193*6` on m-1241; it also preserves the accepted equal-daily m-1264 and
m-1255 cases. The candidate may relax exact daily equality by at most one full
brand-day only when two-tanker servings per patrol are no worse. This is a
conditional comparison inside official tiers, not a weighted score.

`SCORE-ROLE-024` is rejected and not present in source. Its fresh short-horizon
lane was `0/6/0` exact. All 12 low-fuel cases reached `6/60` with zero
invalid/emergency; seven retained the same mask and revealed local cutoff noise as
large as -83. Seed861001's flip was unstable across fixed repeats and was excluded
from causal verdict. Stable same-binary fixed attribution on the other four flips
was `2/0/2`: gains `+6,+5` versus losses `-54,-15`, tails `+6/-54`. The high-stock
and threshold losses dominate the overnight and rare-brand gains. This is a
material global defeat even though m-1266 is positive, so default/high and BTC
were not opened. Patrol-normalized early productivity cannot protect total
late-horizon capacity. Source and tests are byte-identical to champion `08771f1`;
the opened masks may not be used to retune this axis.

`SCORE-ROLE-025` tested terminal patrol fuel per patrol as an independent
late-horizon discriminator. It is rejected before holdout and absent from source.
On m-1266 it would select mask192, whose terminal patrol fuel is `600/6` versus
one-tanker `486/7`, and whose exact score gains 47 servings. But on the development
counterexample m-1241 it selects mask96: rollout `6/56/211`, terminal patrol fuel
`466/6`, over one-tanker mask32 at rollout `6/57/195` and fuel `328/7`. Exact
counterfactual mask96 is only `6/60/302` versus the previously measured mask32
`6/60/331`, a 29-serving loss with the first two tiers tied. Terminal fuel is
therefore surplus after under-serving, not a safe capacity signal. The frozen
holdout remained unopened and may be reused only for a mechanism independently
derived from new telemetry.

`SCORE-ROLE-026`, parent `08771f1`, tested the measured structural
gap is budget allocation, not another terminal proxy: the expanded low-fuel beam
coarsely rolls all 16 masks only through 85% of the role budget, while the interval
to the established 92% role wall is used only to prewarm the already selected
mask. The preregistered mechanism spends that interval on a paired richer rerun of
the current best one-tanker and best two-tanker, with equal deadline slices and
the same exact simulator/validator. Each mask retains the lexicographic maximum
of its old and new evidence, then the unchanged lifetime/daily equality guard is
applied. There is no daily relaxation or new feature threshold. Development must
first had to admit profitable m-1266 mask192. A loaded-local run left mask192 at
`6/48/176` behind mask1 at `6/51/192`, but this deadline-sensitive result has no
performance authority because the local host was contended. The candidate is
inconclusive and absent from source; m-1241 and fresh holdout SHA256
`A785F6F89FA8AEBD7BA4924CF1FA4FDFA529798F5C3B9D9507C0F6D67B65D238` remained
unopened. It may be resolved only by a BTC live run at the explicit 5000 ms cap.

Temporary per-day attribution identified that mismatch. On m-1266 mask192, the
role rollout selected the master plan on all ten days and patrol fuel reached the
full `600/600` by day 4, yet cumulative daily coverage advanced by only four or
five brands on most later days while servings repeated at 18 per day. On m-1241,
the same evaluator usually retained five or six brands and fuel was not saturated.
`RouteColumnGenerator` orders target spots by lifetime rarity and stock, then its
anytime query and seed caps retain distinct spots, not distinct brands. With six
target slots and two spots per brand, duplicate high-stock brands can therefore
erase lower-stock brands before the official master sees them.

`SCORE-ROLE-027`, parent `08771f1`, tested a preregistered change that
promotes the first target of each brand ahead of duplicate-brand targets in both
query order and retained-seed order, then fills remaining capacity with the
unchanged priority/spot rules. Query counts, seed counts, deadline and official
comparison remained unchanged. It is rejected and absent from source. On m-1266
it raised one-tanker mask2's rollout to `6/60/169` and selected that mask, while
the best visible two-tanker was still only `6/52/179`. Frozen exact development
scores are mask2 `6/60/206` versus mask1 `6/60/228`, so the apparently improved
coverage chose a role with a known 22-serving loss and still hid mask192's gain.
Fresh holdout SHA256
`4CFF75E3D849C82466BBCCB1FEC1ED2FFB8ADAE98C800083474877C1BAF763D8` remained
unopened. Rollout coverage is not promotion evidence unless the retained columns
also improve the actual day solver under exact attribution.

Exact day-level attribution now shows why two-tanker capacity is invisible. A
fresh champion counterfactual for m-1266 mask192 scored `6/60/267` (the same
positive direction as the prior `275`) versus live mask1 `6/60/228`. Mask192's
daily servings were `29,29,27,25,27,25,25,25,26,29`; mask1 was
`31,32,30,23,23,15,17,18,21,18`. The benefit begins after day 3 and comes from
maintaining harvest throughput, not from a first-tier or second-tier trade. The
role evaluator instead saturates all six patrols at fuel 100 but remains near 18
servings/day. Code attribution confirms full `solve_day` enables production
harvest extensions and conditional orienteering, while `rollout_role_assignment`
leaves every harvest-extension option disabled.

`SCORE-ROLE-028`, parent `08771f1`, enabled long-horizon low-fuel
full role rollouts only, it will reuse the canonical production harvest-extension
configuration: enabled extensions, uncached target allowance, configured source
count and day-dependent depth. One-day probes and short/default/high lanes remain
unchanged; all work stays inside the current 85% rollout wall and 5000 ms hard cap.
configuration inside the existing slices. It is inconclusive and absent from
source. A loaded-local m-1266 run kept one tanker ahead and showed mask192 at
`6/46/164`, but neither score nor elapsed time is authoritative on the contended
host. Fresh holdout SHA256
`0EF4FBFEA6850208765A92E5772849A08D52EC8A28544E8E9DBC7B55F850EA1D` remained
unopened. Resolve only with a BTC live run at 5000 ms.

The exact terminal day itself provides a causal discriminator. On m-1266, live
one-tanker day 10 served 18 while mask192 served 29 in the current exact
counterfactual. On m-1241, live one-tanker day 10 served 37 while the exact
two-tanker mask96 served 30. Both comparisons cover all six brands, so this is a
tier-3 late-capacity difference inside an official single-day score, not a
weighted aggregate or a fuel proxy.

`SCORE-ROLE-029`, parent `08771f1`, shortens coarse full-horizon work
to 75% instead of 85% and preserves the state/ledger at the start of its
final day. Best one-tanker and best two-tanker then split the interval to the
existing 92% role wall and re-solve that day with production-like columns,
harvest extensions and conditional exact orienteering. Two tankers may relax the
coarse daily gap only when lifetime ties and their executable terminal official
score is no worse in its first two components and lexicographically better. The
mechanism is inconclusive and absent from source. A loaded-local diagnostic
returned only `4/4/9` for the best one-tanker and `3/3/10` for mask192, but that
deadline-sensitive score cannot reject the candidate. Fresh holdout SHA256 is
`61BE95EAB2FCDA6886CF8EB68CC3009C42F39B55DE3FEDC3B320F8C993C8A6DB`.

Two explicit-advanced BTC runs used the frozen candidate binary SHA256
`ABC213A7F21A11C3C9CFD5AC94FB8B5CA77560470EB9F8F9062F0BF3255C81ED`.
`m-1278` exercised the two-tanker branch (mask192), but the HTTP client stopped
after sending day 7 with WinHTTP `12002`; only six action results were received,
all valid at `3121..3130 ms`, and days 8--10 were lost. The final `6/44/196`
rank4 result is therefore a lifecycle failure, not a role-score verdict. Replay
SHA256 is `4D538FD15E3545848191F1803B40BE32CBB4D5E5172AD5CACECFA89639236E4B`.
`m-1279` completed 10/10 valid acknowledgements and ranked first at `6/60/310`
over `6/60/220`, `6/59/201` and `6/55/198`; response total was `29190 ms` and
per-day maximum `3061 ms`. It selected the existing one-tanker mask1, so it
validates BTC runtime but does not attribute a gain to the new terminal probe.
Replay SHA256 is `79E964C81D16C11604C3EF8BF9B706C409CA46FCB2F6DD6B61BA9920390E56E1`.
`m-1280` completed 10/10 valid acknowledgements with two-tanker mask192 and ranked
first at `6/60/300`, response maximum `3131 ms`. Replay SHA256 is
`FAB35645B9FEBA89E4A2DC0DEF0C864B24F8172D2DBB5542E97875CF65E86F44`.
However, replay-role attribution showed all leading one- and two-tanker coarse
rollouts tied on lifetime/daily (`6/51` on m-1280, `6/60` on m-1278), so the
already-accepted SCORE-ROLE-023 equality fallback can select two tankers without
the terminal probe. The probe itself saw only `4/4` on m-1280 and did not recover
the production trajectory's `6/6`; m-1266 likewise returned one/double terminal
scores `4/4/9` and `3/3/10`. SCORE-ROLE-029 is rejected and absent from source:
re-solving only the last day from an already-diverged coarse trajectory does not
measure the multi-day state that creates the known m-1266 gain. Reopen only with
a trajectory-consistent multi-day finalist witness inside the same 5000 ms cap.

`PERF-DEADLINE-003`, parent `08771f1`, is accepted. BTC
`m-1278` proved that a single WinHTTP receive timeout can terminate the client
after an action is sent, discard three remaining days and turn a live lead into
rank4. The six earlier ACKs were valid at `3121..3130 ms`; this is a transport
lifecycle failure, not a solver overrun. The official BTC API page states that
`POST /actions` may be resent. The frozen mechanism therefore keeps the solver,
role selection, action body, comparator and 5000 ms cap unchanged; it gives each
action ACK a bounded request-level receive slice and resends the identical body
only for retryable WinHTTP timeout/resend errors while the authoritative day
deadline still permits another attempt. Ledger state advances exactly once and
only after an accepted ACK whose day equals `wireDay + 1`. Frozen holdout SHA256
is `AEF7B9C72F08FEAA2E79BE69CAFB6C16AC8D6FAC6942AD1A598C91DEFE680A6A`.

A controlled semantic probe first delayed the initial ACK beyond a `250 ms`
request slice and confirmed two byte-identical POST bodies, one applied ACK and
one retry telemetry event. This probe is not local performance evidence. Fresh
explicit-advanced BTC production match `m-1283` used hard difficulty, three bots,
10 days, 32x32, 100 steps/day, 5000 ms, eight agents, 12 spots, six brands and
fuel 1x. It completed 10/10 valid ACKs, reconciled 9/9 transitions and ranked
first at `6/60/312`; maximum server-reported response was `2958 ms` and maximum
solver time was `2925 ms`. No transport retry was needed. Replay SHA256 is
`E085ACA345C2361FBBD05142905FF64EC4341D27EB6AF2795A3C6A965CF6A173`.

Fresh BTC conformance match `m-1284` used the same explicit advanced class and a
temporary probe binary that discarded the first valid day-1 ACK, then resent the
exact frozen body. BTC accepted both submissions for day 1; the runtime applied
only the second ACK, completed 10/10 valid applied days, reconciled 9/9 transitions
and ranked first at `6/60/277`. Maximum server-reported response was `3060 ms` and
maximum solver time was `2848 ms`. Replay SHA256 is
`F0454045E278E7D696FBA365335CB41764971D0E5EC0587A2808DBDE9EEBAAA9`.
The temporary forced-resend flag was removed before the final production rebuild.
The final executable SHA256 is
`ACA63DCC68DA5339DF98A7D5BCC792AF168D97CF09F382861540A1341B4BF7E0`;
that binary replay-checks both matches to their exact `6/60/312` and `6/60/277`
summaries. The candidate therefore closes a catastrophic lifecycle downside while
the no-timeout production path preserves action/score semantics and the 5000 ms
cap.

BTC `m-1228` (hard, 3 bots, 32x32, 10 days, 100 steps/day, 8 agents,
12 spots, 6 brands, fuel 200, response 5000 ms) is the authoritative new
counterexample. SCORE-ROLE-016 selected mask `2`, but day 1 column generation
spent `7342 ms` in exact orienteering (`2111440` settled states, seven complete
patrol agents) against a `4353 ms` total solver profile, so the runtime skipped
submission. Day 2 later received a stale-day `E_STEP_OVERFLOW`; replay-check
shows the plan is exact-valid against the state it was built from, so the first
confirmed fault is the unbounded exact-orienteering stage/lifecycle deadline, not
route semantics. Final live result was rank 4 at `6/48/217` versus rank-1
`6/60/281`. Invalid/deadline behavior is a hard block: preserve SCORE-ROLE-016
as a frozen patch/binary, restore source to `6f84a06`, and fix the performance-only
deadline path before resuming role-score comparison.

`PERF-DEADLINE-001` is the only active implementation axis. Attribution on the
frozen parent path first showed that exact route enumeration accepted the search
deadline while team coordination/finalization could continue after it. A first
cooperative-cancellation candidate returned promptly but lost the exact bundle and
regressed `m-1228` day 1 from `6/36` to `6/16`, so that intermediate form was
rejected. Deeper telemetry then isolated two exact performance defects without
changing the search space: deadline polling used `settledStates`, so stale queue
entries could run without checking the clock, and the team feasibility DFS visited
`3,000,181` nodes even after the local incumbent had reached the proven absolute
upper bound over all positive-stock brands and `sum(min(stock, patrolCount))`.

The current candidate polls by processed queue/bucket entries, enumerates independent
high-fuel patrol starts on at most four workers, caches only exact hex distances,
and skips feasibility only when the incumbent equals that mathematical upper bound.
Team beam/local/pairwise work remains cooperative and retains only fully constructed
incumbents. On the frozen `m-1228` day-1 replay at the live-observed `4353 ms`
available window, candidate and parent are byte-identical for all eight agents and
both score `6/36`; candidate exact attribution was `187 ms` total (`137 ms`
enumeration, `50 ms` finalization), zero deadline overrun and `0` feasibility nodes,
with total solve `2729 ms`, leaving `1624 ms` for the `1600 ms` HTTP floor. Local
targeted equivalence/expired-deadline tests pass. BTC target-host `m-1235` then ran
the candidate on the preregistered explicit advanced configuration: hard, three
bots, 10 days, 32x32, 100 steps/day, 5000 ms/day, eight agents, 12 spots, six
brands and medium fuel 2x (`200`). All 10 submissions were HTTP 200 and valid,
with zero emergency, zero exact deadline overrun and a maximum response of
`2647 ms`. The independent replay validator agreed on every day and reconstructed
the final official score `6/60/337`; the candidate ranked first over bots at
`6/60/293`, `6/60/261` and `6/60/255`. Replay SHA256 is
`73F36180715F6E9ADF1E2FAB6199118461DABECC758689DA962D8B7361F879CD`.
This clears the fresh BTC runtime hard block for the medium-fuel lane, but does
not grant commit authority until the replay joins the frozen semantic equivalence
suite and the protected comparison is complete. No route cap, score comparator,
search width, role logic or simulator semantics changed.

`SCORE-ROLE-001` was rejected. BTC replay `m-1143` is a 32x32, 3-agent,
13-brand, 4-day map.
The live `6f84a06` candidate selected all patrols and finished `13/26/29`; after
day 2 its patrol fuel was `1/0/5`, limiting days 3-4 to `3/3` each. On the same
authoritative replay, the frozen binary with tanker mask `4`, a 5000 ms cap, the
exact simulator and the independent validator produced `13/29/29`. This is a
development counterexample, not a promotion result, because later replay traffic
contains the original team's footprint.

The earliest demonstrated mismatch remains pre-match role evaluation. `replay-roles`
ranks all-patrol first at `13/25/27` and mask `4` fourth at `13/20/21`, while the
day solver counterfactual reverses them. The role rollout uses a narrower portfolio
than production: in the timed path at most 4 columns and 6 targets per agent, no
production seed plan, shallow harvest extension, and 256 master combinations. The open
axis is to improve role-rollout fidelity within the same 5000 ms cap, not to route
by map identity or to prefer tankers heuristically. Replacing the narrow rollout
with a wider one was not monotonic: on `m-1138` it changed the selected mask from
`16` to all-patrol and exact replay score collapsed from `6/60/326` to `6/45/140`.
The wider search made coordinated tanker roles harder to solve before their shared
deadline, so its incomplete score cannot overwrite a stronger narrow-rollout witness.

`m-1138` is now a development counterexample. The previous six-replay holdout was
opened for role ranking during the rejected experiment and cannot be reused to tune
its successor. `SCORE-ROLE-002` preserved the development lanes and tied the first
six-replay exact holdout, but was rejected on the unchanged secondary holdout:
`m-0929` switched tanker mask `8` to `32` and fell from `6/60/367` to `6/60/357`.
Repeated role scans also changed the top mask on the same binary, confirming that
wall-clock cutoffs are influencing role rank. The next role candidate must be
operation-bounded or deterministic before score tuning continues.

A deterministic central-one-tanker policy was falsified before implementation.
It retained the known `m-1143` gain (`13/29/29`) and scored `6/60/338` on
`m-1138`, but on `m-0929` central mask `128` scored `6/60/362`, below parent
mask `8` at `6/60/367`. Therefore centrality may seed an operation-bounded beam
but cannot select the production role by itself.

`SCORE-ROLE-008` combined a complete fixed-operation role pass with additive
post-refuel harvest extensions. It completed the `m-1143` role gate in `544 ms`
without a deadline flag, but still ranked all-patrol `13/27/27` above mask `4`
`13/23/23` and mask `2` `13/21/21`. The candidate therefore failed the first
pre-registered development gate and was reverted without opening another map.
This rules out the unsupported claim that merely enabling post-refuel extension
inside a complete pass repairs tanker valuation. The next step is attribution of
the exact stage where a valid refuel-enabled witness disappears or loses rank,
not another parameter or portfolio-width change.

`SCORE-ROLE-009` instrumented that path without changing its intended ordering.
One full pass showed mask `4` with `14` eligible post-refuel sources, `3` generated,
`3` retained and `0` selected. When atomic companion retention was added, two runs
fail-closed to the day-1 structural fallback (`1616 ms` and `1565 ms`); the day-1
mask-4 trace was `2/1/1/1/1`, proving an extension and its tanker companion can
survive and be selected. The full pass is therefore not genuinely operation-bounded:
fixed combination caps still coexist with internal wall-clock deadlines. No later
score attribution from a partial/fallback pass is admissible.

`SCORE-ROLE-010` removed every internal wall-clock cutoff from the ranked role
pass. Two runs were identical (`465 ms`, `403 ms`): all-patrol `13/27/27`, mask
`4` `13/23/23`, mask `2` `13/21/21`. For mask `4`, `14` eligible sources produced
`3` extensions, all `3` survived patrol pruning, `2` retained their tanker
companion, but none was selected. In contrast, the fixed narrow day-1 probe had
selected its atomic extension. The measured successor gap is bounded-master
portfolio non-monotonicity: widening the per-agent portfolio can hide a complete
narrow witness even when its route columns remain present. Timing noise and raw
route absence are no longer admissible explanations for this counterexample.

`SCORE-ROLE-011` evaluated the complete narrow and wide fixed-operation
portfolios independently and advanced their exact lexicographic maximum on every
role/day. It still ranked all-patrol `13/27/27` above mask `4` `13/23/23` and mask
`2` `13/21/21` at `1946 ms`; mask `4` chose the narrow portfolio on one of four
days but received no full-horizon gain. The candidate was fully reverted. The
earliest quantified mismatch is now day 1 itself: the fixed role probe produces
only `4/4` for mask `4`, whereas the frozen production counterfactual produces
`7/8` on the same authoritative initial state. Therefore another portfolio merge,
post-refuel extension or bounded-master width change is not an admissible next
hypothesis. The open gap is production-quality day-1 seed fidelity.

`SCORE-ROLE-012` closed that day-1 gap without tuning after observation. Across
two stable runs (`698 ms`, `798 ms`), mask `4` produced the frozen production
day-1 score `7/7/8`. However, continuing from its exact terminal state with the
fixed cheap evaluator ended at only `12/21/22`, while the no-seed whole-horizon
floor retained all brands at `13/20/20`; all-patrol remained first at `13/24/27`.
The source was fully reverted. Production-quality day-1 route generation is no
longer the earliest open cause. The new counterexample is a certified tier-1
continuation failure from the improved terminal state.

`SCORE-ROLE-013` added a fixed `64`-rollout MacroMCTS candidate on later days
while lifetime brands remained missing. Two stable runs (`884 ms`, `835 ms`)
restored mask `4` to all `13` lifetime brands but reached only `13/22/24`, below
all-patrol `13/24/27`. Its exact daily sequence was `7/8, 5/5, 6/7, 4/4`, versus
the frozen production counterfactual `7/8, 6/6, 8/8, 6/6`. The lifetime-only
trigger stopped enhanced continuation after day 3, although tier 2 was still
open. This is the architecture's documented tier-2-witness failure in executable
form: completing lifetime coverage does not authorize a cheap continuation.

`SCORE-ROLE-014` kept the production-width fixed pass active on every day. It
completed in `4250 ms`, but mask `4` reached only `13/25/26` with daily sequence
`7/8, 5/5, 7/7, 6/6`, below the frozen `13/27/28`; mask `2` ranked first at
`13/27/27`. The candidate was rejected without repeat or another map. Increasing
the same operation caps is now prohibited. Existing production replay telemetry
shows the independent planner produced an exact-valid candidate on every day, but
does not expose its score or whether the final incumbent came from it. That stage
boundary is the next read-only attribution target.

## Invariants

- Official lexicographic score only; no weighted-sum promotion.
- Exact simulator and independent validator must agree; invalid rate remains zero.
- Performance changes preserve semantics.
- Logic changes must show a sufficiently large paired global advantage with bounded
  downside across protected official-budget strata; small sparse regressions are
  admissible, systematic or catastrophic regressions are not.
- Same fixture, role policy, traffic history and budget for every checkpoint pair.
- Solver and role-selection wall clock must not exceed 5000 ms on primary lanes.
- Evidence produced with an internal budget above 5000 ms has no promotion authority.
- Local output is a development filter only. BTC target-host is authoritative for
  latency, cutoff behavior, runtime safety and the final candidate verdict.
- A candidate cannot be promoted or committed while BTC connectivity/evidence is
  unavailable, even if all local checks pass.
- Commit only after a comprehensive global win against parent and relevant lane
  champions: zero invalid/emergency, no hard-cap violation, broadly distributed
  paired benefit whose magnitude exceeds bounded downside, and a passing BTC gate
  at the same 5000 ms cap. No fixed win ratio and no weighted tier aggregation.

## Protected lanes

1. Primary competition: outer server window `60000 ms`, internal hard cap `5000 ms`,
   fixed and deadline-bounded native roles, low/default/high fuel, 8x8 generated and
   BTC-like 32x32, horizons 4/5/10 days.
2. BTC preflight: hard cap `5000 ms`, protocol validity, runtime safety and score gate.
3. Degradation diagnostics: `500/1200/2500 ms`, never the sole production verdict.

## Next actions

1. Keep SCORE-QUEUE-048 frozen at canonical executable SHA256
   `CE37B6D06F5AD92CDB3921DCCA6CAE21A4D6E688431232A78A7E25DDEF2EE6CF`;
   do not rebuild, retune or reopen its local holdout.
2. Treat fresh advanced BTC `m-1796` as a passed target-host validity/lifecycle/
   hard-cap gate only. Do not use its rank 1 as quality evidence and do not rewrite
   `p99Calibrated=false` from one ten-day sample.
3. Join the official real-opponent queue with only the same frozen binary and
   internal 5000 ms cap. Record the authoritative match configuration first because
   the outer response window may vary by round.
4. Use real-opponent matches to seek counterexamples across opponent behavior and
   report official score, first differing tier and downside. A non-paired match
   cannot prove parent superiority; paired candidate-vs-parent evidence remains the
   promotion authority whenever an identical fixture can be obtained.
5. While the real-opponent queue is deferred, complete preregistered
   `ROBUST-QUEUE-052` without changing production source: prove its same-binary
   oracle on development, then open its structural holdout exactly once.
6. Close SCORE-QUEUE-048 accepted or rejected before opening another score axis.
   Commit only after broad global benefit, bounded downside, zero invalid/emergency,
   no hard-cap violation and all BTC/real-opponent gates pass.

The frozen `SCORE-ROLE-016` archive holdout was opened only after its 22-row
manifest was written and hashed as
`16F8E3A820D33BF42FA0791D64E125D635962C6E926298213AEDCD8B19C78312`.
The selector kept the parent role mask on 17/22 replay setups. Exact validated
counterfactuals on the five changed masks produced 3 wins and 2 losses, hence the
full role verdict is `3/17/2`. Lifetime was unchanged throughout. Daily-distinct
gains/losses were `+15/-10` (net `+5`), and servings gains/losses were
`+178/-30` (net `+148`). The exact changed-mask pairs were: `m-0854`
`6/60/308 -> 6/60/373`, `m-0865` `6/60/225 -> 6/60/205`, `m-0878`
`6/52/135 -> 6/60/240`, `m-1139` `12/23/23 -> 12/30/31`, and `m-1140`
`13/34/34 -> 13/24/24`. All counterfactuals passed simulator/independent-validator
agreement. This is a broadly positive but non-monotonic holdout, not commit
authority: the protected matrix and a final combined BTC lane still decide whether
the `m-1140` tier-2 loss is acceptable globally.

The combined protected screen tied parent on all 6/6 general fixed and 6/6 general
native seeds with zero invalid/emergency. The first fixed-role BTC-scale pass also
had zero invalid/emergency and preserved lifetime/daily, but showed tier-3 deltas
of `-3`, `-13` and `-7` for default/low/high. Observational day telemetry then
repeated the same fixtures without changing solver logic: default remained a small
negative (`-2`), low flipped from `-13` to `+14`, and high changed from `-7` to a
tie. All runs kept `6/60`; every day was deadline-limited, action hashes diverged
under timed search, and all outputs independently validated. This directly confirms
the documented BTC-scale local cutoff/load instability. The local tier-3 sample is
therefore inconclusive, not a production regression and not a promotion win.
`PERF-DEADLINE-001` remains provisional on its target-host `m-1235` pass, and the
next authoritative gate is a fresh explicit low-fuel BTC match with the frozen
role candidate stacked unchanged.

That gate passed on BTC `m-1241`, created with every field explicit: hard, three
bots, 10 days, 32x32, 100 steps/day, 5000 ms/day, eight agents, 12 spots, six
brands and low fuel 1x (`100`). The combined candidate finished rank 1 at
`6/60/340`; the three bots reached only `6/58/181`, `6/58/171` and `6/57/185`.
All 10 submissions were HTTP 200 and valid, max response was `3224 ms`, exact
deadline overrun and emergency were zero, and replay-check independently rebuilt
`6/60/340`. Replay SHA256 is
`88DB2191111AAFB039CCB5477CFB2624EF12354CA48E1857CA00EB1621918DFA`.
The live assignment was mask `32`; parent and candidate replay-role scans both
select mask `32`, so this match is authoritative combined runtime/safety evidence
but not causal evidence that the role patch improved this particular setup.

The full 60-map `competition-general-native` matrix produced `0/56/4`, with zero
invalid/emergency. The four losses were `310013` (`6/30/49 -> 6/29/47`, mask
`8 -> 1`), `310033` (`7/28/36 -> 7/28/33`, mask `8 -> 1`), `310044`
(`6/24/39 -> 6/24/37`, mask `1 -> 8`) and `310055` (`6/30/46 -> 6/30/42`,
mask `4 -> 8`). A runtime-path audit on 2026-08-01 found that this lane used
`role_mode=exhaustive`, which calls `select_roles(3)`, while SCORE-ROLE-016 changes
only `select_roles_until(...)`. The compared binary also included PERF-DEADLINE-001.
Therefore these four losses are combined-build/timed-day evidence, not causal
evidence against the wide timed-role mechanism. The lane is retained as an
exhaustive-role control and cannot accept or reject SCORE-ROLE-016/017/018.

`SCORE-ROLE-017` kept the frozen
`SCORE-ROLE-016` mechanism unchanged when `spot_count > 8`, because only then can
the parent narrow rollout's `maximumTargetSpots = 8` truncate configured targets.
When `spot_count <= 8`, it skips the duplicate wide pass entirely and therefore
retains the parent role path. The dispatcher is based on an observable semantic
capacity boundary, not seed, match id or family. The 60-map native matrix and the
opened 22-replay archive are development evidence only; a new generated
default/low/high BTC-scale native-role holdout must be frozen before testing.

The first independent guard screen exposed a deeper timing confound: on general
seed `320003`, base scored `6/30/39` with mask `4`, PERF-only scored `6/30/34`
with mask `8`, and the guarded role build again scored `6/30/39` with mask `4`.
Because `spot_count <= 8`, PERF-only and the guarded role build execute the same
narrow role logic; their different result is compile/layout wall-clock sensitivity,
not evidence that the guard itself fixes ranking. `SCORE-ROLE-017` is therefore
inconclusive and its opened screen cannot validate a successor.

`SCORE-ROLE-018` is now the only active role candidate. It enforces global
additivity in two phases: first every parent narrow beam member receives the
original shared rollout window ending at 85% of the role budget; only after all
narrow members finish may production-width rollouts consume genuine unused slack
remaining before that same 85% boundary. Wide results can replace only their
same-assignment narrow result by the official comparator. The `spot_count > 8`
semantic guard remains. The prewarm deadline stays at 92%, and prewarm still
retains at least the 85-92% opportunity. However, parent prewarm can begin before
85% when narrow work finishes early, so wide work may replace some of that earlier
cache-warming headroom. This is an explicit performance trade-off requiring
target-host telemetry, not an equivalent-semantics claim. Wide work cannot consume
time required to construct the parent narrow beam. A completely new generated
deadline-role holdout is frozen
before score; the earlier exhaustive manifest was corrected before opening because
it did not execute the candidate runtime path. The corrected manifest hash is
`A4688DA19C5B6A6611125DF7C3E13659C5EA5727C7FDCE2431CEA9D996590726`.

The first corrected deadline-role general screen still compared two separately
compiled binaries. Although `spot_count <= 8` made the wide stage unreachable,
role masks changed on seeds `330001` and `330004`; the apparent `330004` serving
gain is therefore compile-layout/wall-clock noise and has no candidate authority.
No BTC-like seed from that manifest was opened. A research-only same-binary switch
now disables only the wide slack stage while preserving identical code layout and
the PERF parent. The replacement causal manifest was frozen before score at
`research/holdouts/SCORE-ROLE-018-CAUSAL.csv`, SHA256
`2626EED2A950B53CBF7915DFD9316695D5EA0C0195CAD4DCF6171A4334573F15`.

## Reopen conditions

- Short-deadline quality: only if BTC publishes such a deadline or live telemetry
  demonstrates an equivalent authoritative window.
- High fuel, roles, ALNS, viability or master: only after a new official-budget
  counterexample identifies that subsystem as the earliest score-limiting cause.

## Current BTC role counterexample and SCORE-ROLE-020

Fresh explicit-advanced BTC match `m-1252` used hard difficulty, three bots,
10 days, a 32x32 map, 100 steps/day, 5000 ms response, eight agents, 12 spots,
six brands and low fuel 1x (`100`). The SCORE-ROLE-018 working binary returned
mask `0` (eight patrols) after about 4266 ms of role selection. All 10 day
submissions were HTTP 200 and exact-valid with no emergency, but the team finished
rank 4 at `6/52/138`; the bots, each using one tanker, finished `6/60/182`,
`6/60/186` and `6/60/201`. Replay SHA256 is
`B443AB6C3AC0A73EE6FB623E559684AF0A590CE1C2B8DCA3FD6F5ABCA45B87A4`.

This is an attributed role failure, not a day-planner or protocol failure. The
same frozen setup replayed with exact validation under the current binary reaches
`6/60/315` with central one-tanker mask `64` and `6/60/284` with one-tanker mask
`4`, versus the live all-patrol `6/52/138`. Offline role replay on the same binary
also ranks a one-tanker assignment first, proving the live mask `0` came from
cutoff/load-dependent incomplete evidence rather than a stable score preference.
The full-horizon timed loop allocates slices sequentially; a timed-out assignment
retains its one-day probe score, so the final comparator can mix evidence depths.

`SCORE-ROLE-019` removed the SCORE-ROLE-018 wide-rollout mechanism and initially
treated `rolloutValid` as complete evidence. The first six generated general
screens showed that this measured plan validity, not equal-depth search evidence.
Before opening any BTC-like seed, attribution separated `rolloutValid` from a new
`rolloutComplete` flag that remains true only when the independent planner, column
generation and master all finish without hitting their internal deadlines. This
changes the candidate mechanism, so the old SCORE-ROLE-019 manifest cannot be
used as a clean promotion holdout.

The corrected mechanism is `SCORE-ROLE-020`. Only when any narrow-beam assignment
lacks deadline-complete evidence and the public fuel/day-step ratio is low does it
prohibit all-patrol from winning the cutoff, promoting the best already-enumerated
exactly-one-tanker member by the existing official role comparator. Default/high
fuel paths are controls. The unit gate directly verifies incomplete-low-fuel,
complete-evidence and default-fuel branches. A research-only same-binary switch
disables the fallback and must be removed before any promotion.

The now-development-only BTC-like low-fuel seeds `760000..760003` produced a
conservative causal result of 1 win, 3 ties and 0 losses: on seed `760001`, control
selected all patrols and scored `6/60/160`, while SCORE-ROLE-020 selected mask 32
and scored `6/60/422`, a tier-3 gain of 262 with lifetime/daily preserved. Seeds
760000 and 760002 tied exactly; seed 760003 retained the same mask and its local
tier-3 delta is not causal authority. All runs had zero invalid/emergency.

Fresh explicit-advanced BTC match `m-1255` then used the same hard/3-bot/10-day/
32x32/100-step/5000-ms/8-agent/12-spot/6-brand/low-fuel configuration. Candidate
selected mask 64 in 3911 ms, received 10/10 HTTP 200 valid acknowledgements, had
zero emergency and exact deadline overrun, and the independent replay validator
agreed on all 10 days. It ranked first at `6/60/247` over `6/60/222`, `6/60/208`
and `6/58/202`; maximum day solver time was 2564 ms. Replay SHA256 is
`E3A467251223CE545680DE2731C336805FA6A9788A8CB48D1F71A425AAADC3B0`.

This is strong development and BTC target-host evidence, but not promotion proof
because the mechanism was refined after the old manifest opened. The replacement
holdout was frozen before any new score at `research/holdouts/SCORE-ROLE-020.csv`,
SHA256 `12193CD454ACBF9EA5248C9CBB1A24DE46FA0F67BD280519929099834B4081D8`.

The SCORE-ROLE-020 frozen screen produced no causal regression: six general,
four BTC-low, two BTC-default and two BTC-high fixtures all tied once changes with
the same selected role were separated from local day-planner cutoff noise. Every
run kept zero invalid/emergency. General seed `360001` and default seed `771001`
were repeated without code changes; candidate/control masks and scores crossed
when run order changed, proving the apparent tier-2 deltas were timed-selector
nondeterminism rather than fallback effects. On BTC-low, all four candidate/control
pairs selected the same one-tanker masks; on default/high, the low-fuel guard was
mathematically disabled. SCORE-ROLE-020 is therefore a safe scoped mechanism with
strong development/live benefit, but it does not address the broader instability
and remains uncommitted.

The new counterexample is default-fuel, 10-day seed `771001`: with the fallback
disabled by construction, repeated runs oscillated between all-patrol scores
`6/56/199` and `6/50/209` and one-tanker scores `6/60/322` and `6/60/320`.
Candidate and control exchanged those outcomes solely with run order. A 10-day
high-fuel control also changed daily score by three with the same all-patrol mask,
confirming local solver noise, but the default role flip exposes a real selection
hazard: unequal-depth timed evidence can lose tier 2 outside the original low-fuel
guard.

`SCORE-ROLE-021` replaces the fuel predicate with the public horizon boundary:
short protected horizons of 4/5 days keep the parent selector, while longer
horizons with any incomplete role evidence fail closed to the best existing
one-tanker assignment. This is not a new search or seed-tuned route; it is an
objective dispatcher over day count and evidence completeness. Its fresh manifest
was frozen before code or score at `research/holdouts/SCORE-ROLE-021.csv`, SHA256
`31D9677FC73E4189CD2FC6604979427E349808E776827020359430CF7F229325`.

The frozen SCORE-ROLE-021 screen completed at 3 wins, 15 causal ties and zero
losses across six short-horizon general fixtures and four each BTC-like low,
default and high fuel. All runs had zero invalid/emergency. General fixtures were
six exact same-mask/score ties. Low and default fixtures selected the same tanker
in each pair and preserved `6/60`. High-fuel produced three causal gains and one
tie: seed `782001` improved all-patrol `6/58/360` to mask32 `6/60/449`; seed
`782002` improved all-patrol `6/57/326` to mask64 `6/60/435`; seed `782003`
preserved `6/60` and improved servings `260` to `327`; seed `782000` tied
`6/60/508`. A reversed-order reproduction on `782002` again selected all-patrol
for control at `6/52/333` and mask64 for candidate at `6/60/435`.

Fresh explicit-advanced high-fuel BTC match `m-1257` then used hard difficulty,
three bots, 10 days, 32x32, 100 steps/day, 5000 ms, eight agents, 12 spots,
six brands and fuel 3x (`300`). SCORE-ROLE-021 selected mask8 in 4423 ms and all
submitted plans were exact-valid, but the lifecycle sent only six of ten days.
Days 4, 6, 7 and 8 were replaced by server WAIT with reason
`insufficient-authoritative-day-window`; final result was rank4 `6/52/254` versus
three bots at `6/60/311..326`. Replay SHA256 is
`FBD04D7F2170FF22D4AB19B7156A77C1237D069F2ED5F7FFE683DADFA0E90CF3`.

This is not a role-score refutation. The decision engine produced exact-valid
one-tanker plans, but the final HTTP gate required 1500 ms remaining even though
the solver already reserved 1600 ms network time. The four skipped decisions had
1129, 1352, 989 and 1498 ms of their authoritative local window remaining. A
read-only scan paired `actions` with `action_result` over 366 BTC replay files and
794 successful POSTs: p50 13 ms, p95 81 ms, p99 594 ms, maximum 4773 ms; eight
samples exceeded 500 ms and four exceeded 1000 ms. On m-1257 the six observed
POST RTTs were 8-12 ms except one 443 ms sample.

`PERF-DEADLINE-002` therefore keeps the 1600 ms solver network reserve and all
search/actions unchanged, but lowers only the final go/no-go submission floor to
800 ms, equal to archive p99 plus 206 ms margin. `post_until_deadline` still uses
the authoritative server deadline and the exact WAIT fallback remains below the
floor. This accepts a quantified rare-tail trade-off instead of guaranteeing four
known misses. The frozen evidence/target manifest is
`research/holdouts/PERF-DEADLINE-002.csv`, SHA256
`2A52D220E342B2780063DE65D3F0DE4FE4DF75B931F3B508E4C355A403144A57`.

The preregistered fresh target `m-1258` used the same explicit high-fuel advanced
configuration as `m-1257`. The candidate selected one-tanker mask16 in 4445 ms,
submitted and received HTTP 200 for all ten days, and finished rank1 at
`6/60/415` versus bots at `6/60/363`, `6/60/360` and `6/60/351`. There were zero
submission skips, server WAITs, emergencies and exact deadline overruns; maximum
solver time was 3130 ms and observed POST RTT was 7-49 ms. The independent replay
validator accepted every submitted plan. Replay SHA256 is
`6BA3E14F97259CB0259DEB4F014C6FF86DBC28EE71ED6BC58143052C2C499D1B`.
This clears the `m-1257` lifecycle blocker and supplies target-host evidence for
the composed role/runtime candidate. Seven of nine replay state transitions were
byte-exact; the remaining two were conservative authoritative-state reconciliations
where the server reported more patrol fuel after tanker co-location than the local
simulator predicted. They caused no invalid action or score loss and are recorded
as the next read-only semantic-attribution gap, not silently treated as equivalence.
