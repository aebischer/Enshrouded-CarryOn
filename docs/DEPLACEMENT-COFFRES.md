# Chest movement implementation and investigation history

Current chest transport preserves the existing server entity and inventory. Pickup creates a separate native-style preview; the original remains unchanged until a valid placement commits. Client/server object identity, source state, permissions, destination claim and collision are revalidated. Failure/cancellation must not duplicate, consume or destroy the chest.

The server updates the authoritative transform and native collision manager. Client synchronization waits for replicated pose and updates render and collision state together. Quantized rotations are compared with normalized angular tolerance (0.25 degrees), treating q and -q equivalently; synchronization uses the received pose rather than overwriting the authoritative server pose.

Placement controls use the native input and UI path. Cross-claim movement, loading suspension, session generations and delayed replication are described in [teleportation](TELEPORTATION.md). Shared collision/render regression details are in [the counter integration](DEPLACEMENT-COMPTOIR.md). Both ordinary and compatible magical chests use the common path.

Run the complete suite described in [build and validation](COMPILATION-VALIDATION.md). Manually test filled ordinary/magical chests, preview/cancel, valid and refused placement, rotation, interruptions, cross-claim travel and save/reload. The original notebook below preserves the successive native offsets, diagnostic readings and fixes; it must not be treated as a list of current unsupported features.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/DEPLACEMENT-COFFRES.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.
