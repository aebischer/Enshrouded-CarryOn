# Validation de publication — 18 novembre 2026

Version : 0.1.48-dev, ABI 1, chargeur 0.6.10. SDK synchronisé sur le commit
amont `f9c138b119c677e16b0eb6d520b916f95abb1bfd`.

La commande `tools/validate.ps1` a terminé avec succès avant publication :

- Compilation C++ x64 avec `/W4 /WX`, intégration SDK et politiques de transport.
- Préservation des 4 213 objets et 2 542 recettes natifs ; recette de la pince.
- Adaptateurs de rendu, collisions, inventaire, confort, portes et onze puits.
- Téléportation et 144 parcours d’interruption sur quatre familles d’objets.
- Réplication différée et régression de l’arrondi réseau des rotations.
- 256 combinaisons de lectures mixtes et lot natif de 128 lectures préservé.
- Reprise après échec d’allocation, limites de traitement et 2 000 résultats
  privés consommés concurremment, dans l’ordre et exactement une fois.
- 5 769 profils de pose valides sur 6 282 modèles ; refus explicites du reste.
- Quinze langues, quatorze clés de texte et validation du manifeste final.
- Décodage indépendant du modèle : trois LOD, géométrie, UV, normales, chaînes
  de textures et attachement de l’orbe natif.
- Validation statique du paquet final par le validateur du chargeur.

Une exportation propre des seuls fichiers préparés pour GitHub a également
passé `build.cmd` et ses tests, sans les fichiers ignorés du poste de travail.
Les extractions natives restent locales et ne font pas partie du dépôt.

Ces résultats sont hors jeu. Ils ne certifient ni le comportement de chaque
objet en partie réelle, ni un gain de temps de chargement, ni le multijoueur.
Les essais utilisateur sur la 0.1.48 restent distincts de cette validation.
