# Décors sans ItemInfo de placement — 0.1.37

Premier modèle : `Prop_Destructible_Villageprops_bottles_12`, GUID
`ee386f839f792f4fa64f4a31593a2a1a`. Son collider natif est une boîte de
30 × 66 × 30 cm, centrée à 33 cm au-dessus du pivot. Aucun ItemInfo natif
ne référence ce modèle dans le catalogue examiné.

## Adaptateur commun

Le catalogue natif est consulté en premier. Pour un Prop dynamique éligible au paramètre Décors
mais dépourvu de descripteur, `floor_placement.h` construit un
descripteur complet privé à partir du profil natif de pose au sol du comptoir
(`e3a56abbb97f3946ad1fbd266c5ccace`). Ce profil fournit les commandes et les
règles de sol ; ses dimensions et son modèle sont remplacés par ceux de la cible.
Les tableaux de collisions sont repris depuis la ressource du modèle, avec
leurs enfants et leurs références relatives. Une copie du seul en-tête serait
invalide. Les ingrédients et la consommation d’objets sont désactivés.

Les bounds de placement et d’alignement sont calculés à partir de toutes les
formes : boîtes, sphères, capsules, cylindres et capsules effilées. La réflexion
native confirme leurs champs et le parseur indépendant confirme l’axe Y et la
longueur totale du segment des capsules. Les sphères restent invariantes par
rotation ; les capsules et cylindres utilisent leur fonction de support. Les
collisions originales restent intactes pour la validation native.

Les dimensions signées présentes dans les ressources (y compris le comptoir)
sont prises en valeur absolue pour calculer une enveloppe. Les ressources de
collision vides sont ignorées si d’autres contiennent de la géométrie. Sans
aucune forme, ou en présence de données invalides ou d’une forme inconnue, le
modèle est refusé. Le composant `DoNotSave` interdit le placement de secours.
L’activation de Décors ne rend pas sauvegardable une entité qui ne l’est
pas, et ne transforme pas un assemblage sans collision propre en objet unique.

Le cache possède les allocations pour toute la durée du processus. Les modèles
partagent l’ID numérique du profil mais sont résolus par leur GUID propre, côté
client comme côté serveur. Aucun nouvel objet d’inventaire n’est enregistré.
Les blobs ne sont pas supprimés au changement de monde, car les callbacks natifs
d’entrée peuvent conserver leur adresse entre deux frames.

## Déplacement et persistance

Le parcours reste celui des coffres et du comptoir : aperçu distinct, validation
native de la destination dans la même claim, puis modification de l’entité
existante et synchronisation des collisions et du rendu. Les composants du décor
restent en place, notamment le confort lorsqu’il existe.

Les Props dynamiques de famille 8 passent par le sérialiseur natif déjà étudié
dans RECHERCHE-DECORS.md : identité du template et CurrentTransform complet.
L’adaptateur ne crée pas une copie permanente et ne supprime pas l’original.
Les instances statiques encodées dans la scène restent exclues.

## Validation

`tools/validate.ps1` ajoute l’extraction native des bouteilles aux fixtures locales
(non publiées). Tests : bounds réels et boîte tournée, intégrité des offsets,
géométrie malformée, cache partagé entre modèles, aperçu/validation/commit,
refus natifs et changement d’identité, collisions/rendu et conservation de la
transformation serveur. Les régressions coffre/comptoir restent exécutées.

Ces tests hors jeu ne remplacent pas l’essai réel : déplacer les bouteilles,
vérifier leur aperçu et leur contact au sol, sauvegarder/quitter normalement,
recharger et vérifier une seule instance à destination, puis la déplacer à
nouveau. Tester aussi rotation, annulation et refus hors claim.

## Correction 0.1.36 : saisie des bouteilles détachées stables

Le relevé serveur du 16 novembre (bouteille 2853) montre AttachToSurface :
`ffffffffffffff7f0000000001000000`. La réflexion native identifie les états
0 Uninitialized, 1 Detached, 2 Attached. Le filtre initial exigeait Attached,
ce qui autorisait le surlignage mais bloquait la saisie de cette bouteille.

Le snapshot accepte désormais aussi Detached avec searchStartTime = INT64_MAX,
sans parent, destruction, gravité ajoutée ni détachement forcé. Il conserve cet
état sans le convertir. Le bloc d’attachement complet est comparé pendant
l’aperçu et avant le commit : une transition annule la transaction.
Les tests utilisent les 16 octets du relevé réel et couvrent les refus associés.

## Généralisation 0.1.37

L’inventaire des 6 282 templates de famille Prop du client vérifié contient quatre
formes : boîte (3 815 modèles), sphère (1 611), capsule (1 207), cylindre (2).
Ces nombres se recoupent pour les modèles à plusieurs formes. Aucune exception
par nom n’est nécessaire. La capsule effilée est également décodée selon la
réflexion native, mais n’apparaît pas dans ce corpus de Props.

Le test de corpus construit 5 769 descripteurs. Les 513 refus sont contrôlés :
170 templates DoNotSave, 240 sans géométrie, 103 sans composant de collision.
Les tests numériques vérifient rotations, dimensions signées, formes asymétriques,
conservation des variantes natives et refus des données tronquées. Les tests de
transaction, d’attachement et de réplication sont réexécutés. La préparation d’un
descripteur ne certifie pas un déplacement en jeu de chacun de ces modèles.

## Correction 0.1.38 : objet posé sur un support

Relevé serveur : pot `Prop_Flowerpot_01_small_pink`, entité 3393, attaché au
buffet `Prop_Furniture_Castle_Sideboard_Large_01`, entité 3372. État complet :
`ffffffffffffff7f2c0d000002000000`. Le filtre refusait tout attachmentContainerId
non nul avant de créer l’aperçu. Un rechargement pouvait recalculer ce lien et
masquer le défaut après déplacement du support.

Un Prop Attached peut maintenant référencer un support. Les états Detached avec
parent, les références à soi-même et les indicateurs de destruction/gravité/
détachement forcé restent refusés. Le snapshot capture l’état complet et annule
la transaction s’il change pendant l’aperçu. Le support n’est jamais déplacé.

À la pose validée seulement, le composant d’attachement est obtenu par l’accès
en écriture ECS natif, vérifié intégralement, puis son cache de support est
invalidé : attachmentContainerId=0, searchStartTime=0. L’état Attached et les
autres indicateurs restent conservés. Le code ne déclenche pas l’action native
de détachement, qui peut détruire l’objet ou ajouter de la gravité. La recherche
native de support peut alors reprendre à son prochain traitement/réveil.

Preuves natives : attach_to_surface à 0x32f990, recherche à 0x351340. À 0x3513f6,
INT64_MAX court-circuite la recherche ; à 0x3514fd, zéro initialise une nouvelle
recherche ; à 0x3516c2, l’identifiant trouvé est écrit à +8. Le nouveau cache
n’est pas calculé par le mod et son rafraîchissement effectif reste à observer
en jeu. Ni l’aperçu ni son annulation ne modifient l’attachement.

Tests : état exact du pot, saisie, annulation, refus natif, support changé en
cours d’aperçu, échec d’accès en écriture/expiration sans déplacement partiel,
commit, synchronisation graphique et seconde saisie immédiate sans rechargement.
La validation complète hors jeu passe. Test en jeu conseillé : prendre le pot
sans toucher au buffet, annuler, le reprendre et le poser, puis le reprendre
immédiatement et vérifier la persistance après sauvegarde/rechargement.

## Configuration 0.1.39

Le filtrage par fichier et par nom est supprimé. Seuls les trois paramètres par
monde restent : Décors, Coffres et Debug. Les contrôles natifs et la politique
d’attachement restent appliqués.
