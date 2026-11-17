# Décors du monde : identification et persistance

Relevé du 16 novembre 2026. Recherche statique et lectures mémoire, aucune modification de DLL,
d'archive ou de sauvegarde. Aucun déplacement de décor validé en jeu à ce stade.

## Sources et portée

- Exécutable local épinglé par CarryOn et métadonnées de réflexion.
- Archives locales `enshrouded.kfc` et `enshrouded.kfc_resources`.
- Script reproductible : `../../work/research-decor.py`.
- Résultat détaillé : `../../work/decor-archive-report.json`.
- 12 263 modèles et 1 604 ressources de zones de scène lus ; 6 147 modèles
  portent `keen::ecs::Destructible`. Ce nombre inclut des prototypes, des bases,
  des ennemis/pièges et des ressources naturelles : ce n'est pas une liste de
  décors admissibles. Le lecteur ne traite que les ressources intégralement
  contenues dans un bloc d'archive ; ces nombres décrivent son échantillon.

## Types et exemples effectivement retrouvés

| Famille | Exemple natif | Composants distinctifs |
| --- | --- | --- |
| Table cassée du décor | `Prop_Furniture_Fogger_Table_A_broken_01` | `Destructible`, `OnDestroy`, `StaticTransform` |
| Cage de décor | `Props_scavengers_cage_A` | `Destructible`, `OnDestroy`, `StaticTransform` |
| Pile de caisses | `Prop_Destructible_Furniture_Crate_Stack01D` | mêmes composants, plus `RandomLootOnDestroy` |
| Arbre de ressources | `Scatter_Loot_Grassland_Above_Tree_01_e` | `MiningNode`, `ResourceNodePickupDrops`, `FogVoxelModel`, `Destructible` |
| Nœud de pierre | `Loot_Node_Material_Stone_DeepForest_f` | `MiningNode`, `ResourceNodePickupDrops`, `Health`, `HealthBasedScale` ; pas de `Destructible` sur ce modèle |
| Forge de gemmes | `Shroud_Gem_Forge_01_Grassland` | `ShroudForge`, `Crafting`, `CraftingInteraction`, `Actor`, animation, déclencheur, VFX ; pas de `Destructible` sur ce modèle |

Les trois premiers exemples ont `CurrentTransform`, `RenderTransform`,
`ModelResource`, `ModelComponent`, `ColliderResourceComponent`, `StaticCollider`
et `StaticClientCollider`. Ils n'ont ni `UsedItem` ni `UpgradeStorage`.
Leur présence dans les scènes est vérifiée, pas seulement leur nom dans le catalogue.

`Destructible.maxHealth` est un uint16. `DisableDestructible` existe aussi.
`SpawnEntityOnDestroy` comporte un modèle à engendrer et un état `triggered`.
Il faut éviter de passer par la destruction pour simuler un déplacement : cela
peut produire du butin, déclencher des événements ou changer l'objet.

Les préfixes des noms servent à repérer les candidats ; ils ne constituent pas
une classification moteur suffisante. Commencer par une liste explicite de
modèles de décor simples, avec vérification des composants, plutôt que permettre
automatiquement tout ce qui est destructible. Les forges constituent un adaptateur
distinct, pas un cas couvert par le filtre des décors statiques.

## Ce que la scène conserve

`SceneEntityChunkResource` contient :

- +0x00 : modèles (`templates`).
- +0x08 : mots de bits `templatesDoNotSave`.
- +0x10 : `hasPersistentData` (uint16 par instance dans la zone témoin).
- +0x18 : placements `entities`, avec index de modèle et transformation initiale.
- +0x20 : `ChunkPersistenceInfo`.

`PersistentEntityInfo` décrit une liste de composants avec leurs signatures,
tailles et offsets de données. Sur toutes les listes lues, les données concernent
notamment inventaires, états d'interrupteurs, verrous et spawners. **Aucune ne
contient `CurrentTransform` ou `RenderTransform`.** Cela établit l'absence dans
ce mécanisme, pas l'absence de tout autre canal de sauvegarde du moteur.

Témoin précis : scène `dbea9f50604c5f429c7cdeeefd9b6920`, partie 181.
Elle contient 1 411 placements et 1 411 entrées `hasPersistentData`.
Le modèle `127b1500640cbb49875e891df4dc4103` (table cassée 01), index de modèle
102, est placé à l'index d'instance 373. Son entrée `hasPersistentData` vaut 0,
et son bit `templatesDoNotSave` vaut 0. Même résultat pour la table cassée 03,
modèle index 105, instance 376.

Attention : les tables de composants persistants ne sont PAS indexées directement
par l'index du modèle. Ne pas confondre modèle, placement et entrée persistante.

`SceneEntityChunkTemplateInfo` expose également `doNotSave`, `isMovingEntity`
et `persistentDataIds`. `ForceSave` et `DoNotSave` sont des marqueurs natifs, mais
leur existence ne prouve pas que l'ajout de `ForceSave` convertit un décor en
objet mobile sauvegardé. Cette opération n'est pas validée.

`GameObjectId` possède `value` et `type` (`Entity`/`SnappingBox`). Il n'apparaît
pas comme composant dans les modèles examinés ; ce n'est donc pas une preuve
d'identifiant de sauvegarde stable. Ne pas sérialiser aveuglément l'EntityId runtime.
`HolisticObjectId` n'a pas été établi comme identité de sauvegarde du monde joueur.

## Adaptation nécessaire dans CarryOn

Le commit actuel écrit `CurrentTransform` côté serveur, déplace la collision et
programme la synchronisation du rendu/collision client. Il ne gère pas l'identité
de placement d'un décor de scène. L'adaptateur suppose aussi un `UsedItem` et un
`ItemInfo` pour l'aperçu et le solveur de placement.

Pour un décor simple, les transformations, la synchronisation et une partie des
collisions sont réutilisables. Il faut en revanche :

1. Identifier la scène, la zone d'origine et l'index de placement, puis vérifier
   le GUID de modèle et la transformation initiale. Raccorder cette identité à
   l'entité runtime ; ce raccordement reste à tracer.
2. Construire une description de placement à partir du modèle et de la collision,
   sans exiger une recette ou un objet d'inventaire. Maintenir la même claim.
3. Après validation native, appliquer la nouvelle transformation sur le serveur,
   les collisions et le rendu comme pour les coffres ; conserver l'état existant.
4. Ajouter un traitement de persistance adapté aux instances issues de la scène.

Deux voies à départager par un prototype :

- **Conversion native en instance sauvegardée mobile** : retrouver la voie native
  qui supprime durablement le placement source et sauvegarde une instance au nouvel
  emplacement, sans appeler ses effets de destruction. Préférable si elle existe
  et préserve les composants. Aucune fonction effectuant cette conversion n'est
  identifiée avec certitude dans ce relevé.
- **Registre de déplacements CarryOn par sauvegarde** : conserver l'identité source
  et la destination ; réappliquer le déplacement à l'instance d'origine lors du
  chargement de sa scène, sans créer un doublon. Ce registre doit suivre le cycle
  de sauvegarde/restauration du monde, et pas seulement le profil de configuration
  des mods. Il faut traiter explicitement le chargement de la zone d'origine et
  de destination, leur déchargement, la destruction ultérieure et l'absence du mod.
  Même claim ne signifie pas forcément même zone de streaming.

Ne pas simplement ajouter un champ position au fichier de configuration du monde :
cela ne garantit ni le chargement de l'objet déplacé, ni la cohérence après une
restauration de sauvegarde. Ne pas modifier les ressources de scène globales du jeu.

## Validation à faire avant de déclarer la fonctionnalité prête

Prototype sur une table/cage sans logique complexe, dans une copie de sauvegarde :
déplacement et rotation, sortie/rechargement, déchargement/rechargement de zone,
traversée d'une frontière de zone dans la même claim, puis second déplacement.
Vérifier exactement une instance, ancienne place libérée, modèle/collision au nouvel
emplacement, interaction cohérente, absence de butin généré. Tester ensuite une
destruction volontaire et une restauration de sauvegarde pour éviter la résurrection
ou la duplication. Les forges, portes, objets de quête et sous-entités nécessitent
leurs propres vérifications. Aucun de ces essais en jeu n'a encore été effectué.

## Comptoir témoin : relevé en mémoire et placement de scène

Objet visé par Alain, relevé en lecture seule du processus 11808 le 16 novembre :

- Modèle `Prop_Furniture_Comfort1_Bar_01_Wood`, GUID en ordre d'octets d'archive
  `423c01619630d241b98a591808d9ea21` ; entité runtime 2752 sur client et serveur.
- Position serveur : `(4390.79296875, 779, 2792.595703125)`.
- Correspondance dans la scène `dbea9f50604c5f429c7cdeeefd9b6920`, partie 417,
  placement 1451, modèle index 179. Écart de position inférieur à 0,9 mm.
  Le second exemplaire le plus proche est à 2,17 m ; recherche sur 312 placements.
- Transformation compacte source : `20cb261100000b03b098e80aff010040ff1f`.
- `hasPersistentData` vaut 0 pour ce placement ; `templatesDoNotSave` est faux.
- `ComfortProvider` est effectivement présent côté serveur, statique, avec
  `comfort=2`, `category=3` (Table), `range=5.0`, `freezingResistance=1`.
- Le composant dynamique `Comfort` est également présent. Ne pas confondre son
  stockage d'attributs avec la valeur de contribution du fournisseur.
- `AttachToSurface`/`DynamicAttachToSurface` et `AttachToSurfaceIsSleeping` sont
  présents : la fixation au support est un autre état à vérifier lors du commit.

Fichiers témoins : `work/decor-counter-{client,server}.json`,
`work/decor-counter-scene.json`. Le lecteur des composants présente parfois deux
noms statique/dynamique pour un même index ; ce ne sont pas deux composants distincts.
L'association scène/placement est vérifiée par modèle et position initiale ; le
lien interne du gestionnaire de scène vers l'EntityId reste à tracer.

## Calcul natif du confort : résultat du suivi de code

Dans l'exécutable épinglé, la table de systèmes référence :

- `base_comfort` : callback RVA `0x2538f0`.
- `base_comfort_cleanup` : callback RVA `0x253960`.
- `comfort_energy_gathering` : callback RVA `0x2539a0`.
- `comfort_init` : callback RVA `0x124baa0`.

Le parcours de collecte récupère les entités candidates, lit leur position, puis
calcule leur distance au demandeur (`0x25414c`–`0x254193`). Il lit ensuite le
fournisseur de confort et conserve le maximum par catégorie (`0x2541a0`–`0x2541d1`).
La catégorie 0 utilise le carré de `range` ; pour la catégorie 3 du comptoir, le
seuil est la constante **225**, soit **15 mètres**. Le champ statique à 5 mètres
n'est donc pas sa portée effective dans ce parcours. La requête préparatoire
utilise également une demi-étendue de 15 mètres.

Conséquence : conserver le même fournisseur et faire parvenir le meuble à la
requête spatiale à sa nouvelle position permettrait de garder le calcul natif.
Ne pas ajouter manuellement 2 au confort du joueur : un meilleur objet de catégorie
Table peut déjà être présent. Le suivi de code confirme le calcul final ; il ne
prouve pas encore que la liste spatiale des candidats se mettra à jour après notre
déplacement. Les appels `0x9600c0` et `0x960390` appartiennent à ce parcours de
requête et restent à analyser avant de garantir ce point.

## Piste supplémentaire pour la sauvegarde native

Le moteur contient un parcours explicitement lié au transfert des entités de scène
lors des changements de zone de construction. La fonction RVA `0xe79200` compare
les anciennes/nouvelles listes de zones et enregistre leurs changements. Son message
d'échec indique que les entités de scène ne seront pas transférées si ces changements
ne peuvent pas être enregistrés. Callbacks appelants : `0x3313e0` et `0x331410`.

**Cette fonction ne réalise pas elle-même un déplacement de meuble.** Aucun appel
n'a été exécuté. Il reste à suivre le consommateur de ces changements pour déterminer
comment l'instance source est transférée à la persistance du monde. C'est désormais
la piste prioritaire avant d'introduire un registre de positions propre au mod.
L'absence de transformation dans `ChunkPersistenceInfo` ne prouve donc pas que le
jeu est incapable de sauvegarder un décor transféré vers un autre régime d'entités.

Des sérialiseurs d'entités sauvegardées copient bien les 56 octets de transformation
(par exemple RVA `0xdc5b80`, copie à `0xdc5cd2`), mais leur admissibilité pour le
comptoir n'est pas établie. Ne pas appeler un sérialiseur isolément ni ajouter
`ForceSave` sans avoir établi le transfert et la neutralisation du placement source.

## Conclusion après rechargement : le comptoir est déjà dynamique

Les résultats ci-dessous remplacent la priorité donnée plus haut à la recherche
d'un transfert. Ils concernent ce comptoir dans la zone observée, pas tous les décors.

Lecture de la même sauvegarde rechargée : le comptoir cible possède l'EntityId
2758, sans bit d'entité de scène, et la famille native **8 (Prop)**. Les coffres
magiques voisins sont également dynamiques, de famille **3 (Container)**.
Un autre exemplaire du même comptoir possède au contraire l'identifiant de scène
2626945700. Le modèle seul ne suffit donc pas à déterminer son régime de persistance.

Le placement source 1451 du chunk 417 présente les bits suivants : +0x38 faux,
+0x58 vrai, +0x70 faux. Le parcours de création de scène `0x9d1e10` filtre les
placements par volumes via `0x9d0820` et marque +0x58. Ce chemin ne crée pas
l'instance source ordinaire. Il est distinct du marqueur de suppression +0x38,
que `0x9a2a10` peut écrire lors d'une destruction d'entité de scène.
**Ne pas écrire ces bits ni détruire le placement source pour déplacer ce comptoir.**
L'association de ces volumes aux claims est cohérente avec les relevés ; leur
construction complète n'a pas été retracée.

Témoins reproductibles : `work/read-decor-persistence.py`,
`work/decor-persistence-live.json`, `work/decor-target-reload.json`.

### Sauvegarde : même répartition native, sérialiseurs par famille

- Le répartiteur `0xe187b0` sélectionne la famille : Container vers `0xdc0a50`,
  Prop vers `0xdc1d00`.
- Le wrapper Prop applique notamment les conditions de zone (`0xde9e80`) et
  appelle le sérialiseur `0xdc4ef0`.
- Ce sérialiseur copie les **56 octets de CurrentTransform** à
  `0xdc509e`–`0xdc50be`, ainsi que la référence de modèle. `UsedItem` n'est pas
  une condition obligatoire de ce parcours ; CurrentTransform l'est.
- Les appels communs `0xe18500`/`0xe18670` récupèrent l'enregistrement et sa
  famille avant de passer au répartiteur. Il ne faut pas appeler le sérialiseur
  directement depuis le mod.

Il existe donc un canal natif pour sauvegarder la position d'un Prop dynamique.
Le journal des destructions de scène n'est pas son unique mécanisme de persistance.
La validation du résultat après un déplacement effectif reste un test en jeu.

### Confort : mise à jour spatiale native identifiée

La collecte `0x2539a0` crée une requête AABB filtrée sur ComfortProvider (hash
`0x8f00bc3c`) via `0x9600c0`. `0x960390` détruit la requête ; ce n'est pas une
fonction à appeler pour actualiser le confort.

La mise à jour ECS `0xa2c660` prépare la tâche `0x9d7140`, qui répartit le
traitement des entités entre `0x9952e0` -> `0x9ac530` et la finalisation
`0x9952d0` -> `0x9b1090`. Le traitement lit la position courante en 32.32 à
`0x9ac890`, calcule la cellule, la compare à l'ancienne à `0x9ac90f` et enregistre
le changement. La finalisation retire l'ancienne entrée via `0x995680` et ajoute
la nouvelle via `0x960a90` (`0x9b12d6`–`0x9b130c`). La requête AABB
`0xa29330` lit également CurrentTransform pour tester les candidats.

L'écriture doit passer par l'accès natif mutable `0x9b2930`, qui notifie le
moteur via `0x9d3280`, comme le commit des coffres. Une écriture brute sur un
pointeur obtenu en lecture contournerait ce suivi. Ne pas modifier manuellement
le total de confort ni recréer ComfortProvider.

### Adaptation concrète à réaliser dans CarryOn

1. Autoriser explicitement ce modèle Prop dynamique dans la sélection, avec
   CurrentTransform, collisions et rendu valides ; refuser pour cette première
   étape son homologue encore identifié comme entité de scène.
2. Séparer le descripteur de placement de la présence d'UpgradeStorage et de
   UsedItem : ces exigences du chemin coffre excluent actuellement le comptoir.
   Utiliser le modèle, les dimensions et les collisions de l'entité existante.
3. Garder l'aperçu et le solveur natif, sans toucher à l'original avant validation.
   Imposer la même claim pour origine et destination.
4. Réutiliser le commit de transformation, les collisions serveur et la
   synchronisation rendu/collisions client déjà utilisés pour les coffres.
   Conserver la même entité et ses composants, notamment ComfortProvider.
5. Contrôler l'état AttachToSurface/DynamicAttachToSurface et son sommeil lors
   de l'intégration : leur présence est établie, pas leur réaction à notre commit.
   Ne pas les supprimer arbitrairement ni inventer leurs offsets d'écriture.
6. Laisser les tâches spatiales et le calcul de confort natifs traiter la nouvelle
   position, puis laisser la sauvegarde Prop suivre son parcours habituel.

Critères de validation : annulation sans changement ; collision et image au même
endroit ; confort évalué autour de la nouvelle position (tenir compte du maximum
par catégorie) ; sauvegarde, retour au menu et rechargement avec exactement un
comptoir à destination et aucun à l'origine ; second déplacement après reload.
Tester également une destination dans une autre cellule spatiale de la même claim.

État à la fin de la recherche : aucun patch de jeu, changement de sauvegarde,
installation ou essai de pose effectué. L'intégration a ensuite été réalisée
dans la 0.1.30 avec le descripteur ItemInfo natif retrouvé pour ce modèle ; voir
[DEPLACEMENT-COMPTOIR.md](DEPLACEMENT-COMPTOIR.md) pour le périmètre, les tests
hors jeu et la validation en jeu encore requise.
