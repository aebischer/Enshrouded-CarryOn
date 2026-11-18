Note historique : ce document décrit les étapes de développement à sa date.
Pour le fonctionnement actuel et les limites connues, consulter ../README.md.

# Ciblage de la pince — intégration 0.1.1-dev

Client inspecté : SHA-256
`5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77`.
Les adresses ci-dessous sont des RVA de ce client, pas une API stable.

## État

Le raccordement natif est implémenté dans `src/native_transport.inl`, avec la
politique dans `transport_selection.h`. Validation hors jeu effectuée ; rendu
et variantes de coffres à vérifier par le joueur. Ne pas présenter les tests
simulés comme une validation en jeu.

Installée le 14 novembre 2026, jeu fermé, avec sauvegarde des deux anciens
fichiers. SHA-256 installé :
`650C9EA30C7F561E905DC2211D3B713BCFB772F292BAA808D9C946EDA939F930`.
Reçu et emplacement de sauvegarde : `../work/carryon-transport-install.txt`
dans le dépôt parent (fichier local, non publié).

Le hook `0x292c50` s'exécute après le helper natif du curseur, sur son contexte
de ligne vivant. Il lit l'objet équipé, l'écran et la cible d'interaction (2).
Il filtre les entités possédant `UpgradeStorage`, avec une limite supplémentaire
de 3 mètres. Ce filtre vise les coffres améliorables et ne prétend pas couvrir
les conteneurs de butin ni toutes les variantes du jeu.

Le surlignage utilise uniquement un `HighlightColor` dynamique existant de
taille 1 et la couleur valide du curseur. L'écriture passe par `0x9bf460`, qui
marque la modification native. L'ancienne couleur est restaurée à la sortie,
sauf si un autre effet l'a remplacée. Aucun pointeur d'entité n'est retenu d'une
image à la suivante. L'absence d'un composant graphique compatible est journalisée.

Le hook clavier `0xfa3550` reçoit les `RAWKEYBOARD` après `GetRawInputData`.
Tab est consommée uniquement pour la pince avec l'écran InGame au premier plan ;
Échap seulement en mode transport. La capture conserve les paires enfoncement /
relâchement, n'intercepte pas une touche déjà maintenue par le jeu et ignore les
répétitions. Les fronts sont conservés jusqu'à l'image suivante. Les autres
touches passent au traitement natif intactes. La remappabilité et la manette ne
sont pas encore prises en charge.

## Travail du 14 novembre 2026

La ressource native du marteau conserve `ItemCategory::Blueprints` (10), même
après retrait du drapeau `IsBlueprint`. Ses indications à l'offset 152 ont une
action tertiaire héritée, liée à `UiBuildingMenu` (129), affichée avec Tab.
La copie de la pince est désormais de catégorie Tools (3), avec ses propres
identifiants de texte dans les 15 langues. Le libellé tertiaire suit l'état
du mode : entrer / quitter. Le clavier est raccordé séparément de la localisation.

La politique isolée teste les fronts de Tab, menus, focus, changement d'outil,
changement de session, identité complète de cible, portée, obstruction et
désactivation des coffres. Ces tests ne valident pas le moteur.

Compilation `/W4 /WX`, tests ABI/configuration, tests de politique, tests de
ressources natives et validation statique du paquet réussis. L'adaptateur est
testé avec des substituts des fonctions natives, y compris le marquage d'écriture,
la restauration de couleur, un effet concurrent, un composant absent, le clavier,
les menus, le focus, la configuration et l'expiration des données. Aucun test
visuel effectué, conformément au souhait du joueur.

Lecture mémoire autorisée sur le processus 19044 avec la pince tenue devant
un coffre : plusieurs copies d'ItemInstanceInfo retrouvées (slot observé 5,
inventaire `0x203f`), notamment dans les piles et historiques. Cela ne prouve
pas laquelle est l'instance active ni l'identité du coffre. Ne pas utiliser les
adresses trouvées par balayage dans le mod.

Résolveur statique vérifié : `0x9b1db0` recherche un type par hash dans les maps
du registre (`+0x30`, puis `+0x88`). `0x9b2b50` lit un composant dynamique avec
un résultat pointeur/taille de deux mots. Le monde d'un contexte de système
est le pointeur à `context[0]+0x30`. Pour les composants statiques, le chemin
est différent (`0x9bfca0` lit une référence de template).

Le helper `0x292c50` est appelé depuis `client_cursor` avec le contexte de ligne
déjà initialisé. À l'inverse, l'entrée de `player_ui` de ce client appelle
`iterator_init` puis `iterator_next` : ne pas reprendre sans adaptation le
hook d'entrée de la référence Enshwick d'une autre version.

## Observations

- `client_cursor` : RVA `0x294620`, contexte d'itération `0x218` octets.
  Lit notamment `SlotSelection`, `NetworkCursor`, `NetworkSelectedTargets`.
- `client_cursor_entity` : RVA `0x2963c0`, contexte `0xc0` octets.
  Modifie le curseur et accède à une couleur de surlignage. L'accès observé
  utilise `NetworkCursor.cursorEntityId` : ne pas confondre cette entité de
  curseur avec le coffre réel visé.
- `player_picking_target` : `0x47e8d0` ; `player_target_selector` :
  `0x47f190` ; `copy_ui_targets` : `0x47e530`.
- `SlotSelection.actionItem` commence à `168`, son `itemId` à `176`.
  L'identifiant de la pince est `0xb5e0e52b`.
- `NetworkCursor` distingue `cursorEntityId` à `12`, `hoveredObjectId`
  à `16` et `selectedObjectId` à `32`.
- `NetworkSelectedTargets.targetIds` contient 16 `GameObjectId` de 16 octets.
  Les noms de catégories du code de référence sont Interaction (2),
  Dismantle (4), DismantleFallback (5), Picking (10). Leur pertinence et leur
  valeur effective dans cette situation doivent être vérifiées en mémoire.
- Les champs réfléchis `ClientCursor.previousSelectedEntityId` (192),
  `previousSelectedEntityTintColor` (196) et
  `CursorConfig.highlightHoveredEntity` (84) ne prouvent pas à eux seuls
  l'existence d'un chemin natif utilisable pour les coffres.

## Conditions avant installation

Confirmer sur un coffre posé l'identifiant de la cible et la détection de la
pince tenue. Identifier un critère de coffre qui exclut les autres conteneurs.
Valider l'accès graphique, la durée de vie de l'entité et la restauration de
son état, sans conserver un pointeur de composant entre deux images.

Respecter `move_chests`, les obstacles et la portée. Effacer la sélection à
la sortie de visée, au changement d'outil et à l'ouverture d'un menu.
Ne réactiver ni `IsBlueprint`, ni les séquences du marteau, ni la destruction.
Le survol ne doit modifier ni inventaire ni sauvegarde.

Les tests hors jeu devront couvrir les changements de cible, les cibles
invalides, la configuration désactivée et la restauration du rendu. Les tests
visuels en jeu restent à la charge du joueur, comme demandé.

## Correctif Tab — 0.1.2-dev

Le retour en jeu a révélé que Tab ne déclenchait rien. La condition de l'adaptateur
confondait HudScreenType::Invalid (0) avec InGame (1). La table réfléchie de
l'exécutable pris en charge, descripteur RVA 0x1bb3690, confirme InGame=1.
Le filtre reste strict : seul cet état accepte le mode transport.

Les tests du contexte synthétique initialisaient le champ à zéro et reproduisaient
la même erreur. Ils utilisent maintenant la valeur native explicite 1, vérifient
le refus de tous les 53 autres états (0, 2 à 53), et font passer un appui/relais
Tab par la file d'événements clavier jusqu'à l'adaptateur de frame.
Compilation, tests ABI/politique, tests natifs et validation du paquet réussis.
La confirmation du comportement visuel reste à effectuer par le joueur.

## Diagnostic Tab — 0.1.3-dev

Le joueur confirme que 0.1.2 ne répond toujours pas à Tab. La correction de
l'énumération ne prouve donc pas que le raccordement fonctionne.
0.1.3 ajoute une trace au premier callback de curseur et une trace limitée aux
24 premiers événements Tab pressés : compteur de callbacks, ID d'outil lu,
écran, focus, éligibilité, consommation et ancienneté de la dernière frame.
Aucun texte saisi ni contenu mémoire arbitraire n'est enregistré.
Tests hors jeu et validation du paquet réussis avant installation.
Le diagnostic en jeu est en cours ; ne pas présenter cette version comme une
correction fonctionnelle confirmée.

## Outil sélectionné — 0.1.4-dev

Diagnostic réel 0.1.3 après deux appuis : rows=1863/1888, item=0,
screen=1, focus=1, eligible=0, consumed=0, age_ms=15/16. Le clavier et
le callback de curseur fonctionnent ; actionItem ne décrit pas ici l'outil tenu.

La reconnaissance suit désormais SlotSelection.actionbarSlotSelection (octet
192) et actionbarEntityIds (160, 164). Indices 0..7 : première entité ;
8..15 : seconde, index local moins 8. Ce calcul est visible au RVA 0xea4637.
Lecture seule du composant Inventory (hash 0xb19528a9, taille 96), huit ItemStack
(taille 12, ItemId à 0). Aucun pointeur d'inventaire conservé entre les frames.
Les identités absentes et indices >=16 sont refusés.

Régression : pince sélectionnée avec actionItem=0, les seize emplacements,
les deux barres, taille/entité/slot invalides, et ancien actionItem=pince alors
que la sélection est devenue marteau. Les tests hors jeu passent.

## Surlignage — 0.1.5-dev

Le joueur confirme Tab et son libellé. Journal réel : cible coffre 1101, mais
highlighted=0. L'ancien message « no writable native highlight » était ambigu :
il couvrait aussi une palette de curseur à zéro.
Le mode transport utilise maintenant HighlightColorIndex::ValidState=2,
confirmé dans le descripteur PE RVA 0x18659b0, indépendamment de ClientCursor.
Test de régression : ClientCursor.highlightColorValid=0 et couleur initiale 0,
application de 2 puis restauration ; les autres effets restent préservés.
Messages distincts pour application réussie, composant absent et accès mutable
refusé. Tests hors jeu et paquet validés avant installation. Rendu en attente
de confirmation par le joueur ; ne pas déduire du journal précédent que le
composant est présent.

## Coffres sans HighlightColor — 0.1.6-dev

Essai 0.1.5 : cible 1096 valide, aucun composant HighlightColor dynamique.
Le chemin ECS ne peut donc pas dessiner le contour du coffre testé.
Ajout d'un chemin de rendu sans modification de l'archetype : ModelComponent
(taille 4, descripteur 0x1869430), commande RenderScene de couleur au RVA
0xf6e90, avec deux spans pointeur/taille (handles uint32, couleurs uint8).
Raccordement à static_render_model (0x319cc0), contexte natif 0x58 et RenderScene
à +0x50 initialisé par iterator_init (0x9c7b80). Modèle résolu à nouveau pour
chaque mise à jour ; pas de pointeur de composant conservé. Effet réservé aux
cibles sans HighlightColor, qui ont une couleur native par défaut 0.

Tests : dessin et renouvellement, retrait sur annulation, modèle disparu,
expiration de la cible, changement de monde, aucune écriture de composant.
Compilation /W4 /WX, tests ABI/politique/ressources et validation paquet passent.
La confirmation visuelle reste à effectuer par le joueur.

## Diagnostic coffre magique et parcours voxel — 0.1.7 / 0.1.8

0.1.6 : la cible 1107 est sélectionnée, aucun log de commande de rendu.
0.1.7 installée pour distinguer callback absent, monde/scène incorrects,
ModelComponent absent et VoxelModel présent. Confirmation joueur en attente.

0.1.8 préparée : VoxelModel (descripteur 0x18710f0, taille 8) possède un
renderModelHandle uint32 à 0 et renderBricksHandle uint16 à 4. Le premier
utilise le même gestionnaire de modèles que ModelComponent. Le système
render_voxel_model (RVA 0x315a20) transmet à chaque image une couleur de
HighlightColor, ou 0 quand le composant est absent (0x315e56..0x315ed8).
Ajout d'un hook après ce système pour éviter l'écrasement de la couleur.
Contexte de ligne 0x60, RenderScene +0x58, confirmé par les appels natifs.

Tests : lecture des deux formats, retrait de l'effet voxel, exécution après
le système natif qui remet à zéro, annulation, expiration et disparition.
Compilation et tests hors jeu passent. Ne pas présenter comme validé en jeu.

## Relevé réel 0.1.7 et correction du point de rendu

Coffre magique visé le 14 novembre : entity=1110, ModelComponent=16777718,
VoxelModel=0. Le hook static_render_model est atteint au chargement, mais
aucun callback de frame avec cible n'est reçu ensuite. Son ordonnancement
ne convient donc pas à un effet continu sur les modèles déjà créés.

0.1.8 remplace ce hook par client_cursor_entity (0x2963c0), qui met à jour le
curseur du joueur. Ligne 0xc0 ; EcsFxController à +0x70 et RenderScene à +8
de ce contrôleur, vérifiés via 0x29658d et 0x29d9e0 (appel RenderScene+8 vers
0xf6f80). Aucune adresse de tas mémorisée. Les tests de wrapper couvrent cette
indirection et l'application après la fonction native.
Le support voxel préparé reste disponible ; il ne décrit pas ce coffre magique.
Compilation et tests natifs réussis avant installation ; rendu à confirmer.
