Note historique : ce document décrit les étapes de développement à sa date.
Pour le fonctionnement actuel et les limites connues, consulter ../README.md.

# Nettoyage CarryOn 0.1.25

## Corrections

- Définition de l'ItemInfo isolée dans `pincer_item.inl`, avec offsets nommés
  et contrôle du format donneur. Consommation à la pose et son de placement
  hérités neutralisés. Identités, modèle, icône, textes et programme distincts.
- RecipeInfo construite explicitement : une ferraille, une pince, fabrication
  manuelle instantanée, sans station, énergie, abri ni amélioration. Deux
  conditions de progression déjà validées sont conservées explicitement.
- Liste fermée des neuf composants visuels dans le générateur : une évolution
  du modèle donneur provoque un refus, jamais un héritage silencieux.
- Réponses des ressources privées publiées dans une file consommée par le
  lecteur natif ; réservations décrémentées exactement une fois. Plus d'écriture
  directe dans sa file circulaire. Une erreur avant publication est renvoyée
  au lecteur natif. L'analyse a aussi établi que le moteur réserve la capacité
  avant soumission : le risque de saturation annoncé dans l'audit était donc
  surestimé pour des appels respectant ce contrat.
- Synchronisations client sans limite arbitraire de 16 : réservation mémoire
  avant toute modification serveur et remplacement d'une ancienne destination
  pour la même entité. Échec prolongé signalé, réparation conservée sans bloquer
  les autres déplacements. Purge au changement de monde ou d'identité.
- Enregistrement des textures reprenable après un échec GPU ; pas de seconde
  insertion et pas d'accès natif avant liaison mémoire. Détection d'un catalogue
  réinitialisé. Aucun verrou conservé pendant la recherche native finale.
- Actions natives de commande à la place de Tab/Échap et boutons souris physiques.
  Les politiques de focus, menus, fraîcheur et fronts restent en place.
- Verrou libéré avant traitement natif des commandes ; protection contre la
  réentrée des adaptations pendant une transaction. Le verrou reste conservé
  pendant les écritures atomiques et les validations qui en dépendent.
- Installation des hooks avec contrôle de chaque retour, restauration du menu,
  libération de sa copie non publiée et refus de recommencer après un rollback
  incomplet. DLL épinglée avant activation, conformément à sa durée de vie ABI 1.
- Suppression d'observations diagnostiques jamais lues et du compteur redondant
  des synchronisations. `move_objects` reste reconnu pour les anciens profils,
  mais les quinze langues l'annoncent indisponible.
- Commande de validation complète, prérequis contrôlés, rapport avec empreintes.

## Héritage volontaire et limites

Le format ItemInfo contient des données relatives : la base native complète
reste copiée pour conserver ces liens et les valeurs génériques de l'équipement.
Ce n'est pas encore un sérialiseur ItemInfo autonome. La recette conserve les
requêtes natives 0x6807c65f et 0x71ee0e45 ; leur signification précise reste à
identifier avant de changer les règles de déblocage. Les retirer sans preuve
n'est pas une correction sûre.

La tenue en main, les composants graphiques, l'orbe orange et le solveur de pose
sont des services natifs réutilisés intentionnellement. Les allocations GPU ont
la durée de vie du processus : aucun destructeur natif non vérifié n'est appelé.
Le verrou des textures protège les appels du mod ; il ne prouve pas à lui seul
l'absence de concurrence avec tous les travaux internes du moteur.

Les tests simulent l'ABI native et ne certifient ni le planificateur graphique
réel ni toutes les pannes de MinHook/VirtualProtect. Les changements de commandes
nécessitent aussi une validation en jeu, notamment après réaffectation des touches.
Aucune prise en charge nouvelle des serveurs, téléportations ou autres objets.

## Régressions supplémentaires testées

Textures : échec d'allocation suivi d'une reprise réussie, sans double insertion.
Ressources : 65 réponses privées, deux demandeurs isolés, ordre et compte exacts.
Déplacement : 32 réparations en attente, nouveau déplacement et regroupement de
la même entité ; appel imbriqué pendant le verrou sans nouvelle écriture.
Entrées : actions primaires/secondaires natives et refus d'un contexte périmé.
Contenu : refus d'un ItemInfo tronqué et neutralisation de consommation/son.
Les tests existants couvrent aussi les inventaires, les refus de pose, la
rotation, les menus, le focus, le monde, les icônes, les textes et l'effet orange.

Le joueur réalise les essais visuels, les commandes et la sauvegarde/recharge.

## Correctif 0.1.26

Le test en jeu a révélé une régression de l’entrée en mode transport.
La capture directe de Tab/Échap est rétablie ; la promesse de réaffectation
de ces touches est retirée. Le test précédent appelait une traduction clavier
uniquement dans son propre code et ne prouvait pas le raccordement réel.
Le test supplémentaire appelle désormais la fonction de routage de production,
sans contexte d’action native, et vérifie appui, répétition, relâchement,
transmission des autres touches et refus hors du contexte autorisé.
