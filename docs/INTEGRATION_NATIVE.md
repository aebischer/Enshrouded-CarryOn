# Initial native integration research

This was preliminary static research on November 13, 2026, before hooks were installed. String RVAs are not callable function addresses:

| RVA | Native string |
|---|---|
| `0x1c28c50` | `keen::ds::ecs::BuildingPickUpEvent` |
| `0x1d48810` | `StorageChest` |
| `0x1d48778` | `ReadOnlyLootChest` |
| `0x1c257d0` | `ObjectReference<keen::ecs::DefaultInventorySetup>` |
| `0x1c25c48` | `keen::ecs::DefaultInventoryReference` |
| `0x1c34cb0` | `keen::ecs::InventoryTransferAction` |
| `0x1c45c88` | `keen::ecs::UpgradeStorageAction` |
| `0x1c221e0` | `keen::actor::PlaceVoxelObjectEvent` |

Names alone did not establish signatures, inventory preservation or movement support. A direct RIP-relative LEA scan found no references for BuildingPickUpEvent/StorageChest; indirect/reflection references were still possible. Initial SDK checks passed but Initialize deliberately refused because native integration was missing. These are historical findings, not current limitations.

## Original research record

The complete original French notebook is preserved in [the historical archive](archive/fr/INTEGRATION_NATIVE.md). It records earlier development stages and is not the current user/developer guide. Technical measurements and offsets have not been discarded.
