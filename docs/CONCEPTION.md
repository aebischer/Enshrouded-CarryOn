# Original design decisions

The initial goal was to move chests with their inventory, then existing world objects that otherwise only support destruction. The first milestone targeted one ordinary chest in a copied solo world, followed by magical variants and other families.

Core invariants: preserve logical identity, quantities, properties and links; do not duplicate, destroy or create loot; refuse invalid placement without partial mutation; validate reach, permissions, collision, support and claims; handle interruption/death/world closure without loss; save/reload exactly one object. Quest/progression objects require separate analysis.

The chosen interaction uses equipped pincers, a separate transport mode, crosshair targeting and one native-style highlight. Hover alone does not mutate the object or inventory. Server-authoritative multiplayer remains future work, not an ABI 1 feature.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/CONCEPTION.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.
