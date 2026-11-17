Note historique : ce document décrit les étapes de développement à sa date.
Pour le fonctionnement actuel et les limites connues, consulter ../README.md.

# Déplacement des coffres : travail en cours — 14 novembre 2026

## Comportement demandé

- La pince équipée montre les limites ; Tab ouvre le mode transport.
- Saisir verrouille une cible et ouvre un aperçu ombré dans le style du contour.
- Le coffre réel, ses inventaires et ses collisions restent à leur place pendant l'aperçu.
- Le calcul de position, l'accrochage, la rotation et les refus doivent employer le placement natif du marteau.
- La pose est limitée à la zone du même autel ; une autre zone adjacente ou superposée ne suffit pas.
- Une pose confirmée déplace l'entité existante. Aucun démontage, butin, consommation, duplication ou conversion en item.
- Annuler, changer d'outil/monde, perdre le coffre source ou ouvrir un menu supprime uniquement l'aperçu.
- Téléportation et transport entre zones reportés. Tests en jeu effectués par Alain.

## État exact

0.1.19-dev : le test réel 0.1.18 montrait les aides, mais molette/Q restaient
affectées à la barre d'outils et à l'arme à distance. Le journal confirmait que
0x2477d0 recevait déjà l'ItemInfo du coffre : cette intervention était trop tardive.
Le nouveau raccordement agit avant 0xec24b0 (évaluation des bindings), via l'API
native 0xe85800 et son getter 0xea2ab0 : actions 35/36, 93..104, 108/109 actives ;
83/84 et 117..119 inactives pendant la saisie. Pas de redimensionnement 105/106.
Le contexte mapping est publié uniquement par le callback joueur validé. Garde
de focus, mode, monde/cancellation et fraîcheur de 250 ms. Les décisions natives
sont conservées par contexte/thread et restaurées à la prochaine évaluation de
jeu inactive ; une modification native pendant la saisie remplace le baseline.
Aucun pointeur d'ancien contexte n'est déréférencé pour le restaurer.
Les tests observent les flags avant l'évaluation, les conflits molette/arme,
les touches non concernées, les nouvelles décisions natives et la restauration.
Ils ne remplacent pas les essais de commandes réelles en jeu par Alain.
Compilation /W4 /WX, tests et validation du paquet réussis ; installée jeu fermé.
SHA256 : `6CD1A515AD64DE2A5EC96AA81BC56BC31072858C5A7F8D5320A62AC64899D138`.
Ancienne DLL : `work/carryon-backup-20261114-174832`.

0.1.17 confirmée en jeu par Alain : aperçu et déplacement visuel fonctionnent.

0.1.18-dev raccorde le traitement des commandes natif 0x2477d0 à l'ItemInfo du
coffre pendant Preview. Uniquement domaine gameplay 0, acteur identique à celui
de la requête, focus et contexte de placement frais (250 ms). L'appel original
reste unique et conserve tous ses arguments hormis le dernier ItemInfo ; l'outil
équipé, les programmes d'action, l'acteur et ses permissions ne sont pas modifiés.
L'ItemInfo provient de la requête native et reste dans son registre (références
relatives) ; les pointeurs de prêt sont effacés à l'annulation. Les commandes
et incréments restent ceux du jeu ; la taille du coffre reste celle d'origine.
Le HUD ajoute les glyphes natifs 93/94/101 et 102/103/108 pendant la saisie,
avec quatre libellés traduits dans les 15 langues. Les tests simulent le routage
actif/inactif, focus, fraîcheur, acteur, domaine et le transport d'une rotation
du solveur jusqu'au coffre, au rendu et aux collisions. Ils ne valident pas les
touches ni la disposition réelle du HUD : essais en jeu réservés à Alain.
Compilation /W4 /WX, tests et validation du paquet réussis ; installée jeu fermé.
SHA256 : `E3683A8FDEE765BEDDDD366B8B474AE0DFBBDCD4B83D5DF74C20DD41D757249F`.
Ancienne DLL : `work/carryon-backup-20261114-162703`.

0.1.17-dev : correction graphique après l'essai réel 0.1.16. La pose serveur
et la collision fonctionnaient, mais le modèle animé restait à l'origine et
l'aperçu était invisible. L'aperçu initialise maintenant sa pose sans contrôleur
d'animation via 0xf6eb0(scene, handle, true), comme static_render_model 0x319e81
lorsque ClientAnimationGraphInput est absent. Le modèle du vrai coffre conserve
son handle et son animation : après réception de CurrentTransform, le client
met à jour les trois WorldTransform de RenderTransform (176 octets, ticks
préservés), puis appelle 0xf7270 sur le modèle existant et actualise sa collision.
La file reste en attente si la scène, le modèle ou l'accès en écriture manque.
Tests hors jeu : pose statique du fantôme, absence de scène, écriture de taille
incorrecte, trois poses synchronisées, ticks conservés et application unique.
Le rendu et l'animation après pose restent à vérifier en jeu par Alain.
Compilation /W4 /WX, tests natifs, tests de politique et validation du paquet
réussis. Installée jeu fermé ; SHA256
`711CCC0830EA2ADB15E2E83EB55C5DC79AA4C554718F1B288C7630EF3E2E683D`.
DLL précédente sauvegardée dans `work/carryon-backup-20261114-160744`.

0.1.16-dev compilée, testée hors jeu et installée après fermeture confirmée.
SHA256 installé : `36472076100A8BD534419B8DC823C6DCCD3B908BAF6387B2E5C5E796220484B4`.
DLL précédente : `work/carryon-backup-20261114-145751`.
L'essai 0.1.15 a confirmé une saisie reconnue puis annulée après
deux secondes sans aperçu : le filtre du curseur natif exigeait un actionItem
pince, alors que cet outil n'en possède pas. Le filtre vérifie désormais l'outil
réellement équipé via SlotSelection, indépendamment de la configuration native.
Une seule requête client est calculée par ligne de curseur ; la validation
serveur conserve ses refus. Les régressions couvrent un actionItem absent ou
différent, le drapeau primaire absent, le double rappel et le changement d'outil.
Compilation /W4 /WX, tests natifs et validation du paquet réussis. Le rendu et
la pose restent à confirmer dans le jeu ; aucun déplacement réussi n'est établi.

0.1.15-dev installée après compilation /W4 /WX, tests de politique, tests de
l'adaptateur natif et validation du paquet. SHA256 installé :
`6BEE10C2E2D5E28E5051F67C47E4E3955A0581FDB9EC405E627AEE1EEB394AF9`.
Ancienne DLL : `work/carryon-backup-20261114-144642`.
Copie préalable des sauvegardes Steam et locales, 156 fichiers vérifiés par
empreinte : `work/carryon-saves-before-0115-20261114-144617`.
Jeu fermé pendant la copie et l'installation ; aucun test de déplacement en
jeu effectué par l'agent. Alain réalise les essais visuels et de persistance.

La version 0.1.15-dev raccorde la politique de transaction au moteur dans
`native_transport_runtime.inl`. Clic gauche : saisir ; clic droit : poser à la
destination validée ; Tab/Échap : annuler et quitter. Le coffre d'origine reste
immobile pendant l'aperçu. Le modèle d'aperçu n'a ni inventaire ni collision.

Le calcul client réutilise configuration, requête et solveur natifs du marteau
avec une copie locale de l'état de construction. Aucun état réel de construction
n'est activé. La validation serveur utilise le masque natif de permissions sans
le convertir en booléen. La pose au post-step vérifie encore l'identité, la
transformation source, les liens de stockage, l'outil, le verrou d'interaction,
la claim et l'étape serveur de validation avant de déplacer l'entité existante.
Les collisions statiques sont actualisées par le gestionnaire natif ; le
CurrentTransform passe par l'accesseur qui signale le changement au moteur.

Les tests hors jeu simulent ces appels et couvrent le parcours complet ainsi que
les refus. Ils ne prouvent pas le rendu, la réplication ni la persistance dans une
vraie partie : ces essais restent à faire par Alain. À ce stade, seules les
collisions observées sur le coffre magique ont été confirmées en lecture réelle.
Limites : session locale, commandes souris par défaut, pas de téléportation ni de
serveur dédié. Une destination chevauchant la collision du coffre original peut
être refusée par le placement natif.

## Historique des versions de diagnostic

La version intermédiaire 0.1.12-dev, installée après les tests hors jeu, ajoute une observation du
post-step serveur. Elle transmet des adresses de diagnostic et le thread courant,
sans parcourir les inventaires ni changer de composant. Son but est d'identifier
le monde autoritaire avant de brancher la transaction. Elle ne fournit pas
encore la mécanique de déplacement. Un simple mock du callback ne valide pas
son déclenchement réel dans le jeu.

## Relevé du coffre magique réellement visé

Lecture seule, DLL installée identifiée par comparaison de sa section `.text`
avec une carte de symboles reconstruite. Les adresses de tas restent locales
sous `work/` et ne servent pas de constantes dans la DLL.

- CurrentTransform : 56 octets, position 32.32 + quaternion + échelle.
- RenderTransform : 176 octets ; copie de la transformation côté rendu.
- ModelComponent : 4 octets ; pas de VoxelModel pour ce coffre.
- StaticClientCollider : 16 octets ; StaticTransform présent.
- UsedItem : 4 octets, ID relevé 0x0cc076a8.
- InventorySetup : 84 octets. Trois entités d'inventaire liées, 24 emplacements.
  Aucun contenu d'inventaire lu ni modifié pendant le relevé.
- UpgradeStorage est dans la partie statique : la taille zéro du lecteur
  dynamique ne veut pas dire que le composant est absent.

## Points natifs identifiés (exécutable épinglé seulement)

- 0x28c430 : préparation de la configuration de curseur à partir des ItemInfo.
- 0x2905c0 : requête de placement.
- 0x2994d0 : calcul de transformation et drapeaux de validité.
- ClientCursor : transformation principale +0x10, drapeaux +0x80.
- 0x2963c0 : rendu natif du curseur ; dépend aussi de NetworkCursor et de son
  entité d'aperçu. Remplacer seulement le texte ou les drapeaux ne crée pas
  un aperçu de coffre.
- 0xea3610 : inclusion d'une boîte dans une zone. Parcourt des entrées de 0x48
  octets, pointeur +0x10, nombre +0x18 ; 0xea3ab0 teste un point.
  Cette fonction seule ne garantit pas « même autel ».
- 0xe66060 : post-step serveur, confirmé par l'ancre et le prologue ; appel
  après le pas du monde depuis 0xe60af6. Server +0x1d0 est le World complet,
  pas directement la table d'entités. Ne pas confondre les deux.
- 0xa1ebf0 / 0xa1ee20 : création des collisions statiques serveur/client.
- 0x319cc0 : création du modèle statique ; ne constitue pas une API de
  déplacement. Réexécuter aveuglément ces créations peut laisser des handles.
- `move_colliders` 0xa1d0c0 utilise Collider, pas StaticCollider ; ne pas
  appliquer son ABI au coffre par simple analogie.

## Étapes initiales de recherche

1. Résolution de l'objet serveur correspondant, de son autel et de ses liens.
2. Aperçu rendu indépendant, avec destruction propre lors de toute annulation.
3. Réutilisation isolée du placement sans activer construction/destruction natives.
4. Changement de transformation serveur avec suivi natif des changements,
   collisions, accrochages et sauvegarde ; conservation des mêmes inventaires.
5. Revalidation serveur juste avant la pose et rejet des requêtes périmées.
6. Tests de l'adaptateur, build, ressources, paquet avant toute installation.

Le code de référence Enshwick suggère que l'identité persistée d'un objet placé
est son template et sa position, et non un GUID unique sauvegardé. C'est une
piste à vérifier sur ce client : la transaction utilise une session, une entité
avec génération, un template et la transformation source, jamais un ID supposé
stable après rechargement.

## Relevé serveur confirmé et préparation 0.1.13-dev

Le post-step 0.1.12 a été observé dans le jeu. Le World serveur contient son
EntityWorld à +0xe179c0. Le coffre magique visé a la même entité, le même UsedItem
et les mêmes liens InventorySetup côté client et serveur. Son template relevé
est `90a8ba652a024b459cb7b92cb8c06cd4`. Aucun contenu d'inventaire n'a été lu.
La transformation client peut différer légèrement de celle du serveur : prendre
la référence de transaction côté serveur, sans comparaison exacte client/serveur.

Les entrées de claim contiennent le GUID à +0x30 et l'entité autel à +0x40.
La politique de transaction vérifie maintenant les deux identités, notamment
pour les claims qui se chevauchent. Les tests couvrent l'autel remplacé.

`native_transport_placement.inl` et `native_transport_snapshot.inl` sont à ce
stade compilés uniquement dans les tests. Ils vérifient la propriété de l'aperçu,
son annulation, les huit coins dans la claim et la copie des liens de stockage.
Ils ne sont pas raccordés aux entrées joueur ni à la DLL distribuée.
Les couleurs natives sont InvalidState=1, ValidState=2, MutedState=3.

L'accesseur d'écriture natif a été identifié à 0x9b2930, via animal_pet_interaction
(0x230118 -> 0x9b2900 -> 0x9b2930). Le change tracker est à EntityWorld+0x7bc030,
confirmé par l'itérateur 0x9c2780. Ces fonctions ne sont pas encore appelées par
CarryOn pour déplacer un objet.

La 0.1.13-dev ajoute uniquement une observation en lecture seule de move_colliders
(0xa1d0c0). Sa ligne de dépendances fait 0x70 octets, PathGridSystem à +0x60 et
ColliderSystem à +0x68. Elle distingue les mondes client et serveur déjà connus,
ignore les autres et conserve les adresses pour diagnostic seulement.
Ce relevé doit confirmer le gestionnaire propriétaire du StaticCollider avant
d'utiliser la mise à jour native 0xa1fdc0. Ne pas confondre cette fonction avec
la création initiale 0x97f810 ; répéter la création ne déplace pas proprement
un handle existant. La saisie et la pose restent désactivées.

### Résultat réel du diagnostic 0.1.13

Installation vérifiée : SHA256
`E28B7B5B45531545784DBAC3A9F4FEC2C0BBE6E6EC938584BCD7A29165CE142B`.
Relevé effectué pendant que le joueur gardait le jeu au premier plan, mode
transport actif. L'entité sélectionnée était 1211 dans cette session (les IDs
de session précédents ne sont pas réutilisables). Identité, UsedItem et les
84 octets d'InventorySetup identiques côté client et serveur ; ressource Model
`6b2414d18acc6e4f8adff952d1cd0b9e`. Handles de collision : client 486, serveur 157.

Les deux observations `transportServerCollision` et `transportClientCollision`
sont restées entièrement à zéro. Le relevé des gestionnaires de collision a
donc échoué ; ne pas présenter la pose comme débloquée. Ce constat ne permet
pas de distinguer un système non exécuté d'un rejet du filtre de monde.
Le test mock validait le callback, mais pas sa fréquence ni le moment réel
d'exécution. Il faudra notamment vérifier les observations lors de la création
des collisions et avant que les mondes client/serveur soient identifiés.
Les fichiers structurels locaux sont `work/carryon-chest-{client,server}-0113.json`
et `work/carryon-collisions-0113.json`. Aucun déplacement ni écriture mémoire
effectué par les scripts. Perdre le focus annule le mode transport : le joueur
n'a pas besoin de revenir répondre pendant une capture.

### Diagnostic corrigé 0.1.14 et intégration 0.1.15

Les observateurs sont maintenant sur `static_collider` (0xa1ebf0) et
`static_collider_client` (0xa1ee20), sans attendre l'identification du monde.
Leurs lignes font respectivement 0x68 et 0x60 octets ; ColliderSystem se trouve
à +0x58 et +0x50. PathGridSystem serveur est à +0x50. Les lectures de diagnostic
vérifient une séquence de publication et le monde courant plutôt qu'un délai
de cinq secondes : ces systèmes ne sont pas appelés chaque frame.

Le relevé 0.1.14 a confirmé le coffre magique (entité 1217 dans cette session),
le handle serveur 163, le handle client 558, les deux propriétaires 1217 et
la même ressource de collision. Les deux records ont le drapeau statique 1.
Le SlotSelection serveur fait 216 octets et la pince sélectionnée était bien
résolue dans l'inventaire de barre d'action. Le verrou du coffre fermé vaut 0.
Ces identifiants ne sont pas des constantes de production.

La version 0.1.15 utilise 0x9c3d70(iterator, null, null) pour l'entité courante.
0x9c3cd0 renvoie un autre objet du contexte : il ne doit pas servir d'ID joueur.
Le solveur reçoit une copie de contexte de 0x6a0 et de ClientActor de 0xc38
octets ; seuls les bits de mode de la copie sont modifiés. Le masque de droits
et l'octet de résultat du raycast sont conservés sans conversion booléenne.
Les refus de matériaux et de déverrouillage de recette ne s'appliquent pas au
déplacement d'un coffre existant ; les autres refus natifs restent actifs.

Les tests couvrent aussi les boutons maintenus avant l'entrée du mode, le clic
droit, la boîte tournée, le bord de claim, la péremption de validation, le
changement de verrou/outil/template/stockage, le double commit et les quaternions
équivalents après réplication. Les appels natifs sont substitués pendant ces
tests ; ils ne constituent pas un essai de déplacement dans le vrai jeu.


## 0.1.20-dev — contexte natif des commandes de placement

Analyse du parcours normal dans le binaire pris en charge : 3ab140 construit un contexte de 0x63 octets, puis 3c47b0 choisit les actions avant leur évaluation. ItemInfo à +0x58 détermine les contraintes de rotation et translation. La version 0.1.19 activait séparément les actions, sans reproduire cette décision native.

Remplacement des hooks e85800/ec24b0 par 3c47b0 : copie locale du contexte, substitution du seul ItemInfo par celui du coffre transporté, puis appel natif unique. Les pointeurs des composants, les états de menu et de blocage sont conservés. Contrôle du personnage, aperçu récent, focus, mode et paramètres avant substitution. Aucun état de construction écrit sur le personnage réel. Le hook 2477d0 conserve la même substitution pour calculer les commandes du coffre.

Compilation et tests du SDK, des transactions et des adaptateurs natifs réussis. Tests du contexte : copie source intacte, tous les autres octets inchangés, refus sur aperçu périmé, perte de focus, autre acteur, mode inactif et paramètres désactivés. Ces tests emploient des fonctions natives simulées : Q/R/molette/X restent à confirmer en jeu par Alain.

## 0.1.21-dev — modèle 3D de la pince

Modèle FBX Meshy fourni par Alain : 140 174 sommets, 280 400 triangles. Source conservée dans assets/pincer3d/source. Version de travail Blender 4.5.9 portable : poignée redressée, hauteur environ 62 cm, origine à 28 % de la hauteur depuis le bas, trois LODs 5 000 / 1 500 / 500 triangles. Textures couleur et normale 1024², RMA assemblée (métal / rugosité / occlusion blanche), émission noire. Le point de prise doit être confirmé en jeu.

Ressources privées RenderModel, RenderMaterialResource et TemplateResource, et huit contenus binaires embarqués dans la DLL par RCDATA 401. ItemInfo.equipment.visualEntity et equipmentEntityModelSetup.model référencent exclusivement la pince. Le modèle natif du marteau reste intact. L’attache SlotAttachment est conservée ; les deux composants VFX du marteau sont supprimés du clone. Les lectures du contenu sont servies par le hook générique existant, avec contrôle des bornes et remplissage des fins de blocs 4 Kio.

Format vérifié contre la réflexion locale et le lecteur indépendant EnshroudedBlenderTools : https://github.com/Baik90/EnshroudedBlenderTools (référence conservée dans work/enshrouded-blender-reference). Aucun code de cet outil n’est lié à la DLL ; son décodeur est employé pour vérifier les données générées. Outils : tools/prepare-pincer-model.py (Blender), tools/pack-pincer-model.py, tools/test-pincer-model.py. Compression des textures : texconv Microsoft DirectXTex may2026. Le pack source est compilé par build.cmd, aucune application supplémentaire nécessaire chez le joueur.

Validation hors jeu : compilation /W4 /WX, tests du SDK et transactions, tests des adaptateurs natifs, parcours des ressources et contenus privés, conservation de l’attache, suppression des VFX, refus des plages invalides et protection des buffers. Décodage indépendant : tous les LODs, indices, normales, UV et mipmaps ; erreur maximale de quantification des positions < 0,000001 m. Prévisualisation Blender disponible dans assets/pincer3d/preview.png. Rendu et prise en main dans Enshrouded non encore validés ; essai confié à Alain.

## 0.1.22-dev — enregistrement de l’entité visuelle

Retour Alain : marteau disparu, pince invisible. Journal 0.1.21 : ressource TemplateResource privée chargée (entrée 8), aucune demande du RenderModel (entrée 6), du matériau (entrée 7) ou des contenus 3D. Relevé en lecture seule : pince équipée (b5e0e52b), aucune entité du template privé dans le monde client. Le jeu construit la liste des templates ECS au chargement du monde ; le hook list ne complétait que UiTextureResource.

La liste native des TemplateResource inclut désormais le template privé, avant construction des catalogues ECS serveur/client. Les listes des RenderModel et RenderMaterialResource exposent également leurs ressources privées si elles sont demandées. Les clés et variantes natives restent dans leur ordre ; les erreurs natives restent inchangées. Construction transactionnelle des listes pour ne pas conserver de résultat partiel si une collision de GUID est détectée. Les archives restent intactes.

Tests : compilation /W4 /WX et suites hors jeu réussies. Nouveau test du parcours liste des templates → ouverture de la clé privée → résolution de son contenu, plus invariance des entrées natives, variantes, pointeurs stables, erreurs et types sans rapport. Validation visuelle en jeu encore nécessaire après installation.

## 0.1.23-dev — crash du modèle équipé à l’entrée du monde

Le dump `enshrouded_20261115_130802` montre l’assertion native « corrupted streamed texture parameters » (RVA 0xb56af), appelée par le chargement du matériau. L’entité et le modèle privés sont désormais résolus, mais le catalogue des textures a été construit avant l’activation du mod. Aucun contenu 3D n’avait encore été demandé au service de ressources. Le fait que la pince soit équipée au chargement expose immédiatement ce défaut ; il ne nécessite pas de modifier la sauvegarde ou l’équipement.

L’adaptateur intercepte la recherche de texture 0xb5580 uniquement pour les quatre identifiants du matériau privé. Avant la première recherche, il emploie l’enregistrement natif 0xb2fb0, les exigences mémoire GPU 0x10045b0 (structure de **24 octets**), l’allocation 0xfda3a0 et la liaison 0xfcab30. Le parcours reprend les étapes de l’initialisation native 0xbe410 : création de l’image, allocation distincte pour ses petits niveaux, liaison et marquage du descripteur à actualiser. Les données sont ensuite servies par le hook de contenu existant. L’allocation globale du jeu n’est ni remplacée ni reconstruite.

Les réserves d’entrées, le seuil de croissance de la table de hachage, le tableau de bits et la réserve de streaming sont contrôlés avant l’appel natif. Aucune réallocation de ces tableaux n’est autorisée. Les demandes répétées réutilisent l’enregistrement. Un échec retourne un handle nul plutôt que d’appeler la recherche native avec une clé absente. Les petites allocations GPU privées sont conservées pour la durée du processus ; leur libération indépendante et la recréation du périphérique restent hors du contrat actuel (un catalogue réinitialisé est refusé). Le correctif utilise le parcours graphique qui charge le matériau ; le comportement réel du pilote et du streaming doit être confirmé dans le jeu.

Validation hors jeu : compilation /W4 /WX ; suites SDK/configuration/transactions ; adaptateurs natifs ; nouvelle simulation du catalogue déjà initialisé avec pince équipée d’emblée ; ordre enregistrement/allocation/liaison/recherche ; quatre textures, répétitions, transfert inchangé des textures natives ; capacité épuisée, seuil de réallocation, allocation échouée et cache devenu périmé ; décodage indépendant des trois LODs et quatre chaînes de textures ; validation statique du paquet. Ces tests utilisent des substituts des fonctions GPU et ne certifient pas le rendu en jeu. Essai en jeu réservé à Alain.

## 0.1.24-dev — boule lumineuse native sur la pince

Comparaison des templates natifs du marteau `618b3ac8dbea284d914688c4c8d9cb98` et de la baguette d’invocation `0b6db1ac5b832949a2f00c6046bfc35c` : tous deux référencent exactement le même VFX `1973ad464d14b043a9ee842d1c2355c9` (type `0xe64ee036`). Leurs offsets locaux sont respectivement (0, 0.4, 0) et (0, 0.49, 0). La réflexion native confirme VfxComponent.attachmentOffset à +8, avec localOffset / worldOffset / orientationOffset. Il s’agit donc d’une ressource existante à réutiliser, sans recréer son animation, ses particules ou ses textures.

Le pack conserve désormais les neuf composants du template visuel, dont VfxComponent et VfxComponentResource. Seuls les trois flottants localOffset de l’effet changent : (-0.014, 0.209, 0) en coordonnées du modèle. La position est prise au centre de l’articulation, mesurée sur le modèle préparé ; profondeur centrée dans la pince. Le SlotAttachment du modèle, l’orientation et les flags de l’effet sont préservés. Aucun nouvel effet, texture ou hook n’est ajouté, et les ressources natives du marteau et de la baguette restent inchangées.

Tests : génération et décodage indépendant du pack, égalité de la référence VFX des trois outils, comparaison octet par octet du composant natif hors position locale, neuf composants et bornes des blobs ; compilation /W4 /WX ; suites SDK, sélection, transaction, adaptateurs natifs, textures et catalogue ; validation statique du paquet. Tous passent. Le rendu animé et le centrage visuel restent à confirmer par Alain en jeu. La version 0.1.23 (modèle et textures) a été confirmée fonctionnelle par Alain avant ce changement.
