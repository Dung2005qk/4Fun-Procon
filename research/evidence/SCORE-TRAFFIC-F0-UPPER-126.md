# SCORE-TRAFFIC-F0-UPPER-126

Parent: `5dccb0f`

The fresh development split contained 18 fixtures across six traffic families
and low/default/high fuel. Source-frozen candidate-versus-parent A/B/B/A at the
5000-ms internal budget was `0/18/0`, invalid 0, identically in both orders.

Candidate telemetry showed the all-or-nothing road upper sweep attempted and
completed on all 72 decision-days, with zero fallback. The mechanism therefore
ran rather than silently preserving parent selection. Seventeen plan hashes
were identical. High/traffic-stock seed2420400 changed reproducibly
(`b8d0dd5b2b776b6d` candidate versus `bc1f18e3454e873c` parent), but both final
scores were `6/24/24`.

The unit suite passed before the score gate. Verdict: rejected for no development
gain. Holdout remains sealed. All production/test/telemetry candidate source is
restored to `5dccb0f`. Do not tune the upper order, F0 slot count or subwindow on
this split; retaining an upper challenger is not sufficient evidence that W1 can
convert it into a stronger closed-loop result.
