# Objets utilisables : portes — 0.1.40

Le réglage par monde `move_interactive` (Objets utilisables) est indépendant de
Décors, Coffres et Debug. Les quatre réglages sont traduits dans quinze langues.
Première prise en charge : portes animées, pas encore puits et autres appareils.

## Contrat natif observé

La porte visée est `Prop_Door_Small_Castle`, template
`550d7a506569c348bc33efdf9fb894d4`, entité serveur 1407. Son battant physique est
l’entité 3828, `Door_Collision_Small_Wooden`, template
`8b170735e07e6044b0f3be80a837adba`. La porte est de famille **Switch (10)**,
et non Prop (8). Ce point est vérifié dans la réflexion native et la ressource.

La porte possède Collider (24 octets), ClientCollider (16), ServerToggleState,
InteractionToggle, ServerAnimationGraphSlotComponent et DynamicSubEntities
(16 identifiants). L’enfant possède ServerSlotAttachment (112 octets), avec
l’identité du parent à +44, et son propre Collider. Ses données DoNotSave sont
gérées par le cycle natif des sous-entités ; le mod ne les crée ni ne les détruit.

La classification charge les composants du template, pas une liste de noms.
Elle exige Switch, InteractionToggle, ServerAnimationGraphSlotComponent,
SubEntities et Collider, sans DoNotSave. Un objet interactif non pris en charge
ne passe pas par Décors pour contourner le réglage.

## Déplacement

Le snapshot serveur vérifie l’état d’ouverture, les enfants, leurs identités,
leurs handles de collision et leurs attaches locales. Un changement pendant
l’aperçu invalide la transaction. Les colliders racines doivent employer une
ressource unique, le setup 0 et le slot racine ; les autres montages sont refusés.

Le commit conserve l’entité principale et son état, déplace sa transformation et
sa collision via le gestionnaire natif partagé. La distinction statique/mobile
du record de collision est vérifiée. Le marquage path-grid respecte isPathBlocker.
Le rendu et la collision client sont synchronisés avec la transformation répliquée.

Les enfants restent attachés : `server_slot_attachment` puis
`server_slot_attachment_write` (0x349aa0 / 0x34a120) mettent à jour leurs
transformations ; `move_colliders` (0xa1d0c0) applique les poses animées avec
le même déplacement natif 0xa1fdc0. Aucune translation indépendante du battant
ni modification du graphe d’animation n’est injectée.

## Sauvegarde et tests

Le dispatch 0xe187b0, famille 10, appelle 0xdc24d0 puis 0xdc5400.
La transformation complète est copiée à 0xdc5690–0xdc56b7 ; les états natifs
suivent dans le même enregistrement Switch. L’identité de la porte est conservée.

Tests hors jeu : ressource réelle de la porte de château, classification et
descripteur de placement, réglages indépendants, états ouvert/fermé, préservation
des attaches et du toggle, refus d’un enfant absent/changé ou d’une interaction
pendant l’aperçu, collision/rendu racine et régressions existantes.

Ces tests n’exécutent pas l’animation du moteur. Essai en jeu nécessaire :
déplacer la porte fermée puis ouverte, vérifier rotation, interaction des deux
côtés, collisions du battant et absence de collision à l’ancienne position ;
sauvegarder/recharger, vérifier une seule porte fonctionnelle et la déplacer
à nouveau. Vérifier aussi Objets utilisables désactivé avec Décors activé, puis
l’inverse. Les limites de claim restent applicables.

Retour utilisateur du 17 novembre 2026 : déplacement des portes fonctionnel. La conservation après rechargement n’a pas été confirmée séparément.
