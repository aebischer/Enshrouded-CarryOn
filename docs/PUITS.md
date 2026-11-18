# Water wells — introduced in 0.1.43

Usable objects enables functional wells independently of Decor and Chests.
Production classification uses no model/name allowlist.

## Native contract

Read-only observation on November 17, 2026: entity 1479,
`Prop_NPC_Water_Well_c`, template `abc315119baca14c9a4bf92d9bfa04c2`.
Family 13; Fuel (16 bytes), DynamicFuelSetup (4), NetworkFuel (4), server
StaticCollider and client StaticClientCollider (16 bytes each).
Fuel contains an attribute header followed by values at +8/+12; do not treat
it as a plain counter or restore its initial value.

Classification requires Fuel, FuelSetup, FuelCraftingStock, NetworkFuel, both
static colliders and InteractionOfferRuntime with native drawing sequence
`0xf5024957`. DoNotSave, sub-entities and animated collisions are rejected.
Eleven current appearances (village, desert, swamp and craftable wells) share
this contract. Tests use GUIDs to find real fixtures; those GUIDs do not filter
production movement.

## Transaction

The common path creates a preview from native placement data or model collisions.
It retains claim bounds, native validation, rotation, collision movement and
render synchronization. Snapshots distinguish doors and wells: only door leaves
use the dynamic-collision path. Wells keep their entity and components.

Water reserves and refill are never restored from the snapshot: natural refill
during preview must survive commit. Missing Fuel/refill, an NPC owner
(DynamicActiveNpcTarget +24), or an associated animal (DynamicAnimalTarget +4)
refuses pickup or invalidates placement. Native npc_active_target and animal_target
systems read CurrentTransform, marked through the usual native manager.

## Persistence and validation

Dispatch `0xe187b0`, family 13, reaches `0xdc3160` then `0xdc5d60`. The writer
copies CurrentTransform at `0xdc5f59–0xdc5f7b` and serializes Fuel/MaxFuel
attributes at `0xdc60b0–0xdc60be`. Movement preserves identity and marks the
native transform; it does not spawn another well.

Offline tests cover eleven real resources, placement geometry, unrelated or
nonpersistent interactions, independent settings, reserves changed during preview,
NPC/animal users, collision/render and native placement hints. Chest/decor/door
regressions remain enabled. These tests do not execute native save cycles.

Manual checks include moving, drawing water, collision at the new location,
absence at the old location, save/reload, another appearance and disabling
Usable objects. Alain confirmed movement, operation and persistence after
save/reload on November 17, 2026.
