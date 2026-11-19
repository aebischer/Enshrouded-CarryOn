# Build and validate CarryOn 0.1.49

## Standalone build

Clone the public repository (no GitHub account required), open a Windows console
and run `build.cmd` from its root. Visual Studio C++ x64 tools, Windows SDK and
PowerShell are required. MinHook and SDK headers are included. Building the DLL
and its policy tests does not require another repository.

`assets/pincer3d/pincer.pack` is embedded by `src/pincer_model.rc`. The icon and
translations become compiler inputs through `embed-*.ps1`. Outputs stay under
Git-ignored `build` and `dist`. Install the DLL and generated manifest from
`dist/Mods/EnshroudedModLoader/carryon`; the source manifest alone omits generated translations.

## Full validation with local fixtures

`tools/validate.ps1` rebuilds the mod and tests catalogues, native adapters,
memory, the floor-placement corpus, the model through an independent decoder,
and the package through the loader's validator. It does not control the game.

Supply these parameters on another machine:

| Parameter | Required content |
|---|---|
| `-Fixtures` | Directory containing the files below, extracted from the supported client |
| `-ModelReference` | EnshroudedBlenderTools `enshrouded_tools/core`, including `render_model.py` and `material.py` |
| `-PackageValidator` | ModLoader's `build/validate_mod_package.exe`, built by `build-sdk.cmd` |

```text
resource-550d7a506569c348bc33efdf9fb894d4-0x39768775-0.bin
prop-placement-fixtures.bin
resource-ee386f839f792f4fa64f4a31593a2a1a-0x39768775-0.bin
resource-e3a56abbb97f3946ad1fbd266c5ccace-0xb5ce8765-0.bin
resource-873d587c00f3174da27e9a77fbbfaeba-0xb5ce8765-0.bin
resource-5dd364304273ca40bdc9aad58f83bf45-0x72ab39b6-0.bin
resource-0fde575e0c2859458191f02c1ba656e2-0x5f5382cf-0.bin
resource-f64de1468ffb664f9463e23b9e19e48e-0xfef918b8-0.bin
resource-ac11e027e3891e4b8aa6d69aedac1a2d-0xf9e86997-0.bin
resource-618b3ac8dbea284d914688c4c8d9cb98-0x39768775-0.bin
resource-0b6db1ac5b832949a2f00c6046bfc35c-0x39768775-0.bin
```

These game extracts are not published. Filenames preserve resource GUID, type
and part. Build the Prop corpus from local archives with Python and `zstandard`:

```bat
python tools\extract-prop-fixtures.py "D:\SteamLibrary\steamapps\common\Enshrouded" C:\fixtures\prop-placement-fixtures.bin
```

Adjust both paths. Preparing the other fixtures is not automated end-to-end.
Default validation paths assume the historical workspace: sibling `work`, a
reference decoder under it, and the parent loader's `build` directory. Override
these on another machine. Python is required for independent model decoding.

`build/validation.json` is written only after the full suite succeeds. It records
fixture/DLL/manifest SHA-256 values, date and `gameplayTested: false`. An older
report does not validate a changed binary; compare hashes before installation.

## Model regeneration

`assets/pincer3d/source` contains the project-supplied Meshy FBX and textures.
`pincer.blend` is the prepared model; `mesh.json` holds mesh/LOD data;
`build-report.json` records resource IDs and pack digest.

`prepare-pincer-model.py` (Blender) and `pack-pincer-model.py` depend on the
historical `work/pincer3d`, prepared DDS textures and locally extracted native
descriptors. They are not a standalone FBX-to-pack pipeline on a clean clone.
The prepared pack makes normal `build.cmd` independent of regeneration.
Do not publish raw game extracts.

## Installation and manual checks

After validation, close the game, back up the installed files and replace only
`Mods/EnshroudedModLoader/carryon/carryon.dll` and `carryon.dll.mod` from `dist`. Preserve loader
profiles and world saves. There is no separate CarryOn installer.

Manually test pickup/place/cancel, rotations, tool changes, focus loss, map,
teleportation between claims, returning to menus, save and reload. Include a
filled chest, decor on a support, a door and a well. Offline tests certify
neither dedicated-server nor multiplayer behavior.

See the [SDK V1 standard](../sdk/STANDARD_V1.txt) for the integration contract.
