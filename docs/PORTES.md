# Usable objects: doors — introduced in 0.1.40

The per-world `move_interactive` setting is independent of Decor, Chests and
Debug. All four labels are translated into 15 languages. Doors were the first
interactive family; wells were added later in 0.1.43.

## Observed native contract

Observed door: `Prop_Door_Small_Castle`, template
`550d7a506569c348bc33efdf9fb894d4`, server entity 1407. Its physical leaf was
entity 3828, `Door_Collision_Small_Wooden`, template
`8b170735e07e6044b0f3be80a837adba`. The root is **Switch (10)**, not Prop (8),
verified through reflection and the native resource.

Components: Collider (24 bytes), ClientCollider (16), ServerToggleState,
InteractionToggle, ServerAnimationGraphSlotComponent and DynamicSubEntities
(16 IDs). The child has ServerSlotAttachment (112 bytes), parent identity at
+44, and its own Collider. Native sub-entity lifetime owns its DoNotSave data;
the mod neither creates nor destroys the child.

Classification reads template components, not a name list. It requires Switch,
InteractionToggle, ServerAnimationGraphSlotComponent, SubEntities and Collider,
without DoNotSave. Unsupported interactive objects cannot bypass this gate
through the Decor setting.

## Movement

The server snapshot checks opening state, children, identities, collision handles
and local attachments. Changes during preview invalidate the transaction. Root
colliders must use one resource, setup 0 and the root slot; other arrangements
are refused.

Commit preserves the root and state, moves transform/collision through the common
native manager, checks static/dynamic record type and respects isPathBlocker for
path-grid marking. Client render/collision follows the replicated transform.

Children remain attached. `server_slot_attachment` / `server_slot_attachment_write`
(`0x349aa0` / `0x34a120`) update transforms; `move_colliders` (`0xa1d0c0`) applies
animated poses through the same native move `0xa1fdc0`. No independent leaf
translation or animation-graph mutation is injected.

## Persistence and tests

Dispatch `0xe187b0`, family 10, calls `0xdc24d0` then `0xdc5400`. The full transform
is copied at `0xdc5690–0xdc56b7`; native states follow in the same Switch record.
Door identity is preserved.

Offline tests use the actual castle-door resource: classification, placement
descriptor, independent settings, open/closed state, attachment/toggle preservation,
missing/changed child refusal, interaction during preview, root collision/render
and existing regressions. They do not execute the native animation engine.

Manual checks: move both open and closed doors, rotate, interact from both sides,
verify leaf collision and no collision at the old location; save/reload, find
one functional door and move it again. Test Usable objects off with Decor on,
and vice versa. Claim limits still apply.

User feedback on November 17 confirmed door movement. Persistence after reloading
was not separately confirmed in that report. A lock-prompt/opening discrepancy
was also reproduced without CarryOn and left for investigation by the game developers.
