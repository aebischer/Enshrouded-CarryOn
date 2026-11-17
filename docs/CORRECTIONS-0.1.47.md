# Corrections de l’audit — 0.1.47-dev

Cette version corrige les défauts reproduits lors de l’audit de la 0.1.46.
Les validations décrites ici sont hors jeu ; Alain réalise les essais en jeu.

## Déplacement et durée de vie

- La synchronisation graphique suit désormais la position autoritaire si le
  serveur réajuste l’objet après la pose. Une pose différente ne supprime plus
  silencieusement la réparation graphique en attente. Une entité disparue ou
  dont le modèle a changé reste exclue.
- Une pose expirée est annulée avant toute écriture ou déplacement de collision.
- Les commandes de souris sont copiées en valeurs avec date et génération de
  session. Aucun pointeur de mapping ne survit à son callback natif ; la
  destruction du monde efface aussi cet état.
- Les échelles non uniformes sont refusées dès la sélection et à la validation
  serveur. Le déplacement natif de collision utilisé exige une échelle uniforme.
- Les visuels sans handle `Model` ne sont plus proposés à la saisie : le chemin
  de déplacement graphique de `VoxelModel` n’est pas pris en charge. Le diagnostic
  peut toujours identifier ces objets.

## Ressources et commandes

- L’indexation conserve les seuls IDs encore indisponibles et les retente après
  une seconde. Un registre incomplet n’est plus marqué comme terminé.
- Un échec temporaire de lecture ou d’allocation d’un profil au sol est retenté
  avec temporisation. Seuls les formats structurellement incompatibles restent
  exclus. Les allocations de profils déjà publiées restent stables.
- Entrée/sortie et annulation utilisent les actions natives 129/126. Leur filtre
  d’éligibilité est adapté uniquement dans le contexte de la pince en jeu.
  La capture Windows de Tab/Échap a été retirée. Les bindings et leurs glyphes
  restent ceux du jeu ; réaffectation clavier et manette nécessitent encore
  une confirmation en jeu.
- Les profils de pose dérivés du comptoir affichent le libellé localisé « Décors »
  et ne réutilisent plus son nom, sa légende ou sa description.
- La limite de taille des ressources inclut l’alignement de l’ajout.
- Une réinitialisation du catalogue graphique peut reconstruire les textures
  privées. Les anciens handles et allocations GPU ne sont ni réutilisés ni
  libérés arbitrairement par le mod.
- Le résultat de `LoadResource` est vérifié. Les gros tampons du contrôle SHA-256
  sont alloués sur le tas avant l’ouverture des handles système.

## Nettoyage

Classification des décors partagée, hachage des types unique, masque de refus
de création nommé, noms des fonctions de synchronisation et de comparaison
généralisés aux objets. Les champs inutilisés des transitions de sélection et
quatre clés de traduction inutilisées ont été supprimés. Les contrôles client
et serveur restent indépendants. La provenance du header de localisation SDK
est épinglée à la révision vérifiée du chargeur 0.6.10.

## Vérifications

Les régressions couvrent une pose expirée avant mutation, les réajustements
serveur suivis d’une reprise graphique, les ressources tardives, le refus précoce
des visuels incompatibles, le changement de session et la recréation du catalogue
de textures. Les commandes natives sont testées avec fronts, maintien, menus,
blocage et délégation des autres actions. Les suites existantes couvrent les
coffres, décors, portes et puits, 144 interruptions et le transport entre claims.

La validation complète inclut également le corpus de géométrie, les catalogues
natifs, les traductions, le modèle 3D et le paquet SDK. Le rapport de chaque
exécution réussie est écrit dans `build/validation.json`, avec les empreintes du
paquet exact et `gameplayTested=false`.
