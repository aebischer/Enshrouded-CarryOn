# Beds and seats — 0.1.55-dev

The existing **Usable objects** setting now enables beds and
seats, including supported double beds and multi-seat benches, independently of Decor and Chests. No new setting or name allowlist is
introduced. Native sit (`0x2804c2f7`) and sleep (`0xc2ee9b13`) interaction
sequences identify the contract, together with InteractionAttachment, model
and static client/server collision components. Template resources are read
once and cached by GUID; transient read failures retry with throttling.

## Runtime preservation

The common transport path moves the same entity, preserving its interaction,
comfort, and NpcBed.associatedNpc components. There is no destroy/spawn or
inventory conversion. The native npc_bed and npc_active_target programs read
CurrentTransform, which the existing tracked transform update modifies.
Runtime verification after placement and save/reload remains an in-game test.

Before pickup and throughout preview/commit, reject missing required state,
native interaction locks, a current NPC user, or the last interaction guest
still linked to the object. A historical guest who has detached is allowed.
Changes to the interaction timestamps/offset, accepted interaction or NPC bed
assignment during a pending move cancel it before mutation. No actor/NPC
attachment is cleared to force a move. The assignment is observed and retained,
never restored over a newer game value.

## Boundaries

Compound furniture is supported when every child is an unconditional native
sit/sleep helper, with a validated owner/despawn link, static collision and
an authored local transform. The root can provide a place itself (three-place
bench), or only hold its children (double bed). Up to 16 helper slots are
validated; the implementation does not assume that all furniture has two places.
Attached, conditional, world-offset, animated or storage-bearing helpers remain
unsupported. Independent DoNotSave objects remain excluded.

All places are checked for occupancy before pickup and again at commit. Every
collision target, destination and writable transform is resolved before any
position changes. The same root and helpers are moved; no replacement entities
are created. Invisible helpers need client RenderTransform/collision updates
without a ModelComponent. Only validated linked helpers use this path.

Native DoNotSave helpers are recreated by the root's SubEntities definition on
reload. Movement uses these same local offsets and rotations. Root comfort,
StoreAssociatedNpcIds and each child's NpcBed assignment remain untouched.
Nonmatching live helper poses are refused rather than saved inconsistently.

## Verification

On 2026-11-22, the tester confirmed that the installed 0.1.55 build works in-game.
This confirmation is separate from the offline tests below and does not certify
every furniture variant or multiplayer scenario.

- Real native desert bed and Cairn chair classification, resource-read retry,
  and negative cases for unrelated interactions and unsupported structures.
- Native adapter tests: independent setting, locked/occupied state at pickup,
  occupancy/interaction/assignment changes during preview, vacated seats,
  collision/render synchronization and unchanged comfort/assignment.
- Complete local corpus: 6,282 Prop resources, unchanged 5,769 accepted floor
  descriptors; 45 seat and 18 bed templates pass the single-entity contract and
  geometry checks (these counts include base/test resources).
- Real desert bench/double-bed templates and invisible helper contracts; captured
  server positions agree with the authored local-transform calculation.
- Compound adapter tests cover translation and 90-degree rotation, every-child
  preflight, occupancy, invalid/duplicate links, failed writable access without
  partial movement, invisible client synchronization and preserved assignments.
- Existing chest, decor, door, well, teleportation, interruption, model,
  translation and package checks remain enabled.

For manual verification, move a double bed and a three-place bench, use every
place at the new location,
check their comfort and reload the world. Also check an assigned but unoccupied
bed and confirm its NPC assignment. These checks are not claimed completed by
the offline suite. Dedicated-server/multiplayer support is unchanged.

Local proprietary fixtures are generated with tools/extract-prop-fixtures.py
and are never included in the package or repository.
