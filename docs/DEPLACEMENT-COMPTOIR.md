# Decor counter — initial integration 0.1.30

This records the first decor integration. The initial single-model allowlist
and same-claim restriction were later generalized; see the current README.

Model: `Prop_Furniture_Comfort1_Bar_01_Wood`, archive GUID
`423c01619630d241b98a591808d9ea21`, dynamic Prop family 8. Scene IDs with the
high bit set are excluded. Native placement ItemInfo:
`e3a56abbb97f3946ad1fbd266c5ccace`, ID `0xf60b84c2`, debug name
`Prop_Decoration_Comfort2_T1_Tavern_Counter`. Its placedEntity references this
model exactly. No synthetic ItemInfo, recipe or inventory item is needed.
The resource stays in its registry because internal references are relative.

## Execution path

- Native targeting prioritizes chest slot 2, then decor slots 0/4, retaining
  model/family checks and the three-metre reach limit.
- Server pickup validates entity, claim and collisions. InventorySetup and
  UsedItem are required for chests, not for the counter.
- The initial attachment policy accepted state 2 only, without parent,
  destruction, added gravity or forced detachment. Later attachment support
  is described in [floor placement](POSE-AU-SOL.md).
- Preview uses the native counter descriptor, verifies its client/server model
  link and retains native placement solver and permission checks.
- Commit moves the existing entity through native mutable CurrentTransform
  access and collision movement, using the common chest transaction.
- Client synchronization verifies model/placement identity, waits for replication,
  then updates three render poses and collisions. Missing UsedItem must not
  suppress this repair.
- ComfortProvider is preserved and checked for changes during the transaction.
  The engine updates spatial cells and recalculates comfort from the current
  position; no artificial player bonus is applied.
- Native Prop persistence is used: no sidecar save, destruction, duplicate or
  scene-bit mutation. Research evidence is in [decor research](RECHERCHE-DECORS.md).

Offline tests include real ItemInfo identity/model/bounds, target slots, rejected
families and scene entities, no inventory/UsedItem, invalid attachment, changing
comfort, permission/collision refusals, rotation, exactly-once commit and visual
synchronization. Native doubles do not execute the actual save system.

Manual checks: pickup/cancel, rotated placement, collision and image agreement,
comfort at the new location and away from the old one, then save/reload and a
second move. The engine chooses the best provider per category, so another
table may hide the difference; the observed category range is 15 metres.
Include ordinary/magical chests and outside-claim refusal.

## 0.1.31: synchronization after quantized rotation

Intermittent image retention in 0.1.30 depended on rotation. A read-only sample
from entity 2771 on November 16 showed the correct server and replicated client
transform, but stale RenderTransform.

Requested/server quaternion: `(0, 0.5000692606, 0, 0.8659854531)`.
Received quaternion: `(0, 0.4995411932, 0, 0.8662901521)`.
Position difference was 0.488 mm, inside the existing tolerance. Rotation exceeded
the old per-component 0.0001 threshold, so synchronization waited indefinitely
for equality that network quantization would never provide.

Comparison now uses normalized angular distance up to 0.25 degrees, treating
q and -q as equivalent. This is an acknowledgement tolerance, not a permission
change or a complete description of the network codec. Identity, position,
scale and validity checks remain. Render/client collision use the received
pose; synchronization does not overwrite the server transform.

Regression tests use the exact sample, reject stale/different rotation and
changed scale/position, retain pending work before replication and complete it
after rounded replication for both chests and counters. Local evidence:
`work/counter-sync-live.json`, reader `work/read-counter-sync.py`.
