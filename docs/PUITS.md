# Puits à eau — 0.1.43-dev

Le réglage Objets utilisables autorise les puits fonctionnels indépendamment de
Décors et Coffres. Aucune liste de noms ou de modèles n’est utilisée en production.

## Contrat natif

Relevé en lecture seule du 17 novembre 2026 : entité 1479,
`Prop_NPC_Water_Well_c`, template `abc315119baca14c9a4bf92d9bfa04c2`.
Famille 13 ; Fuel (16 octets), DynamicFuelSetup (4), NetworkFuel (4),
StaticCollider côté serveur et StaticClientCollider côté client (16 chacun).
Fuel porte un en-tête d’attributs, suivi des valeurs à +8/+12 ; il ne faut pas
le traiter comme un simple compteur ni réécrire sa valeur de départ.

La classification exige les composants Fuel, FuelSetup, FuelCraftingStock,
NetworkFuel, les deux collisions statiques et InteractionOfferRuntime avec
la séquence native de puisage 0xf5024957. DoNotSave, les sous-entités et les
collisions animées sont refusés. Les onze variantes actuelles (village, désert,
marais et constructibles) partagent ce contrat. Les tests emploient leurs GUID
pour retrouver les ressources réelles ; ces GUID ne filtrent pas le mod.

## Transaction

Le chemin commun produit l’aperçu avec le descripteur natif ou les collisions
du modèle. Il conserve les limites de claim, la validation native, la rotation,
le déplacement de collision et la synchronisation graphique existants.
Le snapshot distingue porte et puits : seuls les battants suivent la voie de
collision dynamique. Les puits gardent leur entité et tous leurs composants.

La réserve et le remplissage ne sont jamais restaurés depuis le snapshot :
une recharge naturelle pendant l’aperçu doit rester acquise à la pose.
Un Fuel/refill absent, un PNJ propriétaire (DynamicActiveNpcTarget +24) ou un
animal associé (DynamicAnimalTarget +4) interdit la saisie ou annule la pose.
Les systèmes natifs npc_active_target et animal_target lisent CurrentTransform ;
le commit marque cette transformation via le gestionnaire natif habituel.

## Persistance et validation

Le dispatch 0xe187b0, famille 13, mène à 0xdc3160 puis 0xdc5d60.
Le writer copie CurrentTransform à 0xdc5f59–0xdc5f7b et sérialise les attributs
Fuel/MaxFuel à 0xdc60b0–0xdc60be. Le déplacement conserve l’identité et passe
par le marquage natif de transformation ; aucun puits neuf n’est créé.

Tests hors jeu : onze ressources réelles, géométrie de placement, refus d’une
interaction différente ou non persistante, indépendance des réglages, réserve
modifiée pendant l’aperçu, refus des utilisateurs PNJ/animal, collision/rendu,
et panneau natif de placement pour le puits. Les régressions coffres/décors/portes
restent exécutées. Ces tests n’exécutent pas le cycle de sauvegarde du moteur.

À vérifier par Alain en jeu : déplacer, puiser, vérifier collision et disparition
à l’ancienne position, recharger et retrouver un seul puits utilisable au nouvel
endroit ; essayer une autre apparence et Objets utilisables désactivé.

Retour Alain : déplacement, fonctionnement et conservation après sauvegarde/rechargement confirmés le 17 novembre 2026.
