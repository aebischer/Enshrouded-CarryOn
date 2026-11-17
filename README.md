# CarryOn pour Enshrouded

Version de développement : **0.1.48-dev**. DLL Windows x64 pour
[Enshrouded ModLoader](https://github.com/aebischer/Enshrouded-ModLoader)
**0.6.10**, SDK V1 / ABI 1. Utilisation solo ; serveur dédié et multijoueur non certifiés.

## Utilisation

Fabriquer la **pince de transport** dans les recettes manuelles, après le marteau
et la baguette d’invocation, avec **une ferraille**. La pince possède son icône,
ses textes traduits, son modèle tenu en main et l’orbe orange natif à sa jonction.

1. Équiper la pince : les limites de construction apparaissent.
2. Entrer en mode transport avec l’action native correspondante (Tab par défaut).
3. Viser un objet compatible : il est sélectionné et surligné.
4. Utiliser **Saisir**, ajuster l’aperçu avec les commandes natives de placement,
   puis **Poser** lorsque l’emplacement est autorisé. Les raccourcis affichés
   reprennent le panneau natif : ajustement, rotation, angle et accrochage.
5. Pour un transport entre claims, ouvrir la carte pendant la saisie, se
   téléporter vers un autre claim autorisé, puis reprendre l’aperçu et poser.

L’objet réel reste à son emplacement d’origine jusqu’à validation de la pose.
Le déplacement conserve l’entité et ses composants : inventaire des coffres,
confort des décors, fonctionnement des portes et des puits compatibles.
Annuler l’aperçu ne crée pas d’objet d’inventaire et ne déplace pas l’original.

La carte et le chargement de téléportation suspendent les commandes. La reprise
exige un aperçu renouvelé ; aucune ancienne demande de pose n’est réutilisée.
Les autres menus, la perte du premier plan et le changement d’outil quittent le
mode transport. La mort et la destruction du monde invalident la saisie.
Depuis 0.1.47, entrée/sortie et annulation suivent les actions natives configurées,
plutôt que des touches physiques fixes.

## Réglages par monde

Dans **Modifier → MODS → CarryOn**, le chargeur conserve séparément :

| Clé | Réglage | Défaut |
|---|---|---|
| `move_chests` | Coffres, y compris les variantes magiques compatibles | Activé |
| `move_objects` | Décors non voxel compatibles | Activé |
| `move_interactive` | Objets utilisables : portes et puits compatibles | Activé |
| `inspect_objects` | Debug : nom interne, ID de cible et GUID du modèle | Désactivé |

Les réglages et textes de l’interface sont traduits dans quinze langues.
Le diagnostic fonctionne en mode transport, même pour une cible non déplaçable
ou hors claim. Il dépend du résultat de la requête native ; une cible sans
modèle résolu conserve un identifiant brut. Les cibles sont aussi journalisées.

Aucun fichier de liste n’est nécessaire depuis 0.1.39. `objects.txt` n’est plus
lu ni distribué ; une ancienne copie peut être retirée. Le nom d’un modèle ne
suffit pas à autoriser son déplacement : les composants natifs doivent être compatibles.

## Objets pris en charge et limites

Les décors dynamiques de famille Prop doivent posséder une transformation,
un modèle non voxel, des collisions valides et un état sauvegardable.
Le descripteur natif de placement est privilégié. À défaut, un profil de pose
au sol est préparé à partir des collisions : boîtes, sphères, capsules et
cylindres, seuls ou combinés. Le confort est conservé lorsqu’il existe.
Un objet posé sur un support peut être saisi ; le lien avec l’ancien support
n’est invalidé qu’après une pose validée.

Les modèles sans collisions, marqués `DoNotSave`, encore attachés à la scène
ou avec des fixations non prises en charge restent refusés. Les voxels et
les autres familles ne deviennent pas déplaçables par simple activation d’une option.

Les portes utilisent leur parcours natif Switch, avec état d’ouverture et liens
des battants. Les puits sont reconnus par leur contrat de puisage (famille 13),
sans liste de noms ; un puits utilisé par un PNJ ou un animal est refusé.
Les autres objets fonctionnels nécessitent une étude de leurs composants.

Le client pris en charge est identifié par le SHA-256 de `enshrouded.exe` :

```text
5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77
```

Une mise à jour du jeu peut nécessiter une adaptation. Ne pas assimiler
validation du manifeste, compatibilité moteur et certification multijoueur.

## Compilation et installation

Prérequis : Visual Studio avec C++ x64 et SDK Windows, PowerShell.
`tools/vcvars.cmd` détecte Visual Studio via `vswhere`. Depuis la racine :

```bat
build.cmd
```

Le script compile la DLL et exécute les tests SDK et politiques de transport.
L’icône, les traductions et le pack 3D préparé sont fournis et embarqués ;
Blender n’est pas nécessaire à cette compilation. La sortie à distribuer est :

```text
Mods/
  carryon/
    carryon.dll
    carryon.dll.mod
```

Installer le chargeur séparément. Fermer le jeu, sauvegarder la version précédente,
puis copier `dist/Mods/carryon` dans le dossier `Mods` à côté de `enshrouded.exe`.
Ne pas copier les `.obj`, `.lib`, `.exp` ou le dossier `build`. CarryOn ne fournit
pas son propre `dinput8.dll`. Activer ensuite CarryOn dans les mods du monde.
Une modification de la sélection peut nécessiter une relance complète du jeu.

Le manifeste distribué est généré par `tools/embed-texts.ps1` à partir de
`src/carryon.dll.mod` et `assets/localization/texts.json` : utiliser la copie
produite dans `dist`, qui comprend les traductions. Les paramètres restent
stockés par le chargeur ; ne pas distribuer les profils personnels.

Le dépôt fournit les sources et ressources du mod ; aucune release binaire
n’est créée automatiquement par la publication d’un commit.

## Validation et état des essais

Avant installation, lancer la validation complète hors jeu :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -Fixtures C:\fixtures -ModelReference C:\reference\enshrouded_tools\core -PackageValidator C:\ModLoader\build\validate_mod_package.exe
```

Python est requis pour la vérification indépendante du modèle 3D. Les paramètres
et prérequis sont détaillés dans [COMPILATION-VALIDATION.md](docs/COMPILATION-VALIDATION.md).
Les extractions de jeu utilisées par les tests ne sont pas fournies.
`build/validation.json` consigne les empreintes du paquet testé et indique
explicitement que les tests en jeu ne sont pas automatisés.

La validation couvre les interruptions, le transport entre claims, la réplication,
les commandes, le SDK, les textes, le modèle 3D, les onze variantes de puits et
5 769 profils de pose sur 6 282 modèles examinés. Les autres profils sont refusés
explicitement. Ce corpus ne prouve pas le déplacement individuel de tous ces objets.

Les essais utilisateur ont confirmé les coffres, le comptoir, les bouteilles,
les objets sur support, les portes et les puits. Le transport entre claims et
sa persistance après rechargement ont été confirmés en 0.1.45. Les corrections
ultérieures jusqu’à 0.1.48 ont passé les tests hors jeu ; leur validation complète
en jeu, notamment interruptions et gains de performance, reste à confirmer.

## Dernières corrections

- **0.1.47** : protections de transport et nettoyage détaillés dans
  [CORRECTIONS-0.1.47.md](docs/CORRECTIONS-0.1.47.md).
- **0.1.48** : échecs d’allocation récupérables, lectures absentes temporisées,
  ciblage inutile supprimé, indexation par groupes de 64, synchronisations
  réparties, profils moins gourmands et file privée concurrente optimisée.
  Les lots natifs de lecture restent préservés.
  Voir [OPTIMISATIONS-0.1.48.md](docs/OPTIMISATIONS-0.1.48.md) pour les mesures
  hors jeu et leurs limites ; elles ne constituent pas un gain de FPS mesuré.

Les tests vérifient notamment les 256 combinaisons de lectures mixtes, un lot
natif de 128 requêtes, les pannes d’allocation et 2 000 résultats privés
produits/consommés concurremment, dans l’ordre et exactement une fois.

## Documentation technique

- [SDK et provenance](sdk/UPSTREAM.txt), [standard](sdk/STANDARD_V1.txt), [multilingue](sdk/LOCALISATION.txt).
- [Coffres](docs/DEPLACEMENT-COFFRES.md), [comptoir](docs/DEPLACEMENT-COMPTOIR.md), [pose au sol](docs/POSE-AU-SOL.md).
- [Portes](docs/PORTES.md), [puits](docs/PUITS.md), [téléportation et interruptions](docs/TELEPORTATION.md).
- [Interface de transport](docs/INTERFACE-TRANSPORT.md), [tiers et ressources](THIRD_PARTY.md).

Les notes de recherche datées conservent l’historique et peuvent décrire les
limitations d’anciennes versions. Ce README décrit le périmètre actuel.
