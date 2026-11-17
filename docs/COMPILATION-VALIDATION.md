# Compiler et vérifier la 0.1.48

## Compilation autonome

Cloner le dépôt privé avec un compte autorisé, ouvrir une console Windows et
exécuter `build.cmd` à sa racine. Visual Studio C++ x64 et le SDK Windows sont
nécessaires ; MinHook et les headers SDK sont fournis. Aucun chemin vers un
autre dépôt n’est requis pour cette compilation et ses tests de politiques.

`assets/pincer3d/pincer.pack` est le pack préparé embarqué par
`src/pincer_model.rc`. L’icône et les traductions sont transformées en données
de compilation par les scripts `embed-*.ps1`. Les sorties restent dans
`build` et `dist`, ignorés par Git. La DLL et le manifeste généré sous
`dist/Mods/carryon` sont les deux fichiers à installer.

## Validation complète avec données locales

`tools/validate.ps1` reconstruit le mod puis teste les catalogues, l’adaptateur
natif, la mémoire, le corpus de poses, le modèle avec un décodeur indépendant
et le paquet avec le validateur du chargeur. Il ne pilote pas le jeu.

Fournir explicitement les trois paramètres sur une autre machine :

| Paramètre | Contenu |
|---|---|
| `-Fixtures` | Dossier des fichiers listés ci-dessous, extraits du client pris en charge |
| `-ModelReference` | Dossier `enshrouded_tools/core` du décodeur EnshroudedBlenderTools, contenant `render_model.py` et `material.py` |
| `-PackageValidator` | `build/validate_mod_package.exe` construit dans Enshrouded-ModLoader avec `build-sdk.cmd` |

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

Ces extractions ne sont pas publiées. Le nom conserve le GUID, le type et la
partie de ressource. `extract-prop-fixtures.py` construit le corpus depuis les
archives locales (Python et `zstandard`) :

```bat
python tools\extract-prop-fixtures.py "D:\SteamLibrary\steamapps\common\Enshrouded" C:\fixtures\prop-placement-fixtures.bin
```

Adapter les deux chemins. La préparation des autres fixtures n’est pas
automatisée de bout en bout par le dépôt.

Les valeurs par défaut de `validate.ps1` correspondent à l’espace historique :
dossier parent `work`, référence sous ce dossier et validateur dans le `build`
du parent. Les remplacer sur une autre machine. Python est nécessaire au test
indépendant du modèle 3D.

`build/validation.json` n’est écrit qu’après réussite de toute la suite. Il
contient les SHA-256 des fixtures, de la DLL et du manifeste, la date et
`gameplayTested: false`. Un rapport ancien ne valide pas un nouveau binaire :
comparer les empreintes avant installation. Les essais en jeu sont manuels.

## Sources du modèle 3D

`assets/pincer3d/source` contient le FBX Meshy fourni pour ce projet et ses
textures. `pincer.blend` est le modèle préparé, `mesh.json` les données de
maillage et LOD, `build-report.json` les identifiants et l’empreinte du pack.

`prepare-pincer-model.py` (Blender) et `pack-pincer-model.py` utilisent le
dossier historique `work/pincer3d`, des textures DDS préparées et des
descripteurs natifs extraits localement. Ils ne constituent pas une chaîne
autonome FBX → pack sur un clone vierge. Le pack préparé est fourni pour que
`build.cmd` n’en dépende pas. Ne pas publier les extractions brutes du jeu.

## Installation et essais

Le dépôt ne fournit pas encore d’installateur CarryOn autonome. Après validation,
jeu fermé, sauvegarder les deux fichiers installés puis remplacer uniquement
`Mods/carryon/carryon.dll` et `carryon.dll.mod` par ceux de `dist`.
Conserver les profils du chargeur et les sauvegardes de monde.

Vérifier manuellement saisie/pose/annulation, rotations, changement d’outil,
perte de focus, carte, téléportation entre claims, retour au menu, sauvegarde
et rechargement. Inclure coffre rempli, décor sur support, porte et puits.
La validation hors jeu ne certifie pas le serveur dédié ni le multijoueur.
