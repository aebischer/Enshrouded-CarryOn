# Decor without native placement ItemInfo

First model: `Prop_Destructible_Villageprops_bottles_12`, GUID
`ee386f839f792f4fa64f4a31593a2a1a`. Its native collider is a 30 × 66 × 30 cm
box centred 33 cm above the pivot. No ItemInfo in the inspected catalogue
references this model.

## Shared adapter

The native catalogue is checked first. For eligible dynamic Props without a
descriptor, `floor_placement.h` builds a complete private descriptor from the
counter's native floor-placement profile (`e3a56abbb97f3946ad1fbd266c5ccace`).
This supplies controls and floor rules; dimensions/model are replaced by the
target's. Copy collision arrays with their children and relative references:
a header-only copy would be invalid. Ingredients and item consumption are disabled.

Placement/alignment bounds include boxes, spheres, capsules, cylinders and tapered
capsules. Native reflection verifies fields; an independent parser verifies the
Y axis and full capsule segment length. Spheres are rotation-invariant; capsules
and cylinders use their support functions. Native validation retains the original
collision shapes.

Signed resource dimensions are converted to absolute values for the envelope.
Empty collision resources are ignored if others provide geometry. No shapes,
invalid data or unknown shapes cause refusal. DoNotSave blocks fallback placement.
Enabling Decor cannot make a nonpersistent entity persistent or turn a collisionless
assembly into a single movable object.

The cache owns published allocations for the process lifetime. Models share the
profile's numeric ID but resolve through their own GUIDs on client/server.
No inventory item is registered. Blobs survive world changes because native input
callbacks may retain their addresses between frames.

## Movement and persistence

Preview, native destination validation and commit use the common chest/counter
path. Existing entities and components—including comfort—are preserved, with
collision/render synchronization. Initial same-claim support was later extended
to authorized cross-claim transport; see [teleportation](TELEPORTATION.md).

Dynamic family-8 Props use the native serializer for template identity and full
CurrentTransform. No permanent copy is spawned and the original is not deleted.
Static instances encoded in the scene remain excluded.

Tests use local bottle fixtures, real and rotated bounds, relative offsets,
malformed geometry, shared caches, preview/validation/commit, native refusals,
identity changes, render/collision and authoritative transform preservation.
Manually verify floor contact, rotation, cancel, claim refusal and exactly one
instance at the saved/reloaded destination, followed by another move.

## 0.1.36: stable detached bottles

Server sample from bottle 2853 on November 16:
AttachToSurface `ffffffffffffff7f0000000001000000`.
Native states: 0 Uninitialized, 1 Detached, 2 Attached. Requiring Attached allowed
highlighting but blocked pickup.

Stable Detached is now accepted with searchStartTime = INT64_MAX, no parent,
destruction, added gravity or forced detachment. The state is preserved rather
than converted. Compare the full 16-byte attachment snapshot during preview and
before commit; any transition invalidates the transaction. Tests use the exact
observed bytes and exercise refusal conditions.

## 0.1.37: generalized geometry

Of 6,282 Prop templates, boxes appear in 3,815 models, spheres in 1,611,
capsules in 1,207 and cylinders in 2. Counts overlap for mixed shapes.
No per-name exception is needed. Tapered capsules are decoded from reflection
but do not occur in this corpus.

The corpus builds 5,769 descriptors. Controlled refusals: 170 DoNotSave templates,
240 without geometry and 103 without collider components. Numeric tests cover
rotation, signed dimensions, asymmetric shapes, native variants and truncated
input. Descriptor construction does not certify actual movement of every model.

## 0.1.38: objects on supports

Observed pot `Prop_Flowerpot_01_small_pink`, entity 3393, attached to sideboard
`Prop_Furniture_Castle_Sideboard_Large_01`, entity 3372. Full attachment state:
`ffffffffffffff7f2c0d000002000000`. Rejecting every nonzero attachmentContainerId
blocked preview; reloading could recompute the link and hide the issue.

Attached Props may now reference a support. Detached-with-parent, self-reference,
destruction/gravity/forced-detach flags remain refused. The full snapshot must
remain unchanged. The support itself is never moved.

Only after valid placement, obtain native mutable attachment access, revalidate
it and invalidate the cached support: attachmentContainerId=0, searchStartTime=0.
Preserve Attached and other flags. Do not call the native detach action, which
can destroy the item or add gravity. Native support search can resume on its
next processing/wake cycle. Neither preview nor cancellation changes attachment.

Native evidence: attach_to_surface `0x32f990`, search `0x351340`;
`0x3513f6` skips search for INT64_MAX, `0x3514fd` initializes a search from zero,
`0x3516c2` writes the found ID at +8. The mod does not compute the replacement
support cache; its actual refresh needs game observation.

Tests cover the exact pot state, pickup/cancel/refusal, changed support, failed
mutable access/expiry without partial movement, commit, visual sync and immediate
second pickup. Manually test without moving the sideboard, then save/reload.

## Configuration

The external name/list filter was removed in 0.1.39. Decor, Chests and Debug
remain independent, with Usable objects added for doors/wells. Native component
and attachment checks always apply.
