# Audit corrections — 0.1.47-dev

These fixes address reproduced 0.1.46 defects. Validation described here is
offline; gameplay checks are performed by the user.

## Movement and lifetime

- Visual synchronization follows authoritative server adjustments after placement
  instead of silently discarding pending repair. Missing or changed-model entities
  remain excluded.
- Expired placements are cancelled before writes or collision movement.
- Mouse commands are copied as values with time/session generation. No native
  mapping pointer survives its callback; world destruction clears the snapshot.
- Nonuniform scales are rejected at selection and server validation because the
  native collision movement path requires uniform scale.
- Visuals without a Model handle cannot be picked up: VoxelModel movement is not
  supported. Debug inspection can still identify these objects.

## Resources and controls

- Catalogue indexing retains unavailable IDs and retries after one second rather
  than marking incomplete registries as finished.
- Temporary floor-profile read/allocation failures retry with throttling;
  structurally unsupported formats remain rejected. Published allocations stay stable.
- Native actions 129/126 handle entry/exit/cancel, with eligibility altered only
  for the equipped pincers in gameplay. Windows Tab/Escape capture is removed.
  Native bindings/glyphs remain; rebound keyboard/controller behavior needs game tests.
- Counter-derived fallback profiles display localized Decor instead of inheriting
  the counter's name, caption or description.
- Resource limits include alignment padding.
- Graphics-catalogue resets can rebuild private textures. Old GPU handles and
  allocations are neither reused nor arbitrarily freed by the mod.
- LoadResource results are checked. Large SHA-256 buffers are allocated on the
  heap before opening system handles.

## Cleanup and verification

Decor classification and type hashing are shared; refusal masks are named;
synchronization/pose-comparison names are generalized to objects. Unused selection
fields and four translation keys were removed. Client/server checks remain
independent. At that version, localization SDK provenance was pinned to the
verified ModLoader 0.6.10 revision.

Regression coverage includes expired placement before mutation, server-adjusted
poses followed by render repair, late resources, early incompatible-visual refusal,
session changes and texture-catalogue rebuilding. Native commands are tested for
edges, held inputs, menus, blocking and unrelated-action forwarding. Existing
suites cover chests/decor/doors/wells, 144 interruption cases and cross-claim travel.

Full validation also covers geometry corpus, native catalogues, translations,
3D model and SDK package. Successful runs write build/validation.json with exact
package hashes and gameplayTested=false.
