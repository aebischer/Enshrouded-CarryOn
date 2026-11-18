# Transport interface

Equipping the pincers displays native building boundaries. The transport action
(Tab by default) enters/leaves transport mode. Cancel (Escape by default), menus
other than the map, focus loss and changing tools cancel the current operation.
The command strip offers Pick up, Place and Finish. Pickup requires an authorized
target in an allowed area; placement depends on the preview and native validation.

While carrying, native rotation, fine rotation, offset and snapping hints are
shown. The player's hide-hints setting is respected. Since 0.1.47, every command,
including entry/exit and cancellation (129/126), follows native actions.
Context `0x3c47b0` captures evaluated state; filter `0xe85800` keeps those actions
eligible only for the equipped pincers in focused gameplay with fresh observation.
Consumed states do not trigger the corresponding original action. Bindings,
modifiers and physical input remain engine-managed; there is no Windows Tab/Escape
interception. Offline tests cover edges, held keys, menus, blocking flags and
unrelated actions. Rebound keyboards/controllers still need in-game testing.

Highlight and preview use native graphics. The real move happens only after
validation, entirely within the destination claim. The map and teleport loading
suspend commands/preview, which resume on return to gameplay with a fresh pose.
Custom text is localized; outside-area feedback reuses the hammer's native text
and appearance. The orange orb remains a native resource.

Getter `0x3aa980` provides boundary visual state. A thread-local copy changes only
its visibility flag, not permissions, altar state or shared game data.

## Native placement hints (introduced in 0.1.42)

Parent `0xb6bb80` is preserved. It lays out item panel `0xb6a560` (name, 90-degree
rotation, current angle, snapping), then Adjustment panel `0xc6e1d0` above it.
All three arguments are forwarded: input context and two output rectangles.
Anchoring, scale and visibility settings remain native.

Only during this call, a thread-local pointer identifies the carried object's
ItemInfo. Lookup `0xe94470` returns it exclusively at return site `0xb6bc42`,
where the parent requests the equipped item. Every other lookup stays native;
no inventory item is replaced.

Native constructors handle UI contexts, copies and destruction. Predicate
`0xd8e320` permits these contexts only during that draw scope and when their
ItemInfo at +0 matches the borrowed descriptor. Shared building, permission
and movement state is unchanged.

Hints require a fresh preview, focus and an enabled object category. Otherwise
the parent runs without substitution. No mutex is held during the native call.
Tests cover chests, decor, doors, disabled categories, stale data, context copies,
unrelated lookup forwarding, output rectangles and absence of entity writes.
Visual layout remains subject to manual game verification.
