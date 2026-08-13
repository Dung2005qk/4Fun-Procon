# SCORE-TANKER-POST-PARENT-142

Date: 2026-08-13  
Production parent: `5dccb0f`  
Frozen manifest: `research/holdouts/SCORE-TANKER-POST-PARENT-142.csv`  
SHA256: `6E2D858D719DAABF128357FF0EDCA03642AC26C43236F984489CB2F331F32DBB`

141 had strong scores but still exposed mobile hubs to MacroMcts, changing its
root action count and deterministic RNG sampling. Attribution on the consumed
138 witness showed EventConflict alone reaches the exact score, while mobile
MCTS and backward search are unnecessary.

142 keeps all parent methods static-only. It runs the byte-equivalent parent
EventConflict phase first. Only if that phase finishes before its deadline may
a bounded mobile-only EventConflict phase run from the already protected parent
incumbent; the existing lexicographic comparator is the sole replacement rule.
Thus no parent computation or incumbent can be erased by mobile work.
