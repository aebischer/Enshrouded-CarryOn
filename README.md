# CarryOn for Enshrouded

Development version **0.1.52-dev**, a Windows x64 mod for
[Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader)
0.6.12 or later, SDK V1 / ABI 1.
Currently intended for solo play; dedicated servers and multiplayer are not certified.

Move chests with their contents and compatible world objects using craftable
**Transport pincers**, a native placement preview and the game's placement controls.

## With or without Kiln

Use the same mod download in both cases. **Kiln is optional.**

- **Without Kiln:** install [ModLoader 0.6.14](https://github.com/aebischer/Enshrouded-ModLoader/releases/tag/v0.6.14)
  with `Install.cmd`; it installs its proxy as `<game>/dinput8.dll`.
- **With Kiln:** install [Kiln](https://github.com/frostdev-ops/kiln/releases) first,
  then run the same ModLoader `Install.cmd`. It detects Kiln and installs the
  same loader binary as `kiln/native/enshrouded-modloader/modloader.dll` with
  `native.json`. Kiln keeps ownership of the root `dinput8.dll`.
- In either case, merge this mod's `Mods` folder into the game directory and
  enable it in **Edit world -> MODS**. CarryOn and Lantern are loaded by
  ModLoader, not directly by Kiln: do not copy their DLLs into `kiln/native`,
  `kiln/chain` or the game root.

If your Kiln profile restricts native modules, enable `enshrouded-modloader` or
allow native drop-ins. Do not run two copies of the loader. When adding Kiln to
an existing standalone installation, close the game, install Kiln, then rerun
ModLoader's installer so it can migrate receipt-identified old chain copies.
After removing Kiln, rerun the installer to restore standalone startup.
Preserve `ModLoader/worlds`, package data and installer receipts when updating.

Solo gameplay was tested with ModLoader 0.6.14, CarryOn 0.1.52-dev and Lantern
0.1.31-dev under Kiln **0.2.0-dev.29.19-beta.1**. Lantern lighting and equipped-state
restoration after reload were confirmed in game. This does not certify other
Kiln-hosted frameworks, multiplayer or future game/Kiln builds.

## Install without compiling

Download the compiled ZIP from
[Releases](https://github.com/aebischer/Enshrouded-CarryOn/releases/tag/v0.1.52-dev), install
ModLoader separately, then follow [INSTALLATION.txt](INSTALLATION.txt).
Both repositories and their downloads are public; no GitHub account is required.

With the game closed, merge the supplied `Mods` directory into the game directory:

```text
Enshrouded/
  enshrouded.exe
  dinput8.dll                 ModLoader standalone, or Kiln when installed
  Mods/EnshroudedModLoader/
    carryon/
      carryon.dll
      carryon.dll.mod
```

Do not create `Mods/Mods`, copy the mod into `ModLoader`, or replace the loader's
proxy. Enable CarryOn in **Edit → MODS** for the chosen world. A changed mod
selection may require a full game restart. Preserve your world saves and profiles.

## Use the pincers

Craft **Transport pincers** in manual crafting, after the construction hammer
and summoning staff, using **one scrap**. The tool has its own icon, translated
text, held model and native orange orb at the joint.

1. Equip the pincers. The building-area boundary becomes visible.
2. Enter transport mode with the native action (Tab by default).
3. Aim at a supported object to select and highlight it.
4. Use **Pick up**, adjust the preview using native positioning, rotation,
   angle and snapping controls, then **Place** at a permitted location.
5. To move between claims, open the map while carrying the preview, teleport
   to another authorized claim, then resume the preview and place the object.

The real object stays at its original location until placement is committed.
Movement preserves its entity and components, including chest contents,
decor comfort and supported door/well behavior. Cancelling the preview does
not create an inventory item or move the original object.

The map and teleport loading suspend commands. Resuming requires a fresh
preview; a stale placement request is never reused. Other menus, losing focus
or changing tools leave transport mode. Death and world destruction invalidate
the carried selection. Entry, exit and cancellation follow the configured
native actions rather than fixed physical keys.

## Per-world settings

In **Edit → MODS → CarryOn**:

| Key | Setting | Default |
|---|---|---|
| `move_chests` | Chests, including supported magical variants | Enabled |
| `move_objects` | Compatible non-voxel decor | Enabled |
| `move_interactive` | Usable objects: supported doors and wells | Enabled |
| `inspect_objects` | Debug: internal name, target ID and model GUID | Disabled |

The inspector works in transport mode even for an immovable target or outside
a claim. It depends on the native targeting query; unresolved models retain
a raw ID. Targets are also logged. No editable object list is required:
`objects.txt` is no longer read or distributed. A model name alone never grants
movement permission; the native component contract must be compatible.

All custom text covers the game's 15 interface languages. The catalogue is
`assets/localization/texts.json`; native controls retain the game's translations.
The build rejects a missing language/key or altered markup. Tests use the actual
native language IDs and do not count English fallback as translation coverage.
Saved setting keys and values are unaffected by language selection.

## Supported objects and limits

Dynamic Prop decor requires a transform, a non-voxel model, valid collision data
and persistent state. Native placement descriptors are preferred. Otherwise,
a floor-placement profile is built from boxes, spheres, capsules and cylinders,
alone or combined. Existing comfort is preserved. Objects resting on a support
can be picked up; the old support link is invalidated only after a valid commit.

Objects without collision geometry, marked `DoNotSave`, still scene-attached,
or using unsupported attachments are rejected. Enabling a setting does not
make voxels or unrelated object families movable.

Doors use their native Switch path, retaining opening state and leaf links.
Wells are identified by their water-drawing contract (family 13), without a name
allowlist; wells in use by an NPC or animal are rejected. Other interactive
objects require investigation of their native components.

Supported `enshrouded.exe` SHA-256:

```text
5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77
```

Game updates may require adaptation. Package validation, engine compatibility
and multiplayer certification are separate checks.

## Build and validate

Install Visual Studio C++ x64 tools, Windows SDK and PowerShell. `tools/vcvars.cmd`
locates Visual Studio through `vswhere`. Run `build.cmd` from the repository root.
It builds the DLL and runs SDK and transport-policy tests. Prepared icons,
translations and the 3D pack are included and embedded; Blender is not needed.

Distribute the generated `dist/Mods/EnshroudedModLoader/carryon` directory. The manifest in `dist`
is generated from `src/carryon.dll.mod` and the translation catalogue; use that
copy so the translated settings are included. Do not distribute build objects,
personal profiles or raw game resources.

For complete offline validation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -Fixtures C:\fixtures -ModelReference C:\reference\enshrouded_tools\core -PackageValidator C:\ModLoader\build\validate_mod_package.exe
```

Python is required for independent 3D-model verification. See
[build and validation instructions](docs/COMPILATION-VALIDATION.md) for fixtures
and dependencies. Game extracts are not provided. `build/validation.json`
records the tested hashes and explicitly reports `gameplayTested: false`.

Validation covers interruptions, cross-claim transport, replication, controls,
SDK, text, 3D assets, eleven well variants and 5,769 accepted floor-placement
profiles out of 6,282 examined models. This does not prove that every individual
object has been moved successfully in-game.

User tests confirmed chests, the counter, bottles, supported objects on surfaces,
doors and wells. Cross-claim movement and persistence after reloading were
confirmed in 0.1.45. Later fixes through 0.1.49 passed offline checks; full in-game
verification of interruptions, performance and new translations remains pending.

## Recent changes

- **0.1.47:** transport protections and cleanup; see [corrections](docs/CORRECTIONS-0.1.47.md).
- **0.1.48:** recoverable allocation failures, throttled missing reads, fewer
  unnecessary target queries, bounded indexing, distributed synchronization,
  smaller profiles and concurrent private-read queues. Native read batches
  remain intact. See [measurements and limits](docs/OPTIMISATIONS-0.1.48.md).
- **0.1.49:** strict 15-language catalogue validation and corrected native-language
  coverage tests. See [localization validation](docs/LOCALIZATION-0.1.49.md).

Tests include 256 mixed-read patterns, a native batch of 128 requests,
allocation failures and 2,000 concurrent private results delivered in order
and exactly once. These are offline measurements, not a measured FPS improvement.

## Technical documentation

- [SDK provenance](sdk/UPSTREAM.txt), [V1 standard](sdk/STANDARD_V1.txt), [localization](sdk/LOCALISATION.txt).
- [Chests](docs/DEPLACEMENT-COFFRES.md), [counter](docs/DEPLACEMENT-COMPTOIR.md), [floor placement](docs/POSE-AU-SOL.md).
- [Doors](docs/PORTES.md), [wells](docs/PUITS.md), [teleportation](docs/TELEPORTATION.md).
- [Transport interface](docs/INTERFACE-TRANSPORT.md), [third-party notices](THIRD_PARTY.md).

Dated research notes preserve investigation history and may describe limitations
of older builds. This README defines the current supported behavior.

## Chest selection correction (0.1.52)

Selection uses native storage template family 3 instead of requiring an upgrade
component. This includes wooden and silver chests and the huge magic chest.
Offline selection tests cover all eleven active chest templates, including the
three without `UpgradeStorage`, and the per-world chest setting. Server-side
inventory, interaction-lock and placement checks remain unchanged. The tester
confirmed the correction in game, including the three previously excluded chests.

## Pickup health correction (0.1.51)

Pickup and placement now check the player's authoritative server `Health`
attribute. The previous check required `NetworkHealth`, which can be absent
on a living player and incorrectly prevent pickup. Client previews do not
require server-only health data; server-side death and invalid-data checks
remain enforced before pickup and commit, including during travel.

Offline tests reproduce the observed 553/553 player state, missing client
health components, invalid attributes and death cancellation. Decor pickup
and basic chest transport were confirmed in game after the correction.

## Package layout update (0.1.50)

This release requires ModLoader **0.6.12 or newer**. Install the loader first: its
installer migrates recognized old packages into `Mods/EnshroudedModLoader/`,
including package data, while retaining mod IDs and per-world settings. Then
merge this release's `Mods` directory into the game directory. Do not delete
`ModLoader/worlds` or keep duplicate legacy package folders.

The 0.1.50 update changed distribution paths without changing gameplay. The
current release also includes the pickup and chest selection fixes above.
Download the [compiled package](https://github.com/aebischer/Enshrouded-CarryOn/releases/tag/v0.1.52-dev).
