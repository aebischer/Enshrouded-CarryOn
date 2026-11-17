# Comptoir de décor — intégration 0.1.30

## Périmètre

Un seul modèle autorisé : `Prop_Furniture_Comfort1_Bar_01_Wood`, GUID archive
`423c01619630d241b98a591808d9ea21`, famille Prop (8), instance dynamique.
Les identifiants de scène (bit haut) ne sont pas admissibles.

Le jeu fournit déjà son ItemInfo de placement :
`e3a56abbb97f3946ad1fbd266c5ccace`, ID `0xf60b84c2`, nom de debug
`Prop_Decoration_Comfort2_T1_Tavern_Counter`. La référence placedEntity pointe
exactement vers ce modèle. Aucun ItemInfo synthétique, nouvelle recette ou
nouvel objet d'inventaire n'est ajouté. La ressource reste dans son registre,
car ses références internes sont relatives à son emplacement.

## Chemin d'exécution

- Ciblage natif : priorité au slot coffre 2, puis slots décor 0/4. Le modèle
  exact et la famille sont vérifiés ; distance maximale de 3 mètres conservée.
- Saisie : validation serveur de l'entité, de la claim et des collisions.
  InventorySetup et UsedItem restent requis pour les coffres, pas pour ce comptoir.
- Fixation : accepter uniquement l'état observé 2, sans attachmentContainerId,
  shouldDestroy, hasAddedGravity ni enforceDetached. Aucun de ces champs n'est
  réécrit. Les états non pris en charge sont refusés.
- Aperçu et commandes : descripteur natif du comptoir ; vérification de son lien
  au modèle côté client et serveur ; solveur et permissions natifs conservés.
- Pose : même transaction que les coffres, dans la même claim, sur l'entité
  existante. Accès mutable natif à CurrentTransform et déplacement des collisions.
- Synchronisation client : vérifier l'identité de modèle et l'ID de placement,
  attendre la transformation répliquée, mettre à jour les trois poses de rendu
  et les collisions. L'absence de UsedItem ne doit pas supprimer cette tâche.
- Confort : conserver ComfortProvider et vérifier qu'il n'a pas changé pendant
  la transaction. Le moteur actualise les cellules spatiales et recalcule le
  confort depuis la position courante. Aucun bonus artificiel ajouté au joueur.
- Persistance : parcours natif Prop ; aucun fichier annexe, appel de destruction,
  création de copie ou modification des bits de scène. Références natives dans
  [RECHERCHE-DECORS.md](RECHERCHE-DECORS.md).

## Validation hors jeu

La suite complète inclut la ressource ItemInfo extraite du jeu (identité, modèle,
bounds), le ciblage des slots 0/4, le refus des autres modèles/familles et des
entités de scène, l'absence d'inventaire et de UsedItem, la fixation non admissible,
la modification du confort pendant la saisie, les refus de collision/permission,
la rotation, le commit unique et la synchronisation rendu/collisions.
Les tests existants des coffres, commandes, ressources et du SDK restent exécutés.

Les doubles natifs des tests ne constituent pas un test de sauvegarde du moteur.
L'empreinte du paquet testé est enregistrée dans `build/validation.json`.

## Essai en jeu confié à Alain

1. Viser le comptoir en mode transport ; vérifier son surlignage et son aperçu.
2. Annuler une saisie : le comptoir doit rester inchangé.
3. Saisir, tourner et poser dans la même claim : image et collision doivent suivre,
   sans meuble fantôme ni blocage invisible à l'ancienne position.
4. Vérifier le confort près du nouvel emplacement et loin de l'ancien. Le moteur
   retient le meilleur fournisseur par catégorie : une autre table peut masquer
   la différence. La portée observée pour cette catégorie est de 15 mètres.
5. Sauvegarder normalement, revenir au menu et recharger : un seul comptoir à
   destination, aucun à l'origine, confort conservé ; le déplacer une seconde fois.
6. Vérifier également un coffre ordinaire/magique et le refus hors de la claim.

État à la préparation du paquet : tests hors jeu réussis ; validation visuelle,
confort effectif et persistance après déplacement/rechargement encore à effectuer.

## Correction 0.1.31 : synchronisation après rotation

Le test en jeu de la 0.1.30 a révélé un déplacement sans mise à jour du rendu,
intermittent selon la rotation. Le relevé en lecture seule du 16 novembre sur
l'entité 2771 montre une transformation serveur correcte et une transformation
client répliquée, mais RenderTransform conserve l'ancienne position.

Quaternion demandé/serveur : `(0, 0.5000692606, 0, 0.8659854531)`.
Quaternion reçu : `(0, 0.4995411932, 0, 0.8662901521)`.
L'écart de position est de 0,488 mm, dans la tolérance existante ; l'écart de
rotation dépasse en revanche l'ancien seuil de 0,0001 par composante. La file
attendait donc indéfiniment une égalité que la quantification réseau empêche.

La comparaison utilise désormais l'écart angulaire normalisé, limité à 0,25°,
avec équivalence de q et -q. Cette limite est une tolérance d'acquittement, pas
une modification des permissions de pose ni une description exhaustive du codec
réseau. Les contrôles d'identité, de position, d'échelle et de validité restent
en place. Le rendu et les collisions client utilisent la pose réellement reçue ;
la transformation serveur n'est pas modifiée par cette synchronisation.

Régression : valeurs exactes du relevé, ancienne rotation et rotation différente
refusées, changement d'échelle/position refusé, file conservée avant réplication
puis terminée après réception arrondie, pour un coffre comme pour le comptoir.
Témoin local : `work/counter-sync-live.json` ; lecteur `work/read-counter-sync.py`.
