# ATTR-ROUTE-036 agent-0 frontier membership

Date: 2026-08-09
Parent: `afcd2da`
Verdict: accepted read-only attribution

The probe reconstructed `m-1285` day 10 from the frozen replay and called the
canonical parent `enumerate_anytime_resource_routes` for agent 0 with exactly
the production semantic parameters for this state:

- minimum spots: 5;
- maximum retained routes: 32;
- maximum settled states: 1,250,000;
- preferred brands: 0 because lifetime coverage is complete;
- wall deadline: absent, so elapsed local time cannot alter the fixed-work
  result and is not measured or interpreted.

The result is supported but incomplete at exactly 1,250,000 settled states. It
contains 32 maximal routes, zero supplemental routes and zero terminal
variants. The exact-bundle mask `0x999` is retained at index 1. The canonical
35-serving mask `0x9D8` is absent from every returned frontier.

The 32 retained routes contain 24 six-spot masks followed by eight five-spot
masks. Therefore `0x9D8`, which has six spots, could not have been discovered
and then evicted by the existing legacy rank: every discovered six-spot mask
strictly outranks every retained five-spot mask.

Independent action-path accounting also rules out an unsupported refuel route.
The `0x9D8` witness consumes 96 movement steps, then waits four steps, and its
raw movement fuel is 73 from initial fuel 100. The `0x999` route uses raw fuel
63. Thus `0x9D8` is a feasible single-agent resource route; it is missing
because the step/fuel-ordered queue exhausts the 1,250,000-state operation cap
before discovering that deeper mask, not because of stock coordination,
retention eviction or tanker-dependent feasibility.

No production file was changed and no cap/rank variant was run. The next
admissible question is where the existing canonical pipeline obtained the
exact-valid `0x9D8` action path, so a general capability can reuse existing
evidence rather than increase the enumerator cap against one replay.
