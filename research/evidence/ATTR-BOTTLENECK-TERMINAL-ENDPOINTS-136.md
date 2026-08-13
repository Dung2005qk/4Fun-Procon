# ATTR-BOTTLENECK-TERMINAL-ENDPOINTS-136

Date: 2026-08-13  
Parent: `5dccb0f`  
Consumed fixture: 135 fuel-bottleneck/low seed 2700400, day 1

The exact plan is absent from the normal width16 route set (`101`) but exists at
width32/64 (`111`). Adding only its two missing independent direct three-serving
routes makes the unchanged master retain exact plan and outcome. One exact route
is current-endpoint-like inside its served terminal; the other is the maximum-
fuel endpoint. Both are resource-undominated and use no escort/refuel/bundle.

This research-only capability probe retains safe wait and selects the union of
at most two deterministic routes per public served terminal: official current
contribution before fuel, and fuel before official current contribution. It
uses no oracle identity or future information. Passing requires route mask 111
and unchanged-master exact/equivalent outcome. Even a pass does not prove the
routes exist before production width16 pruning.

Result: rejected. The union produced route mask `011`, widths `14|13|1`, and
the unchanged master retained neither exact plan nor equivalent outcome. The
exact terminal/fuel/spot state class was present once for agent 1 and the wait
agent, but absent for agent 0. No third endpoint or rank tuning is permitted.
