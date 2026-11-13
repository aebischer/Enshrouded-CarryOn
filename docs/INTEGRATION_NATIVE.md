Recherche native — 13 novembre 2026

Statut : inspection statique préliminaire, aucun hook installé.
Exécutable de référence du chargeur :
SHA256 5D9EF2F140B8ED49E30DBEE655841B89D08BAEC20F799A1817E171A18A850C77.

Les chaînes suivantes existent dans l'exécutable local (RVA de la chaîne,
PAS adresse d'une fonction appelable) :
- 0x1c28c50 : keen::ds::ecs::BuildingPickUpEvent
- 0x1d48810 : StorageChest
- 0x1d48778 : ReadOnlyLootChest
- 0x1c257d0 : ObjectReference<keen::ecs::DefaultInventorySetup>
- 0x1c25c48 : keen::ecs::DefaultInventoryReference
- 0x1c34cb0 : keen::ecs::InventoryTransferAction
- 0x1c45c88 : keen::ecs::UpgradeStorageAction
- 0x1c221e0 : keen::actor::PlaceVoxelObjectEvent

Interprétation limitée : pistes vers les mécanismes de prise, de stockage et
de placement. Les noms ne prouvent ni les signatures, ni la conservation de
contenu, ni la possibilité de déplacer un coffre. PlaceVoxelObjectEvent pourrait
concerner des objets voxel et ne doit pas être assimilé au placement d'un coffre.

Recherche de références LEA RIP-relatives directes pour BuildingPickUpEvent et
StorageChest : aucun résultat avec le scanner existant. Cela n'exclut pas les
tables de réflexion, références indirectes ou autres instructions.

Prochaines étapes
1. Identifier les métadonnées de réflexion, composants et gestionnaires natifs
   de BuildingPickUpEvent et StorageChest, avec signatures et propriétaires.
2. Observer un coffre vide puis rempli dans une copie de monde ; identifier
   entité persistante, inventaire, transformation et état de sauvegarde.
3. Vérifier le chemin natif de déplacement/placement et ses validations.
4. Brancher la mécanique sur ces fonctions ; contrôler la version du jeu avant
   tout hook et refuser les versions inconnues.
5. Tester coffre rempli, dépôt refusé, annulation, sauvegarde/rechargement,
   mort/retour au menu et absence de duplication.
6. Étendre aux autres familles d'objets après validation de leur état propre.

Validé pour ce premier jalon
- Compilation x64 /W4 /WX.
- Tests sur la vraie DLL : exports, configuration valide/invalide, changement
  de monde, ABI invalide, absence de réutilisation après un Configure refusé.
- Initialisation refusée explicitement tant que l'intégration native manque.
- Pas de test de déplacement en jeu effectué ; aucune version installée.

Les extractions du jeu restent locales, hors du dépôt.
